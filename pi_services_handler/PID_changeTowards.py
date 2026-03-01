import math
import time
from QMC5883P_get_angle import QMC5883P

class PID:
        def __init__(self,kp,ki,kd,angle0=None,output_limit=100):
                self.kp                 = kp
                self.ki                 = ki
                self.kd                 = kd
                self.output_limit       = output_limit
                self.angle0             = angle0
                self.last_error         = 0.0
                self.integral           = 0.0

        # update PID paremeters
        # return: output--输出的角度大小
        def update_pid(self,error):
                self.integral   += error
                d_d             = error - self.last_error
                
                output  = self.kp*error+self.ki*self.integral+self.kd*d_d
                output  = max(-self.output_limit,min(self.output_limit,output))

                self.last_error = error
                return output
        
        def clamp(self,v,vmin,vmax):
                return max(vmin,min(v,vmax))

        # Noooote :需要注意左右两侧轮胎的转向应该是相反的
        # angle0 > 0代表need逆时针转动
        def output_motor_speed(self,angle_error,base_speed=30):
                """
                angle_error:    目标方向 - 当前方向 (单位: 度, -180 ~ 180)
                base_speed:     这里是基于最大速度的占比(30%)
                return:         字符串命令 m:/lf/rf/lb/rb
                """
                correction      = base_speed*self.update_pid(angle_error)/self.angle0

                left_speed      = correction
                right_speed     = correction

                left_speed      = self.clamp(left_speed,-100,100)
                right_speed     = self.clamp(right_speed,-100,100)

                # lf,lb,rf,rb这里都是百分数(without '%')
                lf = int(left_speed)
                lb = int(left_speed)
                rf = int(right_speed)
                rb = int(right_speed)
                
                return f"m:/{lf}/{rf}/{lb}/{rb}"

# pid setting
pid = PID(
    kp=0.5,
    ki=0.1,
    kd=0.2,
    angle0=None,
    output_limit=70
)

# function main
def main():
    sensor          = QMC5883P(bus_num=1)
    target_angle    = float(input("target_angle:"))
    # 0<target_angle<360,指向正北为0，顺时针增加
	
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

            x_gauss, y_gauss, z_gauss   = sensor.read_calibrated_data()
            angle_error                 = sensor.calculate_heading(x_gauss, y_gauss) - target_angle
            if angle_error<-180:
                angle_error += 360
            elif angle_error>180:
                angle_error -= 360

            if pid.angle0 is None:
                pid.angle0 = angle_error
                print(f"{pid.angle0:6.1f}")

            cmd = pid.output_motor_speed(angle_error)
            print(cmd)
            time.sleep(0.3)
    except KeyboardInterrupt:
        # 恢复初始状态
        pid.angle0 = None
        print('/r'"-program closed-")
    finally:
        sensor.close()

if __name__ == "__main__":
        main()
