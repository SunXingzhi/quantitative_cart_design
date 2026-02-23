import math
import time
from QMC5883P_get_angle import QMC5883P

class PID:
        def __init__(self,kp,ki,kd,output_limit=100):
                self.kp                 = kp
                self.ki                 = ki
                self.kd                 = kd
                self.output_limit       = output_limit
                self.last_time          = time.time()
                self.last_error         = 0.0
                self.integral           = 0.0

        # update PID paremeters
        def update_pid(self,error):
                now     = time.time()
                dt      = now-self.last_time
                print(error,self.last_error)
                if dt<=0:
                        print(f"time data error")
                        return 0.0
                
                self.integral   += error*dt
                de_dt           = (error-self.last_error)/dt
                
                output  = self.kp*error+self.ki*self.integral+self.kd*de_dt
                output  = max(-self.output_limit,min(self.output_limit,output))

                self.last_error = error
                self.last_time  = now
                return output
        
def clamp(v,vmin,vmax):
                return max(vmin,min(v,vmax))

def output_motor_speed(angel_error,base_speed=30):
        """
        angel_error:    目标方向 - 当前方向 (单位: 度, -180 ~ 180)
        base_speed:     这里是基于最大速度的占比(30%)
        return:         字符串命令 m:/lf/rf/lb/rb
        """
        correction      = pid.update_pid(angel_error)

        left_speed      = base_speed - correction
        right_speed     = base_speed + correction

        left_speed      = clamp(left_speed, -100, 100)
        right_speed     = clamp(right_speed, -100, 100)

        # lf,lb,rf,rb这里都是百分数(without '%')
        lf = int(left_speed)
        lb = int(left_speed)
        rf = int(right_speed)
        rb = int(right_speed)

        return f"m:/{lf}/{rf}/{lb}/{rb}"

# pid setting   
pid = PID(
    kp=1.2,
    ki=0.0,
    kd=0.2,
    output_limit=70
)

# function main
def main():
	sensor		= QMC5883P(bus_num=1)
	target_angel	= float(input("target_angel:"))
	
	if not sensor.initialize():
		print("sensor initialized failed in main")
		return

	try:
		while True:
			if sensor.is_data_ready():
 				# 检查是否溢出
				if sensor.is_overflow():
					print("[×] Data is overflow")
                        		# 读取状态寄存器来清除溢出标志
					sensor.read_status()
					continue
				x_gauss, y_gauss, z_gauss       = sensor.read_calibrated_data()
				angel_error                     = sensor.calculate_heading(x_gauss, y_gauss)
				cmd                             = output_motor_speed(angel_error)
				print(cmd)
			time.sleep(0.3)
	except KeyboardInterrupt:
		print('/r'"-program closed-")
	finally:
                sensor.close()

if __name__ == "__main__":
	main()