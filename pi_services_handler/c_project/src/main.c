#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <math.h>

#include "config.h"
#include "joystick.h"
#include "motion_control.h"
#include "socket_comm.h"
#include "serial_comm.h"
#include "data_parser.h"
#include "qmc5883p.h"
#include "pid_controller.h"

static volatile int keep_running = 1;
static float trajectory_points[2 * MAX_POINTS];
static size_t points_count = 0;
static size_t current_point_idx = 0;
static bool route_mode = false;
static float current_latitude = 0.0f;
static float current_longitude = 0.0f;
static bool position_valid = false;
static qmc5883p_t qmc_sensor;
static pid_controller_t pid;

static void handle_signal(int sig)
{
	(void)sig;
	keep_running = 0;
}

static void on_socket_data(const char *data)
{
	if (!data || data[0] != '@')
		return;

	float parsed_points[2 * MAX_POINTS] = {0};
	size_t parsed_count = 0;
	query_type_t qtype = parse_client_command(data, parsed_points, &parsed_count);
	if (qtype == QUERY_POINTS && parsed_count > 0)
	{
		if (parsed_count > MAX_POINTS)
			parsed_count = MAX_POINTS;
		memcpy(trajectory_points, parsed_points, parsed_count * 2 * sizeof(float));
		points_count = parsed_count;
		current_point_idx = 0;
		route_mode = true;
		fprintf(stdout, "[main] received %zu points path\n", points_count);
	}
	else if (qtype == QUERY_NSTART)
	{
		if (points_count > 0)
		{
			route_mode = true;
			current_point_idx = 0;
			fprintf(stdout, "[main] navigation start\n");
		}
	}
	else if (qtype == QUERY_PGET)
	{
		// 这里可以向客户端发送当前点或状态，需socket模块支持
		fprintf(stdout, "[main] Pget received, points_count=%zu\n", points_count);
	}
	else if (parse_mcu_response(data, &current_latitude, &current_longitude))
	{
		position_valid = true;
		fprintf(stdout, "[main] received MCU @g response: lat=%.6f lon=%.6f\n", current_latitude, current_longitude);
	}
}

int main(void)
{
	signal(SIGINT, handle_signal);

	if (!socket_comm_start())
	{
		fprintf(stderr, "socket_comm_start failed\n");
		return EXIT_FAILURE;
	}
	socket_set_receive_callback(on_socket_data);

	serial_handle_t serial;
	if (serial_open(&serial, SERIAL_PATH, BAUD_RATE) != 0)
	{
		fprintf(stderr, "serial open failed\n");
		// 继续执行仅手柄+socket跟踪
	}

	int js_fd = xbox_open(DEVICE_PATH);
	if (js_fd < 0)
	{
		fprintf(stderr, "unable to open joystick device: %s\n", DEVICE_PATH);
		// 继续执行网络与串口逻辑
	}

	if (!qmc5883p_init(&qmc_sensor, "/dev/i2c-1", 0x2C))
	{
		fprintf(stderr, "QMC5883P init failed, using fallback navigation heading only\n");
	}

	pid_controller_init(&pid, 0.5f, 0.1f, 0.2f, 70.0f);

	xbox_map_t map_data = {0};
	(void)map_data; // [optional] keep variable to avoid unused warnings when logic changes

	while (keep_running)
	{
		bool manual_control = false;
		car_status_t status = CAR_STATUS_DEFAULT;
		if (js_fd >= 0 && xbox_map_read(js_fd, &map_data) == 0)
		{
			manual_control = true;
			joystick_state_t st = {
			    .lx = map_data.lx,
			    .ly = map_data.ly,
			    .rx = map_data.rx,
			    .ry = map_data.ry,
			    .lt = map_data.lt,
			    .rt = map_data.rt,
			    .a = map_data.a,
			    .b = map_data.b,
			    .x = map_data.x,
			    .y = map_data.y};

			status = get_status(st.a, st.b, st.y);
			st = apply_status_to_joystick(status, st);

			joystick_converted_t conv = convert_joystick_data(&st);
			motor_speed_t motors = parsed_motors_speed(conv.v_cx, conv.omega);
			motor_angle_t angles = parsed_motors_angle(conv.lt, conv.rt);

			char cmd_buf[128];
			snprintf(cmd_buf, sizeof(cmd_buf), "@m/%d/%d/%d/%d*",
				 motors.v_fl, motors.v_fr, motors.v_bl, motors.v_br);
			socket_put_data(cmd_buf);
			if (serial.fd >= 0)
			{
				serial_write(&serial, (const uint8_t *)cmd_buf, strlen(cmd_buf));
			}

			snprintf(cmd_buf, sizeof(cmd_buf), "@v/%d/%d*", angles.angle_x, angles.angle_y);
			socket_put_data(cmd_buf);
			if (serial.fd >= 0)
			{
				serial_write(&serial, (const uint8_t *)cmd_buf, strlen(cmd_buf));
			}
		}

		if (!manual_control && route_mode && current_point_idx < points_count && position_valid)
		{
			float target_lat = trajectory_points[current_point_idx * 2];
			float target_lon = trajectory_points[current_point_idx * 2 + 1];

			float heading_current = 0.0f;
			if (qmc5883p_is_data_ready(&qmc_sensor) && !qmc5883p_is_overflow(&qmc_sensor))
			{
				float xg, yg, zg;
				if (qmc5883p_read_calibrated_data(&qmc_sensor, &xg, &yg, &zg))
				{
					heading_current = qmc5883p_calculate_heading(xg, yg, 0.0f);
				}
			}

			float target_heading = calculate_heading_angle(current_latitude, current_longitude, target_lat, target_lon);
			float angle_error = target_heading - heading_current;
			if (angle_error < -180.0f)
				angle_error += 360.0f;
			else if (angle_error > 180.0f)
				angle_error -= 360.0f;

			float correction_speed = pid_controller_output_speed(&pid, angle_error, 30.0f);
			int16_t speed = (int16_t)correction_speed;

			motor_speed_t motors = {
				.v_fl = speed,
				.v_fr = -speed,
				.v_bl = speed,
				.v_br = -speed
			};

			char cmd_buf[128];
			snprintf(cmd_buf, sizeof(cmd_buf), "@m/%d/%d/%d/%d*", motors.v_fl, motors.v_fr, motors.v_bl, motors.v_br);
			socket_put_data(cmd_buf);
			if (serial.fd >= 0)
				serial_write(&serial, (const uint8_t *)cmd_buf, strlen(cmd_buf));

			float distance_to_target = haversine_distance(current_latitude, current_longitude, target_lat, target_lon);
			if (distance_to_target < 0.5f || fabsf(angle_error) < 5.0f)
			{
				current_point_idx++;
				if (current_point_idx >= points_count)
				{
					route_mode = false;
					points_count = 0;
				}
			}
		}

		if (keep_running)
		{
			struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000L}; // 1 ms
			nanosleep(&ts, NULL);
		}
	}

	if (js_fd >= 0)
		xbox_close(js_fd);
	socket_comm_stop();
	if (serial.fd >= 0)
		serial_close(&serial);

	return EXIT_SUCCESS;
}
