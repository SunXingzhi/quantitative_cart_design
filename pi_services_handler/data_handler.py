import	pi_config
from	enum import Enum

	
# 状态存放区

#coordinates_list_status	= 0	# 0->没有坐标，1->有坐标
coordinates_list		= None
#nevigation_status	= 0	# 导航开始状态 0->未开始，1->开始
coordinates_get_status	= 0	# 获取坐标点请求状态

# [Internal]
class event_status(Enum):
	coordinates_list_event	= 0
	nevigation_event	= 1
	coordinates_get_event	= 2
	error_event		= 3

# 解析客户端命令（GUI）
# 返回命令类型
def parse_client_command(command_string):
	if len(command_string)==0:
		print("[×] empty string")
		return	None

	command_char	= command_string[1]

	end_char	= command_string[-1]
	if end_char != '*':
		print("[×] invalid command")
		return	None
	
	# 去除结束符
	#command_string	= command_string[:-1]
	# 匹配命令字符
	match command_char:
		# 导航命令，传入命令字符串，需要解析成？
		case "p":	
			return "points"
		case "q":
			query_type	= (command_string[:-1].split('/'))[1:]
			return query_type[0]



# 解析串口响应(MCU)
# 需要注意的是,每条命令除了结束符'*', 结尾还有'\n',方便配合(class)raspberry的serial_readline函数使用
# serial_readline 回一直读取串口数据直到遇到换行符为止,因此可以作为 一条 命令.
# 一共有
MCU_error_message_header	= "[x] parse MCU response: "
def parse_mcu_response(response_string):
	if len(response_string)==0 or response_string==None:
		print(f"{MCU_error_message_header}empty string")
		return None
	response_header    = response_string[1]
	
	end_char	= response_string[-2]
	if end_char!='*':
		print(f"{MCU_error_message_header}Invalid end char")
		return None
	
	# 删除后缀符号(\n')
	response_string = response_string[:-1]
	match response_header:
		case 'n':
            return response_string
		case 'g':	# 小车当前定位数据(#g/latitude/longitude*)
			# 提取经纬度信息
			latitude	= response_string.split('/')[1]
			longitude	= response_string.split('/')[-1]
			
			return	{
					'latitude': f"{latitude}",
					'longitude':    f"{longitude}"
				}	


from math import sin, cos, atan2
def calculate_heading_angle(a_latitude, a_longitude, b_latitude, b_longitude):
	# 经纬度转弧度
	a_latitude_radian	= a_latitude*pi_config.PI/180.0
	a_longitude_radian	= a_longitude*pi_config.PI/180.0
	b_latitude_radian	= b_latitude*pi_config.PI/180.0
	b_longitude_radian	= b_longitude*pi_config.PI/180.0

	longitude_radian_difference	= b_longitude_radian-a_longitude_radian

	x	= sin(longitude_radian_difference)*cos(b_latitude_radian)
	y	= cos(a_latitude_radian)*sin(b_latitude_radian)-sin(a_latitude_radian)*cos(b_latitude_radian)*cos(longitude_radian_difference)

	heading_degree	= atan2(x, y)*180.0/pi_config.PI

	if heading_degree<0:
		heading_degree	= heading_degree+360
	return heading_degree

	

# 处理请求函数
# 针对命令类型，做出对应响应
# 如果是坐标命令(@n/.../*)，命令中会自带坐标点集合。最多10个点

'''
def get_response(command_string):
	global	coordinates_list

	query	= parse_client_command(command_string)
	if query != None:
		if query == "points":
			# 获取坐标列表并设定状态
			command_string		= command_string[:-1]
			coordinates_list	= (command_string.split("/"))[1:] 
			return	event_status.coordinates_list_event.value
		elif query == "Nstart":
			return	event_status.nevigation_event.value
			# 需要创建对应线程
		elif query == "Pget":
			return	event_status.coordinates_get_event.value
		else:
			print("[×] Invalid query type")
			return event_status.error_event.value
	else:
		print("[×] Query type is None")
		return event_status.error_event.value
'''	
# [internal] 坐标点信息解析命令
#def parse_coordinates(command_string):

# 测试用例
if __name__ == "__main__":
	test_string	= parse_client_command("@q/Nstart*")
	print(test_string)
