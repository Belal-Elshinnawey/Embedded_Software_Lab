import socket
import select
import struct
from collections import defaultdict
import numpy as np
import cv2

RPI_HOST = "10.0.15.51"
DE10_HOST = "10.0.15.71"
MY_PI_HOST = "belalpi.local"
HOST = RPI_HOST
WIDTH = 320
HEIGHT = 240

frames = defaultdict(dict) 
expected_chunks = {}

latest_com = (-1, -1)

def show_frame(data, com):
    try:
        img = np.frombuffer(data, dtype=np.uint8).reshape((HEIGHT, WIDTH)) * 255
        img = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
        if com != (-1, -1):
            x, y = com
            x = int(np.clip(x, 0, WIDTH-1))
            y = int(np.clip(y, 0, HEIGHT-1))
            cv2.circle(img, (x, y), 5, (0, 0, 255), thickness=-1)

        cv2.imshow("Debug Mask + COM", img)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            exit(0)
    except Exception as e:
        print(f"Error displaying frame: {e}")

def assemble_frame(msg):
    frame_id, chunk_id, total_chunks = struct.unpack('<IHH', msg[:8])
    payload = msg[8:]

    frames[frame_id][chunk_id] = payload
    expected_chunks[frame_id] = total_chunks

    if len(frames[frame_id]) == expected_chunks[frame_id]:
        chunks = [frames[frame_id][i] for i in range(total_chunks)]
        full_data = b''.join(chunks)
        print(f"Received full frame {frame_id}, size={len(full_data)}")

        show_frame(full_data, latest_com)

        del frames[frame_id]
        del expected_chunks[frame_id]

def receive_com(data):
    global latest_com
    if len(data) >= 8:
        x, y = struct.unpack('<ff', data[:8])
        # Update only if values are valid
        if (x, y) == (-1.0, -1.0):
            latest_com = (-1, -1)
        else:
            latest_com = (x, y)
        print(f"COM received: {latest_com}")

def udp_client(server_address, ports):
    sockets = []
    for port in ports:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(5)
        sockets.append(sock)

    print(f"Sending handshake to {server_address} on ports {ports} and listening for replies...")

    for sock, port in zip(sockets, ports):
        sock.sendto(b"hello", (server_address, port))

    while True:
        readable, _, _ = select.select(sockets, [], [])
        for s in readable:
            data, addr = s.recvfrom(4096)
            _, src_port = addr
            if src_port == ports[0]:
                assemble_frame(data)
            elif src_port == ports[1]:
                receive_com(data)

if __name__ == "__main__":
    udp_client(HOST, [5000, 5001])
