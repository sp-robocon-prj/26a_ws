#!/usr/bin/env python3
import socket

# 受信するIPアドレスとポート番号
# 0.0.0.0 はすべてのネットワークインターフェースからの受信を許可します
UDP_IP = "0.0.0.0"
UDP_PORT = 8888

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))

    print(f"Test UDP Server is listening on port {UDP_PORT}...")
    print("Waiting for PING messages...")

    try:
        while True:
            # データの受信
            data, addr = sock.recvfrom(1024)
            msg = data.decode('utf-8').strip()
            
            print(f"[{addr[0]}:{addr[1]}] Received: {msg}")
            
            # PINGを受け取ったらPONGを返す
            if msg == "PING":
                reply = "PONG"
                sock.sendto(reply.encode('utf-8'), addr)
                print(f" -> Sent reply: {reply}")
                
    except KeyboardInterrupt:
        print("\nServer stopped.")

if __name__ == "__main__":
    main()
