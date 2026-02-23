import time
import math
import smbus2

class QMC5883P:
        # I2C 地址
        I2C_ADDR = 0x2C  # 默认地址 2CH
        
        # 寄存器地址
        REG_CHIP_ID   = 0x00
        REG_DATA_X_L  = 0x01
        REG_DATA_X_H  = 0x02
        REG_DATA_Y_L  = 0x03
        REG_DATA_Y_H  = 0x04
        REG_DATA_Z_L  = 0x05
        REG_DATA_Z_H  = 0x06
        REG_STATUS    = 0x09
        REG_CTRL_1    = 0x0A
        REG_CTRL_2    = 0x0B
        
        # 状态位
        STATUS_DRDY = 0x01  # 数据就绪
        STATUS_OVFL = 0x02  # 数据溢出

        def __init__(self, bus_num=1):
            """
            初始化 QMC5883P 传感器
            bus_num: I2C 总线号 (树莓派通常是 1)
            """
            self.bus = smbus2.SMBus(bus_num)
            self.initialized = False
            
        def initialize(self):
            """
	        初始化传感器
	        """
            try:
                # 验证芯片ID
                chip_id = self.bus.read_byte_data(self.I2C_ADDR, self.REG_CHIP_ID)
                if chip_id != 0x80:
                    print(f"[!]: Chip id doesn't matched, Expected:0x80, but 0x{chip_id:02X}")
                    return False
                
                print("[√] QMC5883P: ID detected successfully")
                
                # 软复位
                self.soft_reset()
                time.sleep(0.1)  # 等待复位完成
                
                # 配置控制寄存器2
                # BIT[1:0] = 00: Set and Reset On (推荐)
                # BIT[3:2] = 10: 量程 ±8Gauss
                # BIT[5] = 0: 自测试关闭
                # BIT[7] = 0: 不复位
                ctrl2_value =  0x10
                self.bus.write_byte_data(self.I2C_ADDR, self.REG_CTRL_2, ctrl2_value)
                
                # 配置控制寄存器1 - 连续模式
                # BIT[1:0] = 11: 连续模式
                # BIT[3:2] = 11: 输出数据速率 200Hz
                # BIT[5:4] = 10: 过采样率 OSR1 = 128
                # BIT[7:6] = 00: 降采样率 OSR2 = 1
                ctrl1_value = 0b00101111  # 0x2F
                self.bus.write_byte_data(self.I2C_ADDR, self.REG_CTRL_1, ctrl1_value)
                
                print("[√] QMC5883 init successfully")
                print("==========================================")
                print("|MODE:                |Continuous mode   |")
                print("|Range:               |±8 Gauss          |")
                print("|Data rate:           |200 Hz            |")
                print("|OverSampling rate:   |128               |")
                print("==========================================")
                self.initialized = True
                return True
                
            except Exception as e:
                print(f"[×] QMC5883P: Failed to init: {e}")
                return False
        
        def soft_reset(self):
            """
	        执行软复位
	        """
            try:
                # BIT[7] = 1: 触发软复位
                self.bus.write_byte_data(self.I2C_ADDR, self.REG_CTRL_2, 0x80)
                print("[√] Reset completed by software")
            except Exception as e:
                print(f"[×]: Failed to reset the sensor: {e}")
        
        def read_status(self):
            """
	        读取状态寄存器
	        """
            try:
                status = self.bus.read_byte_data(self.I2C_ADDR, self.REG_STATUS)
                return status
            except Exception as e:
                print(f"[×] Failed to read the sensor status: {e}")
                return 0
        
        def is_data_ready(self):
            """
	        检查数据是否就绪
	        """
            status = self.read_status()
            return (status & self.STATUS_DRDY) != 0
        
        def is_overflow(self):
            """
	        检查是否数据溢出
	        """
            status = self.read_status()
            return (status & self.STATUS_OVFL) != 0
        
        def read_raw_data(self):
            """
            读取原始的磁力数据
            """
            if not self.initialized:
                print("[×] Sensor havn't init")
                return (0, 0, 0)
            
            try:
                # 连续读取6个数据寄存器
                data = self.bus.read_i2c_block_data(self.I2C_ADDR, self.REG_DATA_X_L, 6)
                
                # 组合16位有符号整数
                x = self._combine_bytes(data[1], data[0])  # X_MSB, X_LSB
                y = self._combine_bytes(data[3], data[2])  # Y_MSB, Y_LSB  
                z = self._combine_bytes(data[5], data[4])  # Z_MSB, Z_LSB
                
                return (x, y, z)
                
            except Exception as e:
                print(f"[×]: Failed to read the sensor data: {e}")
                return (0, 0, 0)
        
        def read_calibrated_data(self):
            """
            读取校准后的磁力数据 (转换为高斯)
	        """
            x, y, z = self.read_raw_data()
            
            # 转换为高斯 (灵敏度为 3750 LSB/G 对于 ±8G 量程)
            sensitivity = 3750.0
            x_gauss = x / sensitivity
            y_gauss = y / sensitivity  
            z_gauss = z / sensitivity
            
            return (x_gauss, y_gauss, z_gauss)
        
        def calculate_heading(self, x, y, declination_angle=0):
            """
            计算航向角 (适用于水平放置)
            x			        X轴磁力值 (高斯)
            y			        Y轴磁力值 (高斯)
            declination_angle	磁偏角 (度), 正东偏转为正
	        degree			    航向角 (度, 0=北, 90=东, 180=南, 270=西)
            """
            # 计算航向角 (弧度)
            heading_rad = math.atan2(y, x)
            
            # 转换为度
            heading_deg = math.degrees(heading_rad)
            
            # 调整到 0-360 度范围
            if heading_deg < 0:
                heading_deg += 360
            if heading_deg > 360:
                heading_deg -= 360
            
            # 应用磁偏角校正
            heading_deg += declination_angle
            
            # 再次标准化到 0-360 度范围
            heading_deg %= 360
            
            return heading_deg
        
        def _combine_bytes(self, msb, lsb):
            """
	        将两个字节组合为16位有符号整数
	        """
            value = (msb << 8) | lsb
            # 处理有符号数 (2的补码)
            if value & 0x8000:
                value = value - 0x10000
            return value
        
        def close(self):
            """关闭I2C连接"""
            self.bus.close()
            print("[√] the I2C connection is closed")

        # def correct_deg(self,deg)

        def get_direction(self,deg):
            """
	        通过degree得到大致方向
            """
            
            Directions	= [(0,22.5,"N"),(22.5,67.5,"NE"),(67.5,112.5,"E"),(112.5,157.5,"SE"),
		         (157.5,202.5,"S"),(202.5,247.5,"SW"),(247.5,292.5,"W"),(292.5,337.5,"NW"),(337.5,360,"N")]
        
            for (min_deg,max_deg,direction) in Directions:
                if (min_deg<max_deg and (deg>=min_deg and max_deg>deg)):
                    return direction
            return "无法获得方向"

