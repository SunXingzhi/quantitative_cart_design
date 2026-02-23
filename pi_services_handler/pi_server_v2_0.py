import	time
import	serial
import	pi_config
import	threading
import	thread_handler				# socket通信处理
import	data_handler				# 数据命令处理
import	car_motion_handler
import	joystick_handler
import	sys
import	ctypes

from car_motion_handler import car_status
# 程序启动标志

print(''' \033[36m					
░████  ░████████████████████████████████████  ░████	 ░████
 ░██  ░██				 ░██   ░██	   
  ░██░██  ░██   ░█████     ░██████      ░██    ░████████  ░██
   ░███   ░██ ░██    ░██ ░██    ░██    ░███    ░██    ░██ ░██
  ░██░██  ░██ ░██    ░██ ░██    ░██   ░██      ░██    ░██ ░██
 ░██  ░██ ░██ ░██    ░██ ░██   ░███  ░██       ░██    ░██ ░██
░████  ░█████████   ░████  ░████░██ ░██████████████   ░███████
				░██	       ░██	  
  ░███████████████████████████████	     ░██████	   
\033[0m							     
''')

# ======================网络socket创建及多线程===================
# global server data buffer
data_buffer				= []
thread_handler.connection_status	= 0	# 1代表连接成功
thread_handler.to_client_socket		= None
thread_handler.server_socket		= None  # 新增：服务器socket全局变量
thread_handler.to_client_ip_address	= 0
send_status		= 0	# 1代表可发送
GUI_receive_status	= 0	# 1代表可接收
received_data		= ''

# 创建多线程
GUI_communication_thread	= threading.Thread(
	target=thread_handler.socket_connect_thread,
	args=(pi_config.SOCKET_CLIENT_ADDRESS, pi_config.SOCKET_PORT),
	name="socket_listener"
	)
GUI_send_thread			= threading.Thread(target=thread_handler.socket_send_thread,	 name="socket_sender")
GUI_receive_thread		= threading.Thread(target=thread_handler.socket_receive_thread, name="socket_receiver")
QMC5883P_thread			= threading.Thread(target=thread_handler.QMC5883P_get_angle_thread, name="QMC5883P")
# 默认启动线程（保持顺序无关）
GUI_communication_thread.start()
GUI_receive_thread.start()
GUI_send_thread.start()
QMC5883P_thread.start()

