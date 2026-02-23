# socket 处理部分
import	threading
import	queue
import	socket
import	time
import	serial
import	pi_config
from	QMC5883P_get_angle import QMC5883P
import	timeout_decorator
import  sys

#import	data_handler

to_client_socket	= None
server_socket		= None
to_client_ip_address	= 0
GUI_receive_status	= 0
connection_status	= 0	# 1代表连接成功

# socket连接线程
def socket_connect_thread(HOST, PORT):
	global to_client_socket, server_socket, to_client_ip_address, connection_status

	# 先清理旧资源
	cleanup_socket()

	try:
		# AF_INET-> IPV4, SOCK_STREAM-> 基于TCP
		server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		# 允许复用地址（TIME_WAIT）
		server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		try:
			server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
		except (AttributeError, OSError):
			pass 

		server_socket.bind((HOST, PORT))
		server_socket.listen(1)
		print(f"[✓] Server listening on {HOST}:{PORT}")

		# 阻塞等待客户端连接
		to_client_socket, to_client_ip_address = server_socket.accept()
		connection_status = 1
		print(f"[✓] Client connected: {to_client_ip_address}")

	except Exception as e:
		print(f"[✗] socket_connect_thread failed: {e}")
		cleanup_socket()

# 清理socket残留
def cleanup_socket():
	global to_client_socket, server_socket, connection_status
	if to_client_socket:
		try:
			to_client_socket.close()
		except:
			pass
		to_client_socket = None
	if server_socket:
		try:
			server_socket.close()
		except:
			pass
	server_socket = None
	connection_status = 0

# socket 发送线程
def socket_send_thread():
	global connection_status
	while True:
		try:
			if connection_status == 1 and to_client_socket:
				data = latest_data.get_latest_data()
				if data:
					to_client_socket.send(data.encode('utf-8'))
					print(f"[→] Sent: {data[:50]}...", end='\r')
				else:
					# 未连接时休眠，不主动重连（由监听线程负责）
					time.sleep(0.5)
		except (ConnectionResetError, BrokenPipeError) as e:
			print(f"[!] Connection lost: {e}")
			connection_status = 0
			cleanup_socket()  # 关闭悬空 socket
		except Exception as e:
			print(f"[✗] Send error: {e}")
			connection_status = 0
			cleanup_socket()
		time.sleep(0.01)  # 控制发送频率

# socket 接收线程
def socket_receive_thread():
	global received_data, GUI_receive_status, connection_status
	while True:
		if connection_status == 1 and to_client_socket:
			try:
				to_client_socket.settimeout(0.5)	# 非永久阻塞
				raw = to_client_socket.recv(1024)	# 设置最大接收字节，不代表一次返回的字节数
				if not raw:  
					print("[!] Client closed connection")
					connection_status = 0
					cleanup_socket()
					continue
				received_data		= raw.decode('utf-8', errors='ignore')
				GUI_receive_status	= 1
			except socket.timeout:
				continue
			except (ConnectionResetError, OSError):
				print("[!] Receive connection broken")
				connection_status = 0
				cleanup_socket()
		else:
			time.sleep(0.5)  # 未连接时降频

# 与下位机通信线程
class raspberry:
	# mcu初始化，包括串口...
	def __init__(self, serial_path, serial_baud_rate, time_out_time):
		try:
			self.serial	= serial.Serial(serial_path, serial_baud_rate, timeout=time_out_time)
			if self.serial.isOpen():
				print("[√] Open the serial successfully")
				self.serial.write("[√] Open the serial successfully".encode("utf-8"))
			else:
				sys.exit(1)
		except Exception as e:
			print(f"[×] Failed to open the serial:{e}")
	# 需要注意这里数据的类型为字节，传入时应该encode(UTF-8)
	def serial_write(self, data_bytes):
		self.serial.write(data_bytes)
	# 读取数据。参数为数据大小（uint: size)
	def serial_read(self, data_size):
		if data_size <= 0:
			print("[×] Invalid data_size for serial-read")
		self.receive_data	= self.serial.read(data_size)
	# 读取一行数据。超时时间与串口初始化相同
	def serial_readline(self):
		self.receive_data	= self.serial.readline()
	# 判断是否开启串口
	def serial_is_open(self):
		self.serial.is_open

	# 关闭串口
	def close_serial(self):
		self.serial.close()
		
# ======================= QMC5883P传感器相关 ===================
x_raw		= 0
y_raw		= 0
z_raw		= 0
x_guass		= 0
y_gauss		= 0
z_gauss		= 0
direction	= 0
heading		= 0
qmc5883p_status	= 0	# 1代表数据已准备

