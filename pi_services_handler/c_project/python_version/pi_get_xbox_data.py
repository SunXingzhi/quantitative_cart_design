import ctypes
import sys
import time
from enum import Enum
import serial
import pi_config
import socket
import queue
import threading

# ======================网络socket创建及多线程===================
# global server data buffer
data_buffer			 = []
connection_status	   = 0	# 1代表连接成功
to_client_socket		= None
server_socket		   = None  # 新增：服务器socket全局变量
to_client_ip_address	= '172.86.100.81'
send_status		= 0	# 1代表可发送
receive_status		= 0	# 1代表可接收
received_data		= ''

# socket连接线程(树莓派作为服务端)
def socket_connect_thread(HOST, PORT):
	global to_client_socket, server_socket, to_client_ip_address, connection_status
	
	# 如果已有服务器socket，先关闭
	if server_socket:
		try:
			server_socket.close()
		except:
			pass
	
	try:
		# INET-> IPV4, SOCK_STREAM-> TCP
		server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		# 允许端口重用, 避免重连时的地址被占用的问题
		server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		server_socket.bind((HOST, PORT))
		server_socket.listen(2)	# 监听，最大连接数设置为2
		print(f"Server listening on {HOST}:{PORT}")
		
		# 堵塞 等待接收（这里相当于只允许一个客户端进行连接）
		to_client_socket, to_client_ip_address = server_socket.accept()
		connection_status = 1	
		print(f"连接成功! ip:{to_client_ip_address}")	
		
	except Exception as e:	 
		print(f"socket连接失败: {e}")
		connection_status = 0
		if server_socket:
			try:
				server_socket.close()
			except:
				pass
		server_socket = None


# socket接收线程
def socket_receive_thread():
	global to_client_socket, received_data, receive_status, connection_status
	
	while True:
		receive_status = 0
		if connection_status == 1 and to_client_socket:	
			try:
				# 设置超时，避免一直阻塞
				to_client_socket.settimeout(1.0)
				received_data = to_client_socket.recv(1024)	# 最大单次接收缓冲区
				receive_status = 1
				print(f"接收到的数据为{received_data.decode('utf-8')}")
			except socket.timeout:
				continue
			except Exception as e:
				print(f"接收数据错误: {e}")
				connection_status = 0
				if to_client_socket:
					try:
						to_client_socket.close()
					except:
						pass
		else:
			print("接收线程：连接客户端失败！", end='\r')
			time.sleep(1)
				

# socket发送线程
def socket_send_thread(HOST, PORT):
	global to_client_socket, send_status, connection_status
	
	while True:
		try:
			if connection_status == 1 and to_client_socket:
				data = latest_data.get_latest_data()
				if data:
					to_client_socket.send(data.encode('utf-8'))  # 确保数据被编码
					# 使用end替换默认的\n结束符为\r, 便于查看调试
					print(f"发送到GUI的数据为{data}", end='\r')	
			elif connection_status != 1:
				print("暂未连接主机！尝试重连...", end='\r')
				# 重连时先关闭可能存在的客户端socket
				if to_client_socket:
					try:
						to_client_socket.close()
					except:
						pass
					# 需要设置为None，防止访问悬垂指针
					to_client_socket = None
				# 进行重连
				socket_connect_thread(HOST, PORT)
				# 重连后等待一会儿再继续
				time.sleep(1)
		except Exception as e:
			print(f"发送数据错误：{e}")
			# 清空连接状态
			connection_status = 0
			# 关闭客户端socket
			if to_client_socket:
				try:
					to_client_socket.close()
				except:
					pass
				to_client_socket = None
			# 等待一会儿再重连
			time.sleep(1)


# 创建数据处理类（put和get都需要锁）
class data_handler():
	def __init__(self):
		self.lock			  = threading.Lock()
		self.data			   = None
		self.data_status		= threading.Event()
		
	def put_data(self, data):
		with self.lock:
			self.data = data
		# 更新事件状态，代表有数据
		self.data_status.set()
		
	def get_latest_data(self):
		with self.lock:
			# 可以使用wait进行堵塞，直到获取到True
			if self.data_status.is_set():
				self.data_status.clear()
				return self.data
			else:
				return None

# 创造数据处理实例
latest_data	 = data_handler()

# 创建与GUI通讯线程
GUI_communication_thread	= threading.Thread(target=socket_connect_thread, args=('172.16.100.86', 65432))
GUI_send_thread			= threading.Thread(target=socket_send_thread, args=('172.16.100.86', 65432))
GUI_receive_thread		= threading.Thread(target=socket_receive_thread)

# 开启线程
GUI_communication_thread.start()
GUI_receive_thread.start()
GUI_send_thread.start()


