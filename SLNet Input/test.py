import socket

UID = 1
Port = 1

UDP_IP = '127.0.0.1'
UDP_PORT = 481
BUFFER_SIZE = 1024
MESSAGE = "I:"+str(UID)+":"+str(Port)+":2:0"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
sock.sendto(str.encode(MESSAGE), (UDP_IP, UDP_PORT))
