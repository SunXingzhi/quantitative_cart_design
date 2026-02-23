# ======================= C结构体与函数绑定 =======================
from ctypes import c_int, Structure, POINTER, byref
import	ctypes
import	sys
import	time

try:
	import pi_config
	JOY_STICK_SO_PATH = pi_config.JOY_STICK_SO_PATH
except ImportError:
	print("警告: 无法导入 pi_config，使用默认配置")
	JOY_STICK_SO_PATH = './get_xbox_data.so'  # 默认路径

#import	pi_config
# 加载动态库
try:
	xbox_lib = ctypes.CDLL(JOY_STICK_SO_PATH)
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

if __name__ == "__main__":
	try:
		fd		= xbox_open(pi_config.DEVICE_PATH)

		while True:
			if fd<0:
				print("Failed to open the joystick")
				fd	= xbox_open(device_path)
				time.sleep(1)
			else:
				print("Successed to open the joystick")
				break
		# Read the joystick data

		map_data	= XboxMap()
		while True:
			result	= xbox_map_read(fd, ctypes.byref(map_data))
			if result < 0:
				print("xbox_don't change", end='\r')
				time.sleep(0.01)
				continue
			# print(f"map_data_ry:{map_data.ry}")
	except Exception:
		print("Error!")


