# 基本变量配置
DEVICE_PATH		= b"/dev/input/js0"	# 手柄默认路径
SERIAL_PATH		= '/dev/ttyAMA0'	# 串口默认路径
# BAUD_RATE		= 115200		# 设置串口波特率
BAUD_RATE		= 9600			# 设置串口波特率
TIMEOUT_TIME		= 1			# 串口读数据超时时间（单位：s)[N]:可能影响程序效率
FRONT_BACKED_DISTANCE	= 726			# 前后轮间距（单位：mm）
LEFT_RIGHT_DISTANCE	 = 560			# 左右轮间距（单位：mm）
JOY_AXIS_MAX		= 32767			# XBOX手柄摇杆的最大值
JOY_AXIS_MIN		= -32767		# XBOX手柄摇杆的最小值
JOY_STICK_SO_PATH	= './get_xbox_data.so'  # c动态共享库路径
TURNING_RADIUS_MAX	= 5			# 最大转弯半径（单位：m），可能会微调误触区间

SOCKET_SERVER_ADDRESS	= '172.16.100.113'
SOCKET_CLIENT_ADDRESS	= '0.0.0.0'		# 服务端监听地址（所有）
SOCKET_PORT		= 65432			# 监听端口
# 串口常用固定命令
USART_OPEN_BRAKE_CMD	= "@b/1*"
USART_CLOSE_BRAKE_CMD	= "@b/2*"
REVOLVE_MAX		= 180			# 最大角度
REVOLVE_MIN		= 0			# 云台最小角度
CAR_SPEED_MAX		= 1.5			# 小车最快速度（单位：m/s）
CAR_ANGULAR_SPEED_MAX	= 2.0			# 小车最大角速度（单位：rad/s）
IN_PLACE_LY_TURNING_THRESHOLD	= 1000		# 小车原地转弯阈值(以手柄原数据为准)
IN_PLACE_LX_TURNING_THRESHOLD	= 500		#
# 树莓派I2C部分
QMC5883P_I2C_ADDRESS	= 0x2c			# 磁力计模块默认地址
I2C_BUS_ID		= 1			# 树莓派I2C总线地址
# 寄存器地址
REG_CHIP_ID 		= 0x00			# 
REG_DATA_X_L		= 0x01			# 
REG_DATA_X_H		= 0x02			# 
REG_DATA_Y_L		= 0x03			# 
REG_DATA_Y_H		= 0x04			# 
REG_DATA_Z_L		= 0x05			# 
REG_DATA_Z_H		= 0x06			# 
REG_STATUS  		= 0x09			# 
REG_CTRL_1  		= 0x0A			# 
REG_CTRL_2  		= 0x0B			# 

PI			= 3.14159265358979323846
