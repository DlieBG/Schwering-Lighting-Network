import socket

UID = 1
Port = 1

TCP_IP = '127.0.0.1'
TCP_PORT = 481
BUFFER_SIZE = 1024
MESSAGE = "I:"+str(UID)+":1:1:512"

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
s.send(str.encode(MESSAGE))
data = s.recv(BUFFER_SIZE)
s.close()
