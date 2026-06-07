"""Send START via held connection, verify pad bytes change in RAM."""
import socket, json, time

def send(sock, cmd):
    sock.sendall((json.dumps(cmd)+"\n").encode())
    resp = b""
    while b"\n" not in resp:
        resp += sock.recv(4096)
    return json.loads(resp.strip())

s = socket.create_connection(("127.0.0.1", 4370), 5)

# Pre-check
r = send(s, {"cmd":"ping","id":1})
print("Frame:", r.get("frame"))

r = send(s, {"cmd":"read_ram","addr":"0x8009EB58","len":4,"id":2})
print("BEFORE pad (0x9EB58):", r.get("hex"))

# Set START (0x0008)
print("\nSetting START override...")
send(s, {"cmd":"set_input","buttons":"0x0008","id":3})
time.sleep(0.5)

# Check if pad bytes changed
r = send(s, {"cmd":"read_ram","addr":"0x8009EB58","len":4,"id":4})
print("DURING pad (0x9EB58):", r.get("hex"))

r = send(s, {"cmd":"read_ram","addr":"0x80008580","len":4,"id":5})
print("DURING game state:", r.get("hex"))

# Hold for 3 seconds
print("\nHolding START for 3s...")
for i in range(6):
    time.sleep(0.5)
    r = send(s, {"cmd":"read_ram","addr":"0x8009EB58","len":4,"id":10+i})
    gs = send(s, {"cmd":"read_ram","addr":"0x80008580","len":4,"id":20+i})
    sk = send(s, {"cmd":"read_scratch","addr":"0x1F8001D3","len":1,"id":30+i})
    print("  [%d] pad=%s state=%s skip=%s" % (i, r.get("hex"), gs.get("hex"), sk.get("hex")))

# Release
send(s, {"cmd":"clear_input","id":40})
print("\nReleased. Waiting 2s...")
time.sleep(2.0)

r = send(s, {"cmd":"read_ram","addr":"0x8009EB58","len":4,"id":50})
print("AFTER pad:", r.get("hex"))
r = send(s, {"cmd":"read_ram","addr":"0x80008580","len":4,"id":51})
print("AFTER game state:", r.get("hex"))

# Check scratchpad 0x1FC for newly-pressed edge (game checks this for button events)
r = send(s, {"cmd":"read_scratch","addr":"0x1F8001FC","len":4,"id":52})
print("Scratch[1FC] (newly-pressed):", r.get("hex"))

# Check 0x8009C9D8 (active-high pad result from FUN_80028D70)  
r = send(s, {"cmd":"read_ram","addr":"0x8009C9D8","len":4,"id":53})
print("Active-high pad (0x8009C9D8):", r.get("hex"))

s.close()
