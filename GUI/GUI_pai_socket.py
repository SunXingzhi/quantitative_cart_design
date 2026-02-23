import socket

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind(('0.0.0.0', 65432))
server_socket.listen(5)
print("服务端已启动，等待客户端连接...")

while True:
    client_socket, addr = server_socket.accept()
    print(f"连接地址：{addr}")
    
    client_socket.send("欢迎访问服务端！".encode('utf-8'))
    # 为当前客户端开启一个消息循环
    while True:
        try:
            data = client_socket.recv(1024).decode('utf-8')
            if not data:  # 客户端关闭连接时，recv 会返回空字节
                print(f"客户端 {addr} 已断开")
                break
            print(f"客户端消息：{data}")
        except ConnectionResetError:
            print(f"客户端 {addr} 强制断开")
            break
        except Exception as e:
            print(f"错误：{e}")
            break
    
    client_socket.close()  # 处理完该客户端后关闭连接