# ======================= 主程序 =======================
if __name__ == "__main__":
	# 初始化串口
	# try:
	# 	# 使用非堵塞模式，来接收来自MCU的串口数据
	# 	mcu_serial	= serial.Serial(pi_config.SERIAL_PATH, pi_config.BAUD_RATE, timeout=0)
	# 	# input_serial	= serial.Serial(pi_config.SERIAL_PATH, pi_config.BAUD_RATE, timeout=1)
	# 	if mcu_serial.isOpen():
	# 		print("串口打开成功")
	# 		mcu_serial.write(b"Serial open successfully\n")
	# 	else:
	# 		print("串口打开失败")
	# 		sys.exit(1)
	# except Exception as e:
	# 	print(f"串口初始化失败: {e}")
	# 	sys.exit(1)
	send_counter	= 0
	# 串口初始化：使用timeout = None, 代表接收数据时为堵塞模式（这里使用的seria.readline())
	zero2w	= thread_handler.raspberry(pi_config.SERIAL_PATH, pi_config.BAUD_RATE, pi_config.TIMEOUT_TIME)
	# 开启与MCU的串口通信线程（读数据）
	pi_receive_MCU_thread	= threading.Thread(target=thread_handler.pi_mcu_communication_thread, args=(zero2w,), name="pi_mcu_communicator")
	pi_receive_MCU_thread.start()
    print("[√] pi received_data thread open successfully\n")
    # 开启MCU数据解析线程
    pi_pasrsing_MCU_thread  = threading.Thread(target=thread_handler.parse_mcu_data_thread, args=(zero2w,), name="pi_MCU_data_parser")
    pi_pasrsing_MCU_thread.start()
    print("[√] pi parsing_data thread open successfully\n")

    # 打开手柄设备
	device_path = pi_config.DEVICE_PATH  # 转为 bytes
	fd = joystick_handler.xbox_open(device_path)

	# 堵塞连接，直到xbox成功连接(需要创建线程）
	while True:
		if fd < 0:
			print("无法打开手柄设备", end='\r')
			# 重新尝试连接
			fd = joystick_handler.xbox_open(device_path)
			time.sleep(1)  # 降低重试频率
		else:
			print("手柄设备打开成功")
			print("当前状态: 默认待机状态")
			break

	map_data = joystick_handler.XboxMap()  # 创建结构体实例

	try:
		print("[√] Begin to read the joystick data")
		while True:
				
			# 查询Client发送的数据(最高优先级)
			if thread_handler.GUI_receive_status == 1 and pi_receive_MCU_thread != None:
				# 执行接收数据和解析逻辑
				print(f"[←] Received: {thread_handler.received_data.strip()}")
				query_type	= data_handler.parse_client_command(thread_handler.received_data)
				if query_type == "points":
					coordinates_list	= (thread_handler.received_data[:-1]).split('/')
					coordinates_number	= len(coordinates_list)-1
					print(f"coordinates_number length:{coordinates_number}")
					if coordinates_number%2 != 0 or (coordinates_number/2)>10:
						print("[×] Invalid coordinates number")
						coordinates_list	= []	# clean the list
					points_number	= len(coordinates_list)/2
					# 还需要清理点	
				if query_type == "Nstart":
					# 检查当前系统是否有坐标点
					if points_number == 0:
						print("[×] Invalid coordinates number")
					# 开启导航线程
					navigation_single_thread	= threading.Thread(target=thread_handler.navigation_thread, args=(zero2w, coordinates_list, points_number), name="nevigation_thread_handler")
					navigation_single_thread.start()
				if query_type == "Pget":	# Client->GUI:当前小车位置请求
					#print("13424")
					continue	

				thread_handler.GUI_receive_status = 0  # 重置接收状态
			
			result	= joystick_handler.xbox_map_read(fd, ctypes.byref(map_data))
			#if result < 0:
			#	print("XBOX don't change", end='\r')
			#	time.sleep(0.01)
			#	continue
			# print(f"\t\tLY:{map_data.ly}")
			# 获取当前状态
			status = car_motion_handler.get_status(map_data)
			# 打小车状态-debug
			# print(f"\t\t\t\t\t\t当前小车状态：{status}\t", end='\r')
			# 根据状态修改数据权限
			map_data = car_motion_handler.get_motor_data_by_status(status, map_data)
			# 转换摇杆数据
			joystick_data = car_motion_handler.convert_joystick_data(map_data)
			#print(f"lt:{joystick_data.lt},rt:{joystick_data.rt}")
			# 解析电机速度
			motors_speed = car_motion_handler.parsed_motors_speed(joystick_data['v_cx'], joystick_data['omega'])
			# 解析舵机角度i
			motors_angle = car_motion_handler.parsed_motors_angle(joystick_data['lt'], joystick_data['rt'])
			# print(f"\t\t\t\t\t\lx:{joystick_data['lx']}", end='\r')
			# 构造串口协议字符串
			usart_motor_cmd = f"@m/{motors_speed['v_fronted_left']}/{motors_speed['v_fronted_right']}/{motors_speed['v_backed_left']}/{motors_speed['v_backed_right']}*"
			usart_servo_cmd = f"@v/{motors_angle['angle_X']}/{motors_angle['angle_Y']}*"
			usart_open_brake_cmd	= "@b/1*"
			usart_close_brake_cmd	= "@b/0*"

			# 发送到串口
			# 创建计数器（计数三次发送1次）
			send_counter	= send_counter+1
			#servo_send_counter	=
			#print(f"status:{status}", end='\r')
			if status==car_status.OPEN_BRAKE_STATUS.value:
				#zero2w.serial_write(usart_open_brake_cmd.encode("utf-8"))
				print(f"Already send {usart_open_brake_cmd}")
			if status==car_status.CLOSE_BRAKE_STATUS.value:
				#zero2w.serial_write(usart_close_brake_cmd.encode("utf-8"))
				status	= car_status.DEFAULT_STATUS.value
				print(f"send {usart_close_brake_cmd}")
			if send_counter == 4:
				zero2w.serial_write(usart_motor_cmd.encode("utf-8"))	
				print(f"\t\t\t\tmotor:{usart_motor_cmd},servo:{usart_servo_cmd}", end='\r')	# debug
			if send_counter == 5:
				zero2w.serial_write(usart_servo_cmd.encode("utf-8"))	
				send_counter	= 0
			
			
			# 读取串口数据
			# serial_received_data	= zero2w.serial_read(10)


			# 将数据发送给Client (使用共享变量）
			#GUI_communicator_information = usart_motor_cmd + usart_servo_cmd + '@n/116.3974673500868/39.90873966065374*'  # 添加定位信息
			GUI_communicator_information	= usart_motor_cmd
			if thread_handler.connection_status == 1:
				thread_handler.latest_data.put_data(GUI_communicator_information)
				#print("1")	
				# send_status = 1
			
			# # 控制循环频率 ≈ 50Hz
			time.sleep(0.01)



	except KeyboardInterrupt:
		print("\n用户中断，正在关闭设备...")

	except Exception as e:
		print(f"运行时错误: {e}")

	finally:
		# 安全关闭所有资源
		if 'mcu_serial' in locals() and zero2w.serial_is_open:
			zero2w.close_serial()
			print("串口已关闭")

		if 'fd' in locals() and fd >= 0:
			joystick_handler.xbox_close(fd)
			print("手柄设备已关闭")
			
		# 关闭socket
		if thread_handler.to_client_socket:
			try:
				thread_handler.to_client_socket.close()
			except:
				pass
				
		if thread_handler.server_socket:
			try:
				thread_handler.to_client_socket.close()
			except:
				pass
				
		# 退出程序
		sys.exit(0)
