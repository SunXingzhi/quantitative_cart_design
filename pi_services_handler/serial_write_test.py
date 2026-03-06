#!/usr/bin/env python3
"""
树莓派串口发送脚本
设备: /dev/ttyAMA0, 波特率 9600, 8N1, 无流控
输入消息后按回车发送，自动添加回车换行 \r\n
空行或 Ctrl+C 退出
"""

import serial
import sys

SERIAL_PORT = '/dev/ttyAMA0'
BAUDRATE = 9600

def main():
    try:
        # 打开串口
        ser = serial.Serial(
            port=SERIAL_PORT,
            baudrate=BAUDRATE,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=None,        # 写操作通常不需要超时
            xonxoff=False,
            rtscts=False
        )
        print(f"已打开串口 {SERIAL_PORT}，波特率 {BAUDRATE}")
        print("输入要发送的消息（空行退出）：")

        while True:
            try:
                msg = input("> ")
            except EOFError:
                break  # 如果输入流关闭则退出
            if msg == "":
                print("空行，退出发送程序")
                break

            # 将字符串编码为字节，并添加回车换行
            data = msg.encode('utf-8') + b'\r\n'
            try:
                ser.write(data)
                print(f"已发送 {len(data)} 字节")
            except serial.SerialTimeoutException:
                print("发送超时")
            except serial.SerialException as e:
                print(f"发送错误: {e}")
                break

    except serial.SerialException as e:
        print(f"串口错误: {e}")
        print("请检查设备路径和权限（可能需要加入 dialout 组）")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n用户中断，退出发送程序")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("串口已关闭")

if __name__ == "__main__":
    main()
