import socket

UDP_IP = "0.0.0.0"  # Listen on all interfaces
UDP_PORT = 5657

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.settimeout(1.0)  # Timeout every 1 second to allow interruption

print(f"Listening on {UDP_PORT}...")

try:
    while True:
        try:
            data, addr = sock.recvfrom(1024)
            print(f"Received from {addr}: {data.decode()}")
        except socket.timeout:
            continue  # Avoids blocking indefinitely
except KeyboardInterrupt:
    print(f"\nStopped listening on {UDP_PORT}...")
finally:
    sock.close()
