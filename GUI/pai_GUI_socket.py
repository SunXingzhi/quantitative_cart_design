# client_cli.py —— 命令行 TCP 客户端，与树莓派通信

import socket
import threading
import sys
import argparse

class TCPClient:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None
        self.running = True

    def connect(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            print(f"✅ 已连接到 {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"❌ 连接失败: {e}")
            return False

    def start_receiving(self):
        """启动接收线程"""
        def receive_loop():
            while self.running:
                try:
                    data = self.sock.recv(1024)
                    if not data:
                        print("⚠️ 服务器断开连接")
                        break
                    print(f"\n[树莓派]: {data.decode('utf-8', errors='ignore')}")
                    print(">>> ", end="", flush=True)  # 恢复输入提示
                except Exception as e:
                    if self.running:
                        print(f"\n❌ 接收出错: {e}")
                    break
            self.close()

        thread = threading.Thread(target=receive_loop, daemon=True)
        thread.start()

    def send_message(self, message):
        try:
            self.sock.sendall(message.encode('utf-8'))
        except Exception as e:
            print(f"❌ 发送失败: {e}")

    def close(self):
        self.running = False
        if self.sock:
            try:
                self.sock.shutdown(socket.SHUT_RDWR)
            except:
                pass
            self.sock.close()
        print("\n🔌 连接已关闭")

def main():
    parser = argparse.ArgumentParser(description="TCP客户端 - 与树莓派通信")
    parser.add_argument("host", help="172.16.100.137")
    parser.add_argument("-p", "--port", type=int, default=65432, help="65432")
    args = parser.parse_args()

    client = TCPClient(args.host, args.port)

    if not client.connect():
        sys.exit(1)

    client.start_receiving()

    print("💬 输入消息并回车发送，输入 'quit' 或按 Ctrl+C 退出\n>>> ", end="", flush=True)

    try:
        while client.running:
            try:
                msg = input()
                if msg.lower() in ('quit', 'exit'):
                    break
                if msg.strip():
                    client.send_message(msg)
                print(">>> ", end="", flush=True)
            except EOFError:
                break
    except KeyboardInterrupt:
        print("\n⌨️  用户中断")
    finally:
        client.close()

if __name__ == "__main__":
    main()