def QMC5883P_get_angle_thread():
	global x_raw, y_raw, z_raw, x_guass, y_gauss, z_gauss, heading, direction

	# 创建QMC5883P实例
	sensor	= QMC5883P(bus_num=pi_config.I2C_BUS_ID)
	# 初始化传感器
	if not sensor.initialize():
		print("[x] QMC5883P: Failed to initialize the sensor, exiting the program...")
		return
	print("\n开始读取数据")

	try:
		while True:
			if sensor.is_data_ready():			
				if sensor.is_overflow():
					print("[!] QMC5883P: data is overflow")
					sensor.read_status()
					continue
	
				# 读取原始数据
				x_raw, y_raw, z_raw	 = sensor.read_raw_data()
	
				# 校准数据
				x_gauss, y_gauss, z_gauss	   = sensor.read_calibrated_data()

				# 计算航向角
				heading		= sensor.calculate_heading(x_gauss, y_gauss)
				direction	= sensor.get_direction(heading)
				# what is need to be noticed is heading's valiable type
				qmc5883p_data.put_data(heading)					
				# 设置接收状态
				qmc5883p_status	= 1
				#print(f"x:{x_guass},y:{y_gauss},z:{z_gauss}",end='\r')
				print(f"x:{x_raw},y:{y_raw},z:{z_raw}, 航向:{heading:6.1f}", end='\r')
				time.sleep(0.5)
	except	KeyboardInterrupt:
		print("[!] QMC5883P: Process interupting by user")
		qmc5883p_status	= 0
	finally:
		qmc5883p_status	= 0
		sensor.close()
	return

# MCU 通讯线程



# 通讯
'''
class communicator(raspberry):
	def send_query(query_object, query_information):
		# 检查数据类型
		if isinstance(query_information, String) == False:
			print("[x] Query information valiable must be 'String' type")

		# 下位机使用串口
		if query_object == 'MCU':
			#respberry	= respberry(pi_config.SERIAL_PATH, pi_config.BAUD_RATE, 0)
			raspberry.serial_write(query_information)
		# 客户端使用Socket
		elif query_object == 'Client':
			latest_data.put_data(query_information)
		else:
			  print("[x] Invalid object type")
			  return None
	# 接收响应
	def receive_response(query_object, query_information):
		
		# 检查数据类型
		if isinstance(query_information, String) == False:
			print("[x] Query information valiable must be 'String' type")
		
'''

navigation_data_parsing_status    = 0
received_serial_status	= 0
# [debug] 树莓派和MCU通信线程
# 主要作用是将数据放入待处理数据队列中, 以供解析线程进行解析
def pi_mcu_communication_thread(raspberry_serial):
	global received_serial_status
	received_data	= ""
	# received_serial_status	= 0
	print("[√] Open MCU received thread successfully.")
	# 轮询获取串口缓冲区数据
	while True:
		# method readline will wait until serial buffer read the '\n'.
        # So the command must include '\n' in the mcu program.
		received_data	= raspberry_serial.serial_readline()
		if received_data != None:
			# 置位接收状态
            #received_serial_status  = 1
            #mcu_serial_received_data.put_data(received_data)
            mcu_response_data_with_parsing.put_data(received_data)
		else:
            #received_serial_status  = 0
            pass

# 解析mcu数据线程(主程序中还没开启)
def parse_mcu_data_thread(raspberry_serial):
    global  navigation_data_parsing_status
    while True:
        parsing_data    = mcu_response_data_with_parsing.get_latest_data()
        if parsing_data!=None:

            parsing_data    = data_handler.parse_mcu_response(parsing_data)
            if isinstance(parsing_data, dict) == True:
                mcu_GPS_data.put_data(parsing_data)
                # TODO:后续进行读取逻辑,可使用状态机
            elif isinstance(parsing_data, str) == True:
                # 导航数据解析完毕,状态置1, Nooooote:使用后需要置0
                navigation_data_parsing_status  = 1
                mcu_navigation_response_data.put_data(parsing_data)
# =========================== 导航部分 =========================
# 请求
QUERY_navigation_START		= "@n/A*"
QUERY_navigation_STOP		= "@n/Z*"
QUERY_navigation_POSITION	= "@n/G*"
QUERY_navigation_MOVING_HEADER	= "@n/T*"


