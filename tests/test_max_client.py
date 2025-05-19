import socket

def open_idle_connection():
    s = socket.socket()
    s.connect(('127.0.0.1', 8080))
    return s

# Open MAX_CLIENTS connections
clients = [open_idle_connection() for _ in range(100)]

# Try one more connection
extra = socket.socket()
extra.connect(('127.0.0.1', 8080))
extra.send(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
print(extra.recv(1024).decode())

# Cleanup
for c in clients:
    c.close()
extra.close()
