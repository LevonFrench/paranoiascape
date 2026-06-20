import socket
import json
import time

def send_recv(sock, cmd_dict):
    msg = json.dumps(cmd_dict) + "\n"
    sock.sendall(msg.encode())
    resp = b""
    while b"\n" not in resp:
        chunk = sock.recv(4096)
        if not chunk:
            break
        resp += chunk
    return json.loads(resp.decode().strip())

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    for _ in range(10):
        try:
            s.connect(('127.0.0.1', 4370))
            break
        except Exception:
            time.sleep(1)
    
    print("Waiting 5s for boot...")
    time.sleep(5)
    
    # Skip FMV
    print("Pressing START to skip FMVs...")
    send_recv(s, {"cmd": "set_input", "buttons": "0x0008", "id": 1})
    time.sleep(0.5)
    send_recv(s, {"cmd": "clear_input", "id": 2})
    time.sleep(2)
    
    # Wait for Title
    while True:
        r = send_recv(s, {"cmd": "read_ram", "addr": "0x8013DC60", "len": 4, "id": 6})
        print(f"8013DC60={r}")
        if r.get('hex') in ('0f000000', '0000000F', '0F000000'): # 15 = Title Screen
            break
        time.sleep(1)
        
    print("Waiting 5s for Title Screen to become interactive...")
    time.sleep(5)
    print("At Title. Executing sequence...")
    
    def press(btn_hex, name):
        print(f"Pressing {name} ({btn_hex})...")
        send_recv(s, {"cmd": "set_input", "buttons": btn_hex, "id": 1})
        time.sleep(0.3)
        send_recv(s, {"cmd": "clear_input", "id": 2})
        time.sleep(2.0)

    press("0x0008", "START")
    press("0x2000", "CIRCLE")
    press("0x0040", "DOWN")
    press("0x2000", "CIRCLE")
    press("0x4000", "CROSS")

    
    for i in range(20):
        time.sleep(1)
        r = send_recv(s, {"cmd": "gpu_state", "id": 5})
        v1 = send_recv(s, {"cmd": "read_ram", "addr": "0x80134AE8", "len": 4, "id": 6})
        v2 = send_recv(s, {"cmd": "read_ram", "addr": "0x8013DC60", "len": 4, "id": 7})
        print(f"[{i}] offset={r.get('draw_offset_x')},{r.get('draw_offset_y')} 80134AE8={v1.get('hex')} 8013DC60={v2.get('hex')}")

if __name__ == '__main__':
    main()
