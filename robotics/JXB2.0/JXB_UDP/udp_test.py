import socket
import sys
import time
import urllib.error
import urllib.request
from urllib.parse import urlparse


def normalize_host(value):
    if "://" not in value:
        return value

    parsed = urlparse(value)
    return parsed.hostname or value


ESP32_IP = normalize_host(sys.argv[1]) if len(sys.argv) > 1 else "192.168.1.50"
ESP32_PORT = 8888


def print_local_ip():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.connect((ESP32_IP, ESP32_PORT))
        print(f"PC local IP: {sock.getsockname()[0]}")
    except OSError as exc:
        print(f"PC local IP: unavailable ({exc})")
    finally:
        sock.close()


def check_http():
    url = f"http://{ESP32_IP}/status"
    print(f"HTTP check: {url}")
    try:
        with urllib.request.urlopen(url, timeout=2) as response:
            body = response.read().decode("utf-8", errors="replace")
            print(f"HTTP OK: {body}")
            return True
    except urllib.error.URLError as exc:
        print(f"HTTP failed: {exc}")
        return False


def send(sock, message):
    sock.sendto(message.encode("utf-8"), (ESP32_IP, ESP32_PORT))
    try:
        data, addr = sock.recvfrom(1024)
        print(f"{message} -> {data.decode('utf-8')} from {addr[0]}:{addr[1]}")
    except socket.timeout:
        print(f"{message} -> timeout")


def main():
    print(f"ESP32 target: {ESP32_IP}:{ESP32_PORT}")
    print_local_ip()
    check_http()
    print()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(1.0)

    send(sock, "ping")
    time.sleep(0.5)
    send(sock, "movej:90,0,180,130,90")
    time.sleep(0.5)
    send(sock, '{"cmd":"movej","joints":[120,30,90,90,90]}')
    time.sleep(0.5)
    send(sock, "stop")
    time.sleep(0.5)
    send(sock, "run")


if __name__ == "__main__":
    main()
