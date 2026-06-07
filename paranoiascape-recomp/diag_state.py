import socket, json, time

def send(sock, cmd):
    sock.sendall((json.dumps(cmd)+"\n").encode())
    resp = b""
    while b"\n" not in resp:
        resp += sock.recv(4096)
    return json.loads(resp.strip())

s = socket.create_connection(("127.0.0.1", 4370), 2)

# Check frame and GPU state
r = send(s, {"cmd":"ping","id":1})
print("Frame:", r.get("frame"))

r = send(s, {"cmd":"gpu_state","id":2})
print("GPU: da=(%s,%s) do=(%s,%s)" % (
    r.get("display_area_x"), r.get("display_area_y"),
    r.get("draw_offset_x"), r.get("draw_offset_y")))

# Read game state byte at 0x80008580
r = send(s, {"cmd":"read_ram","addr":"0x80008580","len":4,"id":3})
print("Game state (0x80008580):", r.get("hex"))

# Check pad state at 0x9eb5a
r = send(s, {"cmd":"read_ram","addr":"0x8009EB58","len":4,"id":4})
print("Pad state (0x8009EB58):", r.get("hex"))

# Check scratchpad skip flag
r = send(s, {"cmd":"read_scratch","addr":"0x1F8001D3","len":1,"id":5})
print("FMV skip flag (scratch[1D3]):", r.get("hex"))

# Check FMV-related scratch state
r = send(s, {"cmd":"read_scratch","addr":"0x1F8001CC","len":16,"id":6})
print("Scratch[1CC..1DB]:", r.get("hex"))

# Check if DrawOTag was ever called - read the counter
r = send(s, {"cmd":"read_ram","addr":"0x8009D6AC","len":4,"id":7})
print("OT head A (0x8009D6AC):", r.get("hex"))

r = send(s, {"cmd":"read_ram","addr":"0x8009E3BC","len":4,"id":8})
print("OT head B (0x8009E3BC):", r.get("hex"))

# Get recent frame record
r = send(s, {"cmd":"get_frame","frame": send(s, {"cmd":"ping","id":9}).get("frame",0) - 1, "id":10})
print("Last frame PC:", r.get("pc"), "dispatch_misses:", r.get("dispatch_misses"))
print("Last func:", r.get("last_func"))

s.close()
