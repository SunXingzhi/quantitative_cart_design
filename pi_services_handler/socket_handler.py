# socket 处理部分
import	threading
import	queue
import	socket
import	time
import	serial
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
class mcu:
	# mcu初始化，包括串口...
	def __init__(self, serial_path, serial_baud_rate, time_out_time):
		try:
			self.serial	= serial.Serial(serial_path, serial_baud_rate, timeout=time_out_time)
			if mcu_serial.isOpen():
				print("[√] Open the serial successfully")
				mcu_serial.write("[√] Open the serial successfully")
			else:
				sys.exit(1)
		except Exception as e:
			print(f"[×] Failed to open the serial:{e}")


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
latest_data	 = data_handler()
