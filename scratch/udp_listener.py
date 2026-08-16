import socket

UDP_IP = "127.0.0.1"
UDP_PORT = 4001

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.settimeout(5.0)

print("Listening on", UDP_IP, ":", UDP_PORT)
try:
    while True:
        data, addr = sock.recvfrom(1024)
        print("Received message:", data.hex())
except socket.timeout:
    print("Timeout reached")