# ======================= 小车状态枚举 =======================
class car_status(Enum):
	EXCEPCTION_ERROR_STATUS = -1	# 异常状态
	STOP_STATUS		= 0	# 停止状态
	DEFAULT_STATUS		= 1	# 默认待机状态
	CHANGE_POSITION_STATUS  = 2	# 调整喷水机构位置状态
	BRAKE_STATUS		= 3	# 刹车状态
	SPRAY_STATUS		= 4	# 喷水状态
	GPS_STATUS		= 5	# GPS定位状态
	NEVIGATION_STATUS	= 6	# 自动导航状态

# 当前小车状态寄存器
car_status_regiter = car_status.DEFAULT_STATUS.value

# 状态名称映射（用于日志）
STATUS_NAMES = {
	-1:	"异常状态",
	0:	"停止状态",
	1:	"默认待机状态",
	2:	"调整位置状态",
	3:	"刹车状态",
	4:	"喷水状态",
	5:	"GPS状态",
	6:	"导航状态"
}

# ======================= 获取当前状态 =======================
def get_status(xbox_data):
	global car_status_regiter
	is_brake	= xbox_data.a
	is_position = xbox_data.y
	is_canceled = xbox_data.b

	new_status = car_status_regiter  # 默认保持原状态

	if is_brake:
		new_status = car_status.BRAKE_STATUS.value
	elif is_position:
		new_status = car_status.CHANGE_POSITION_STATUS.value
	elif is_canceled:
		new_status = car_status.DEFAULT_STATUS.value

	# 仅当状态发生改变时更新并打印
	if new_status != car_status_regiter:
		old_name = STATUS_NAMES.get(car_status_regiter, "未知")
		new_name = STATUS_NAMES.get(new_status, "未知")
		print(f"🚦 状态切换: {old_name} → {new_name}")
		car_status_regiter = new_status

	return new_status

# ======================= 根据状态控制数据权限 =======================
def get_motor_data_by_status(car_status, xbox_data):
	match car_status:
		case -1:	# 异常状态：全置 -1
			for field_name, _ in xbox_data._fields_:
				setattr(xbox_data, field_name, -1)
			return xbox_data

		case 0:		# 停止状态：全置 0
			for field_name, _ in xbox_data._fields_:
				setattr(xbox_data, field_name, 0)
			return xbox_data

		case 1:		# 默认待机：只保留 lx, ly，其余清零
			for field_name, _ in xbox_data._fields_:
				if field_name not in ['lx', 'ly']:
					setattr(xbox_data, field_name, 0)
			return xbox_data

		case 3:		# 刹车状态：清零运动轴
			xbox_data.lx = 0
			xbox_data.ly = 0
			return xbox_data

		case '':	# 其他状态：保留原始数据（前提是已刷新！）
			return xbox_data

# ======================= 摇杆死区处理 =======================
def apply_deadzone(value, deadzone=5000):
	"""小于死区的值归零，避免漂移"""
	return 0 if abs(value) < deadzone else value

# ======================= 数据转换 =======================
def convert_joystick_data(xbox_data):
	# 应用死区
	lx = apply_deadzone(xbox_data.lx)
	ly = apply_deadzone(xbox_data.ly)
	rx = apply_deadzone(xbox_data.rx)
	ry = apply_deadzone(xbox_data.ry)

	# 转换比例
	lx = int(lx / pi_config.JOY_AXIS_MAX * pi_config.TURNING_RADIUS_MAX)
	ly = -int(ly / pi_config.JOY_AXIS_MAX * 100)  # Y轴反向（推前为正）
	rx = int(rx / pi_config.JOY_AXIS_MAX * pi_config.TURNING_RADIUS_MAX)
	ry = -int(ry / pi_config.JOY_AXIS_MAX * 100)

	return {
		'lx': lx,
		'ly': ly,
		'rx': rx,
		'ry': ry
	}

# ======================= 四轮差速模型 =======================
def parsed_motors_speed(r, center_speed):
	# 轮距参数（转换为米）
	front_back_dist = pi_config.FRONT_BACKED_DISTANCE / 1000.0
	left_right_dist = pi_config.LEFT_RIGHT_DISTANCE / 1000.0

	MAX_SPEED = 100

	# 初始化四个轮速
	v_fl = v_fr = v_bl = v_br = 0

	if r == 0 or abs(r) < 0.1:  # 原地转弯
		v_fl = v_bl = int(center_speed)
		v_fr = v_br = int(-center_speed)
	else:
		# 差速转弯
		v_left  = center_speed * (1 - left_right_dist / (2 * r))
		v_right = center_speed * (1 + left_right_dist / (2 * r))

		# 限幅
		v_left  = max(min(v_left, MAX_SPEED), -MAX_SPEED)
		v_right = max(min(v_right, MAX_SPEED), -MAX_SPEED)

		v_fl = v_bl = int(v_left)
		v_fr = v_br = int(v_right)

	return {
		'v_fronted_left':   v_fl,
		'v_fronted_right':  v_fr,
		'v_backed_left':	v_bl,
		'v_backed_right':   v_br
	}