def main():
        print("QMC5883P 连续模式测试")
        print("=" * 40)
        
        # 创建传感器实例
        sensor = QMC5883P(bus_num=1)  # 树莓派I2C bus = 1
        
        # 初始化传感器
        if not sensor.initialize():
            print("Failed to initialize the sensor, the program is exiting...")
            return
        
        print("\nStarting to read data (press Ctrl C to stop)...")
        print("-" * 40)
        
        try:
            while True:
                # 检查数据是否就绪
                if sensor.is_data_ready():
                    # 检查是否溢出
                    if sensor.is_overflow():
                        print("[×] Data is overflow")
                        # 读取状态寄存器来清除溢出标志
                        sensor.read_status()
                        continue
                    
                    # 读取原始数据
                    x_raw, y_raw, z_raw = sensor.read_raw_data()
                    
                    # 读取校准数据 (高斯)
                    x_gauss, y_gauss, z_gauss = sensor.read_calibrated_data()
                    
                    # 计算航向角 (假设水平放置)
                    heading = sensor.calculate_heading(x_gauss, y_gauss)
		
                    direction = sensor.get_direction(heading)
                    
                    # 显示结果
                    print(f"原始数据: X={x_raw:6d}, Y={y_raw:6d}, Z={z_raw:6d}")
                    print(f"磁场强度: X={x_gauss:7.3f}G, Y={y_gauss:7.3f}G, Z={z_gauss:7.3f}G")
                    print(f"航向角: {heading:6.1f}°    {direction}")
                    print("-" * 40)
                
                # 短暂延迟
                time.sleep(0.5)
                
        except KeyboardInterrupt:
            print("[!] User Interrupt")
        finally:
            sensor.close()


if __name__ == "__main__":
        main()
