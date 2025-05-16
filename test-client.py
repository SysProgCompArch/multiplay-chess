import socket

def tcp_client(host: str, port: int) -> None:
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((host, port))
            print(f"Connected to server({host}:{port}). Type 'exit' to quit.")

            while True:
                try:
                    message = input('msg > ')
                    if message.lower() == 'exit':
                        print('bye')
                        break

                    sock.sendall(message.encode('utf-8'))
                    print(f"Sent: {message}")

                    response = sock.recv(4096)
                    if not response:
                        print("서버와의 연결이 끊어졌습니다.")
                        break
                    print(f"Received: {response.decode('utf-8')}")
                except (ConnectionResetError, BrokenPipeError):
                    print("서버와의 연결이 강제로 종료되었습니다.")
                    break
                except socket.error as e:
                    print(f"소켓 오류 발생: {e}")
                    break
    except socket.error as e:
        print(f"서버에 연결할 수 없습니다: {e}")

if __name__ == "__main__":
    HOST = "127.0.0.1"   # 서버 IP 또는 호스트 이름
    PORT = 8080          # 서버 포트
    tcp_client(HOST, PORT)
