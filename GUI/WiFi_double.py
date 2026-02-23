# server_pi_advanced.py —— 增强双向通信版
import socket
import threading
import time

HOST = '0.0.0.0'
PORT = 65432

    # 启动主动发送线程
    periodic_thread = threading.Thread(target=send_periodic_data, daemon=True)
    periodic_thread.start()

    # 接收客户端消息
    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break
            msg = data.decode('utf-8', errors='replace').strip()
            print(f"[客户端→树莓派]: {msg}")

            # 🎯 支持命令响应
            if msg.lower() == "status":
                reply = "🟢 系统正常 | CPU: 25% | 内存: 45%"
            elif msg.lower() == "time":
                reply = f"🕒 当前时间: {time.strftime('%Y-%m-%d %H:%M:%S')}"
            else:
                reply = f"📌 已收到你的消息: '{msg}'"

            conn.sendall(reply.encode('utf-8'))
    except Exception as e:
        print(f"[错误] 通信异常: {e}")
    finally:
        conn.close()
        print(f"[服务端] 连接 {addr} 已关闭")

# 主程序
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen()
    print(f"[服务端] 等待客户端连接...")

    while True:
        conn, addr = s.accept()
        client_thread = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
        client_thread.start()