# [debug]使用超时判断响应是否成功接收
# 由于多线程场景，超时解释器不适用signals信号(signals信号为LinuxAPI)
# 使用该函数的前提：开启与MCU的通讯线程
@timeout_decorator.timeout(3, use_signals=False)
def is_valid_response(target_response_header):
	# response_status	= 0
    # global received_serial_status
    global navigation_data_parsing_status
	while True:
		# received_serial_status 为共享全局变量
		if navigation_data_parsing_status == 1:
			response_information	= mcu_navigation_response_data.get_latest_data
			if response_information[3] != target_response_header:	
				print("[x] Invalid mcu response")
				#thread_stop_status	= 1
				response_information    = None
                return True
            navigation_data_parsing_status  = 0

            
def timeout_is_valid_response(target_response_header):
	try:
		return is_valid_response(target_response_header)
	except timeout_decorator.TimeoutError:
		return False

# get the heading degree between A and B

# the argument is the type "list<float>"，which is mean that points list
# It still needs to send logs to the client
def navigation_thread(raspberry_serial, coordinates_list, points_number):
	# global	heading, direction	# QMC5883P 测量的航向角
	global		received_serial_status
	
	remaining_path_points	= points_number
	thread_stop_status	= 0
	response_data		= ""
		
	# Send the navigation start query -> MCU
	raspberry_serial.serial_write(QUERY_navigation_START.encode("utf-8"))
	# Nooooote: 这里需要根据是否超时来判断数据，因此获取数据是在超时函数中
	# response_data	= mcu_serial_received_data.get_latest_data
	if timeout_is_valid_response('0') == False:
		print("[x] Response 0 is timeout")
		thread_stop_status	= 1		
		return	
	while remaining_path_points != 0:
		# Send the navigation position query -> MCU
		raspberry_serial.serial_write(QUERY_navigation_POSITION.encode("utf-8"))
		# response_data	= mcu_serial_received_data.get_latest_data
        # TODO 需要更改逻辑:新版本的位置请求是一直发送的.因此只需要通过接收定位线程获取数据即可

		if timeout_is_valid_response('2') == False:
			print("[x] Response is timeout")
			thread_stop_status	= 1
		# Parse the position data
		information_list	= response_data[:-1].split('/')
	
		
		current_latitude	= float(information_list[2])
		current_longitude	= float(information_list[3])


		if points_number<remaining_path_points:
			print("[x] invalid points_number")
			thread_stop_status	= 1
		target_latiitude		= float(coordinates_list[2*(points_number-remaining_path_points-1)])
		target_longitude		= float(coordinates_list[2*(points_number-remaining_path_points-1)])
		# Calculate the car's next move
		current_heading_degree	= qmc5883p_data.get_latest_data	
		target_heading_degree	= data_handler.calculate_heading_angle(current_latitude, current_longitude, target_latiitude, target_longitude)
		'''
		PID占位
		'''
		move_data       = f""
		# 假设已经有运动数据了
		# Send the move query -> MCU
		raspberry_serial.serial_write((QUERY_navigation_MOVING_HEADER+move_data).encode("utf-8"))
		# response_data	= mcu_serial_received_data.get_latest_data
		if timeout_is_valid_response('3') == False:
			print("[x] invalid points number")
		
		
		# PID-debug
		if thread_stop_status == 1:
			print("[!] Thread has stopped")
			break
		
		remaining_path_points   = remaining_path_points-1
	# Send the nevigaton stop query -> MCU
	raspberry_serial.serial_write((QUERY_navigation_STOP).encode("utf-8"))
	if timeout_is_valid_response('1') == False:
		print("[x] Don't respond to the navigation stop signal")
		thread_stop_status	= 1
	# Stop the thread

# 创建数据处理类（put和get都需要锁）
class data_handler():
	def __init__(self):
		self.lock		= threading.Lock()
		self.data		= None
		self.data_status	= threading.Event()
		
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

# debug
# 创造数据处理实例
latest_data			= data_handler()

mcu_response_data_with_parsing  = data_handler()
# TODO 将 mcu_serial_received_data 替换为 mcu_navigation_response_data
mcu_navigation_response_data    = data_handler()
mcu_serial_received_data	= data_handler()
qmc5883p_data			= data_handler()
mcu_GPS_data			= data_handler()

# 测试用例
if __name__ == "__main__":
        # 测试导航线程
        coordinates_list        = ['116.3974673500868', '39.90873966065374', '116.3974673500868', '39.90873966065374']
        points_number           = len(coordinates_list)
        zero2w  = raspberry(pi_config.SERIAL_PATH, pi_config.BAUD_RATE, 1)
        navigation_single_thread        = threading.thread(target=navigation_thread, args=(zero2w, coordinates_list, points_number))

	
