import socket
import select
import time

HEARTBEAT_INTERVAL = 5  # 초
HEARTBEAT_MSG = '__ping__'


def tcp_client(host: str, port: int) -> None:
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((host, port))
            print(f"Connected to server({host}:{port}). Type 'exit' to quit.")
            sock.setblocking(0)

            last_heartbeat = time.time()
            while True:
                try:
                    now = time.time()
                    timeout = max(0, HEARTBEAT_INTERVAL - (now - last_heartbeat))
                    rlist, _, _ = select.select([sock, 0], [], [], timeout)

                    # 표준 입력 처리
                    if 0 in rlist:
                        message = input('msg > ')
                        if message.lower() == 'exit':
                            print('bye')
                            break
                        sock.sendall(message.encode('utf-8'))
                        print(f"Sent: {message}")

                    # 소켓 수신 처리
                    if sock in rlist:
                        response = sock.recv(4096)
                        if not response:
                            print("서버와의 연결이 끊어졌습니다.")
                            break
                        decoded = response.decode('utf-8')
                        if decoded == HEARTBEAT_MSG:
                            # 서버의 heartbeat 응답은 무시
                            continue
                        print(f"Received: {decoded}")

                    # heartbeat 전송
                    if time.time() - last_heartbeat >= HEARTBEAT_INTERVAL:
                        try:
                            sock.sendall(HEARTBEAT_MSG.encode('utf-8'))
                        except Exception as e:
                            print(f"Heartbeat 전송 실패: {e}")
                            break
                        last_heartbeat = time.time()
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
