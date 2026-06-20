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

def wait_until_frame(sock, target_frame):
    print(f"Waiting for runner to reach frame {target_frame}...")
    while True:
        r = send_recv(sock, {"cmd": "frame", "id": 99})
        curr_frame = r.get("frame", 0)
        if curr_frame >= target_frame:
            print(f"Frame {curr_frame} reached!")
            break
        time.sleep(0.5)

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    print("Connecting to runner...")
    for _ in range(10):
        try:
            s.connect(('127.0.0.1', 4370))
            break
        except Exception:
            time.sleep(1)
    
    print("Waiting 0.5s for boot...")
    time.sleep(0.5)
    
    # Skip FMV
    print("Pressing START to skip FMVs...")
    send_recv(s, {"cmd": "set_input", "buttons": "0x0008", "id": 1})
    time.sleep(0.05)
    send_recv(s, {"cmd": "clear_input", "id": 2})
    time.sleep(0.1)
    
    # Wait for Title (osm == 15)
    print("Waiting for Title Screen...")
    while True:
        r = send_recv(s, {"cmd": "read_ram", "addr": "0x8013DC60", "len": 4, "id": 3})
        val_hex = r.get('hex', '')
        if val_hex in ('0f000000', '0000000F', '0F000000'):
            break
        time.sleep(0.005) # Poll very fast (5ms) to catch the window before auto-demo starts
        
    print("At Title. Waiting for runner to reach frame 2000 to ensure interactive state...")
    wait_until_frame(s, 2000)
    
    def press(btn_hex, name, delay=0.1):
        print(f"Pressing {name} ({btn_hex})...")
        send_recv(s, {"cmd": "set_input", "buttons": btn_hex, "id": 4})
        time.sleep(0.02) # Hold button for 20ms
        send_recv(s, {"cmd": "clear_input", "id": 5})
        time.sleep(delay)

    # Menu navigation
    press("0x0008", "START")
    press("0x2000", "CIRCLE")
    press("0x0040", "DOWN")
    press("0x2000", "CIRCLE")
    
    # Get current frame to wait for Stage Select to load
    r = send_recv(s, {"cmd": "frame", "id": 100})
    entered_frame = r.get("frame", 0)
    wait_until_frame(s, entered_frame + 1800)
    press("0x4000", "CROSS")
    
    print("Waiting for tutorial screen (osm == 10)...")
    for _ in range(500):
        r = send_recv(s, {"cmd": "read_ram", "addr": "0x8013DC60", "len": 4, "id": 6})
        val_hex = r.get('hex', '')
        if val_hex in ('0a000000', '0000000A', '0A000000'):
            print("Tutorial screen reached!")
            break
        time.sleep(0.05)
    
    # Press START to bypass tutorial
    print("Pressing START to bypass tutorial...")
    press("0x0008", "START")
    
    print("Waiting for active gameplay (osm == 0)...")
    consecutive_zeros = 0
    for wait_iter in range(300):
        r = send_recv(s, {"cmd": "read_ram", "addr": "0x8013DC60", "len": 4, "id": 7})
        val_hex = r.get('hex', '')
        if val_hex in ('00000000', '00000000'):
            consecutive_zeros += 1
            if consecutive_zeros >= 3:
                print(f"Active gameplay stably reached at iteration {wait_iter}!")
                break
        else:
            consecutive_zeros = 0
        time.sleep(0.05)
        
    # Launch the ball!
    # In Paranoiascape, you pull back the plunger by holding DOWN (0x0040) or CROSS (0x4000). Let's hold DOWN and CROSS for 0.5s.
    print("Pulling plunger...")
    send_recv(s, {"cmd": "set_input", "buttons": "0x4040", "id": 8})
    time.sleep(0.5)
    print("Releasing plunger...")
    send_recv(s, {"cmd": "clear_input", "id": 9})
    time.sleep(0.2)

    # Now play by mashing flippers (L1 = 0x0400, R1 = 0x0800) and pulling plunger on drain
    print("Mashing flippers...")
    for cycle in range(1000):
        # Press L1 + R1
        send_recv(s, {"cmd": "set_input", "buttons": "0x0C00", "id": 10})
        time.sleep(0.05)
        send_recv(s, {"cmd": "clear_input", "id": 11})
        time.sleep(0.05)
        
        # Periodically pull plunger to relaunch if drained (holding DOWN + CROSS)
        if cycle % 25 == 0:
            print(f"Triggering periodic plunger pull (cycle {cycle})...")
            send_recv(s, {"cmd": "set_input", "buttons": "0x4040", "id": 12})
            time.sleep(0.5)
            send_recv(s, {"cmd": "clear_input", "id": 13})
            time.sleep(0.1)

    print("Waiting until frame 12000 to ensure we capture gameplay rendering...")
    wait_until_frame(s, 12000)

    # Quit cleanly
    print("Sending quit to runner...")
    send_recv(s, {"cmd": "quit", "id": 14})

if __name__ == '__main__':
    main()
