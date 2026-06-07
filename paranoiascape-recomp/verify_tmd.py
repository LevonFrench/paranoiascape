"""
Hold-connection FMV skip: keeps TCP alive so set_input isn't cleared on disconnect.
Presses START for 3 seconds, then clears, then presses again to advance title screen.
"""
import socket
import json
import time
import sys

HOST = "127.0.0.1"
PORT = 4370

def connect():
    """Create persistent connection to debug server."""
    return socket.create_connection((HOST, PORT), timeout=5.0)

def send_recv(sock, cmd_dict):
    """Send command on existing socket, read response."""
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
    print("=== FMV Skip (held connection) ===")
    print()
    
    # Connect and keep connection open
    print("Connecting to debug server...")
    sock = connect()
    
    # Verify connection
    # Skip Intro FMV (START)
    print("\n[1] Pressing START to skip intro FMV...")
    send_recv(sock, {"cmd": "set_input", "buttons": "0x0800", "id": 1})
    time.sleep(1.0)
    send_recv(sock, {"cmd": "clear_input", "id": 2})
    time.sleep(1.0)

    # Skip Logo FMV (START)
    print("\n[2] Pressing START to skip logo...")
    send_recv(sock, {"cmd": "set_input", "buttons": "0x0800", "id": 3})
    time.sleep(1.0)
    send_recv(sock, {"cmd": "clear_input", "id": 4})
    
    # Wait for Title Screen 3D engine to render
    print("\n[3] Waiting for Title Screen (draw_offset=160,120)...")
    while True:
        r = send_recv(sock, {"cmd": "gpu_state", "id": 5})
        if r.get('draw_offset_x') == 160 and r.get('draw_offset_y') == 120:
            break
        time.sleep(1.0)
        
    # Press START to go to Main Menu
    print("\n[4] Pressing START (Title -> Main Menu)...")
    send_recv(sock, {"cmd": "set_input", "buttons": "0x0800", "id": 6})
    time.sleep(1.0)
    send_recv(sock, {"cmd": "clear_input", "id": 7})
    time.sleep(2.0)
    
    print("\n[5] Brute-forcing Menu Buttons (START, CROSS, CIRCLE, TRIANGLE, SQUARE)...")
    for btn_name, btn_mask in [("START", "0x0800"), ("CROSS", "0x4000"), ("CIRCLE", "0x2000"), ("TRIANGLE", "0x1000"), ("SQUARE", "0x8000")]:
        print(f"  Pressing {btn_name}...")
        send_recv(sock, {"cmd": "set_input", "buttons": btn_mask, "id": 8})
        time.sleep(0.5)
        send_recv(sock, {"cmd": "clear_input", "id": 9})
        time.sleep(0.5)
    time.sleep(2.0)
    
    # GPU state
    print("\n[5] GPU State:")
    r = send_recv(sock, {"cmd": "gpu_state", "id": 11})
    print(f"  display_area: ({r.get('display_area_x','?')}, {r.get('display_area_y','?')})")
    print(f"  draw_offset: ({r.get('draw_offset_x','?')}, {r.get('draw_offset_y','?')})")
    
    # Monitor for 30 seconds
    print("\n[6] Monitoring for 30s...")
    for i in range(30):
        time.sleep(1)
        r = send_recv(sock, {"cmd": "gpu_state", "id": 20+i})
        f = send_recv(sock, {"cmd": "ping", "id": 40+i})
        print(f"  [{i+1:2d}/30] frame={f.get('frame','?'):>6}  "
              f"da=({r.get('display_area_x','?')},{r.get('display_area_y','?')})  "
              f"do=({r.get('draw_offset_x','?')},{r.get('draw_offset_y','?')})")
    
    print("\n=== Done ===")
    print("Check sshots/ for auto-screenshots and run_tmd_verify.log for GP0-DUMP output.")
    
    # Keep connection alive
    sock.close()

if __name__ == "__main__":
    main()