# ======================= C结构体与函数绑定 =======================
from ctypes import c_int, Structure, POINTER, byref

# 加载动态库
try:
	xbox_lib = ctypes.CDLL(pi_config.JOY_STICK_SO_PATH)
except Exception as e:
	print(f"无法加载手柄库: {e}")
	sys.exit(1)

# 定义结构体
class XboxMap(Structure):
	_fields_ = [
		("time",	c_int),
		("a",		c_int),
		("b",		c_int),
		("x",		c_int),
		("y",		c_int),
		("lb",		c_int),
		("rb",		c_int),
		("select",	c_int),
		("start", 	c_int),
		("home",	c_int),
		("lo",		c_int),
		("ro",		c_int),
		("lx",		c_int),
		("ly",		c_int),
		("rx",		c_int),
		("ry",		c_int),
		("lt",		c_int),
		("rt",		c_int),
		("xx",		c_int),
		("yy",		c_int)
	]

# 绑定函数
xbox_open	= xbox_lib.xbox_open
xbox_map_read	= xbox_lib.xbox_map_read
xbox_close	= xbox_lib.xbox_close

xbox_open.argtypes	= [ctypes.c_char_p]
xbox_open.restype	= c_int

xbox_map_read.argtypes	= [c_int, POINTER(XboxMap)]
xbox_map_read.restype	= c_int

xbox_close.argtypes	= [c_int]
xbox_close.restype	= c_int

# ======================= 主程序 =======================
if __name__ == "__main__":
	# 初始化串口
	try:
		out_put_serial = serial.Serial(pi_config.SERIAL_PATH, pi_config.BAUD_RATE, timeout=1)
		if out_put_serial.isOpen():
			print("串口打开成功")
			out_put_serial.write(b"Serial open successfully\n")
		else:
			print("串口打开失败")
			sys.exit(1)
	except Exception as e:
		print(f"串口初始化失败: {e}")
		sys.exit(1)

	# 打开手柄设备
	device_path = pi_config.DEVICE_PATH  # 转为 bytes
	fd = xbox_open(device_path)

	# 堵塞连接，直到xbox成功连接
	while True:
		if fd < 0:
			print("无法打开手柄设备", end='\r')
			# 重新尝试连接
			fd = xbox_open(device_path)
			time.sleep(1)  # 降低重试频率
		else:
			print("手柄设备打开成功")
			print("当前状态: 默认待机状态")
			break

	map_data = XboxMap()  # 创建结构体实例

	try:
		print("开始读取手柄数据...")
		while True:
			
			# 查询Client发送的数据(最高优先级)
			if receive_status == 1:
				# 执行接收数据和解析逻辑
				print(f"received_data is:{received_data.decode('utf-8')}")
				receive_status = 0  # 重置接收状态
			
			# 读取手柄数据
			result = xbox_map_read(fd, byref(map_data))
			if result < 0:
				print("摇杆位置未改变", end='\r')
				time.sleep(0.01)
				continue
			
			# 获取当前状态
			print(f"result{result}")
			status = get_status(map_data)
			# 打印小车状态-debug
			print(f"当前小车状态为：{status}")
			# 根据状态修改数据权限
			map_data = get_motor_data_by_status(status, map_data)

			# 转换摇杆数据
			joystick_data = convert_joystick_data(map_data)

			# 解析电机速度
			motors_speed = parsed_motors_speed(joystick_data['lx'], joystick_data['ly'])

			# 构造串口协议字符串
			usart_cmd = f"@m/{motors_speed['v_fronted_left']}/{motors_speed['v_fronted_right']}/{motors_speed['v_backed_left']}/{motors_speed['v_backed_right']}*"
			
			# 发送到串口
			print(usart_cmd, end='\r')	# debug
			try:
				out_put_serial.write(usart_cmd.encode("utf-8"))
				
			except Exception as e:
				print(f"串口发送失败: {e}")

			# 将数据放到共享变量，便于与GUI通讯的线程获取数据
			GUI_communicator_information = usart_cmd + '@n/106.3974673500868/29.90873966065374*'  # 添加定位信息
			if connection_status == 1:
				latest_data.put_data(GUI_communicator_information)
				# send_status = 1
			
			# 控制循环频率 ≈ 50Hz
			time.sleep(0.01)

	except KeyboardInterrupt:
		print("\n用户中断，正在关闭设备...")

	except Exception as e:
		print(f"运行时错误: {e}")

	finally:
		# 安全关闭所有资源
		if 'out_put_serial' in locals() and out_put_serial.is_open:
			out_put_serial.close()
			print("串口已关闭")

		if 'fd' in locals() and fd >= 0:
			xbox_close(fd)
			print("手柄设备已关闭")
			
		# 关闭socket
		if to_client_socket:
			try:
				to_client_socket.close()
			except:
				pass
				
		if server_socket:
			try:
				server_socket.close()
			except:
				pass
				
		# 退出程序
		sys.exit(0)
