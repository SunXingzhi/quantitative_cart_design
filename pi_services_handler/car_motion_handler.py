# 小车处理部分
from enum import Enum
import	pi_config
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
	OPEN_BRAKE_STATUS	= 8	# 开启刹车状态
	CLOSE_BRAKE_STATUS	= 9	# 关闭刹车状态
	# TURNING_IN_PLACE_STATUS	= 7	# 原地转弯状态
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
	6:	"导航状态",
	7:	"原地转弯状态",
	8:	"开启刹车状态",
	9:	"关闭刹车状态"
}

nevigation_data	= ""
# 解析传输的定位点数据
def parse_nevigation_data():
	global	nevigation_data
	# 判断数据是否为空
	if len(nevigation_data)==0:
		return None


# ======================= 获取当前状态 =======================
def get_status(xbox_data):
	global car_status_regiter
 
	is_brake = xbox_data.a
	is_position = xbox_data.y
	is_canceled = xbox_data.b
	
	new_status = car_status_regiter  # 默认保持原状态

	if is_brake:
		new_status = car_status.BRAKE_STATUS.value
	elif is_position:
		new_status = car_status.CHANGE_POSITION_STATUS.value
	elif is_canceled:
		new_status = car_status.DEFAULT_STATUS.value
	# 删除原地转弯的状态判断逻辑
	
	# 状态切换记录
	if new_status != car_status_regiter:
		# 处理从默认待机状态切换到刹车状态的情况
		if new_status == car_status.BRAKE_STATUS.value and car_status_regiter == car_status.DEFAULT_STATUS.value:
			new_status = car_status.OPEN_BRAKE_STATUS.value
		# 处理从刹车状态切换到默认待机状态的情况
		elif new_status == car_status.DEFAULT_STATUS.value and car_status_regiter == car_status.BRAKE_STATUS.value:
			new_status = car_status.CLOSE_BRAKE_STATUS.value
		print(f"new_status:{new_status}")
		old_name = STATUS_NAMES.get(car_status_regiter, "未知")
		new_name = STATUS_NAMES.get(new_status, "未知")
		print(f"状态切换: {old_name} → {new_name}")
		car_status_regiter = new_status
		
	return car_status_regiter

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

		case 1:		# 默认待机：其余清零
			for field_name, _ in xbox_data._fields_:
				if field_name not in ['lx', 'ly', 'rx', 'ry', 'lt', 'rt']:
					setattr(xbox_data, field_name, 0)
			
			return xbox_data
		case 2:
			return xbox_data
		case 3:		# 刹车状态：清零运动轴
			xbox_data.ly = 0
			xbox_data.rx = 0
			return xbox_data

		case 8:		# 开启刹车状态
			xbox_data.ly	= 0
			xbox_data.rx	= 0
			return xbox_data
		case 9:
			return xbox_data
		case '':		# 其他状态：保留原始数据（前提是已刷新！）
			return xbox_data

# ======================= 摇杆死区处理 =======================
# def apply_deadzone(value, deadzone=5000):
# 	"""小于死区的值归零，避免漂移"""
# 	return 0 if abs(value) < deadzone else value

# ======================= 数据转换 =======================
# 设定范围阈值
def convert_joystick_data(xbox_data):
	# 应用死区
	# lx = apply_deadzone(xbox_data.lx)
	# ly = apply_deadzone(xbox_data.ly)
	# rx = apply_deadzone(xbox_data.rx)
	# ry = apply_deadzone(xbox_data.ry)
	
	# ly = xbox_data.ly if abs(xbox_data.ly) > pi_config.IN_PLACE_LY_TURNING_THRESHOLD else 0
	# rx = xbox_data.rx if abs(xbox_data.rx) > pi_config.IN_PLACE_LX_TURNING_THRESHOLD else 0
	
	#	ly -> v_cx (线速度)
	#	rx -> ω (角速度)
	v_cx = -xbox_data.ly / pi_config.JOY_AXIS_MAX * pi_config.CAR_SPEED_MAX  # Y轴反向
	omega = xbox_data.rx / pi_config.JOY_AXIS_MAX * pi_config.CAR_ANGULAR_SPEED_MAX
	
	# 3. 限幅
	v_cx = max(min(v_cx, pi_config.CAR_SPEED_MAX), -pi_config.CAR_SPEED_MAX)
	omega = max(min(omega, pi_config.CAR_ANGULAR_SPEED_MAX), -pi_config.CAR_ANGULAR_SPEED_MAX)
	#print(f"ly:{xbox_data.ly}")
	# 4. 保留原始值用于其他控制

	return {
		'v_cx': v_cx,	  # 线速度 (m/s)
		'omega': omega,	# 角速度 (rad/s)
		'lx': int(xbox_data.lx),   
		'ly': int(xbox_data.ly),   
		'rx': int(xbox_data.rx),   
		'ry': int(xbox_data.ry),   
		'lt': int(xbox_data.lt),
		'rt': int(xbox_data.rt)
	}

# ======================= 四轮差速模型 =======================
def parsed_motors_speed(v_cx, omega):
	# 参数：v_cx - 线速度 (m/s), omega - 角速度 (rad/s)
	
	# 轮距参数（转换为米）
	left_right_dist = pi_config.LEFT_RIGHT_DISTANCE / 1000.0  # 轮距 (m)
	
	# 计算左右轮速度 (m/s)
	v_left = v_cx - (left_right_dist / 2.0) * omega
	v_right = v_cx + (left_right_dist / 2.0) * omega
	
	# 转换为百分比 (-100 ~ 100)
	# 假设 MAX_LINEAR_SPEED 对应 100%
	MAX_PERCENT = 100
	left_percent = int(v_left / pi_config.CAR_SPEED_MAX * MAX_PERCENT)
	right_percent = int(v_right / pi_config.CAR_SPEED_MAX * MAX_PERCENT)
	
	# 限幅
	left_percent = max(min(left_percent, MAX_PERCENT), -MAX_PERCENT)
	right_percent = max(min(right_percent, MAX_PERCENT), -MAX_PERCENT)
	
	# 四个轮子：同侧轮子速度相同
	v_fl = v_bl = left_percent
	v_fr = v_br = right_percent
	
	return {
		'v_fronted_left': v_fl,
		'v_fronted_right': -v_fr,
		'v_backed_left': v_bl,
		'v_backed_right': -v_br
	}

# =================== 舵机数据转换函数 ======================
def angle_mapping(pos):
	angle	= int(pos-pi_config.JOY_AXIS_MIN)/(pi_config.JOY_AXIS_MAX-pi_config.JOY_AXIS_MIN)*pi_config.REVOLVE_MAX
	return	angle

# ======================= 舵机 =======================
def parsed_motors_angle(pos1,pos2):
	angle_x =int(angle_mapping(pos1))
	angle_y =int(angle_mapping(pos2))
	return {
		'angle_X':angle_x,
		'angle_Y':angle_y
	}





