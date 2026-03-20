#!/usr/bin/env python3
"""
树莓派串口接收脚本（按行读取）
设备: /dev/ttyAMA0, 波特率 9600, 8N1, 无流控
按 Ctrl+C 退出
"""

import serial
import sys

SERIAL_PORT = '/dev/ttyAMA0'
BAUDRATE = 9600
TIMEOUT = 1  # 读取超时时间（秒），避免永久阻塞，同时允许响应 Ctrl+C

def main():
    try:
        # 打开串口
        ser = serial.Serial(
            port=SERIAL_PORT,
            baudrate=BAUDRATE,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=TIMEOUT,
            xonxoff=False,      # 禁用软件流控
            rtscts=False         # 禁用硬件流控
        )
        print(f"已打开串口 {SERIAL_PORT}，波特率 {BAUDRATE}")
        print("等待接收数据行... (按 Ctrl+C 退出)\n")

        while True:
            try:
                # 读取一行数据（包含行结束符 \n）
                line = ser.readline()
                if line:
                    # 尝试按 UTF-8 解码并去除末尾的换行符
                    try:
                        decoded = line.decode('utf-8').rstrip('\r\n')
                    except UnicodeDecodeError:
                        # 如果解码失败，显示十六进制形式
                        decoded = f"<hex> {line.hex()}"
                    print(f"收到: {decoded}")
                # 如果 line 为空（超时），继续循环，以便检查 KeyboardInterrupt
            except serial.SerialException as e:
                print(f"读取错误: {e}")
                break

    except serial.SerialException as e:
        print(f"串口错误: {e}")
        print("请检查设备路径和权限（可能需要加入 dialout 组）")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n\n用户中断，退出接收程序")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("串口已关闭")

if __name__ == "__main__":
    main()
