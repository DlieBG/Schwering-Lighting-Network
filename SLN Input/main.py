import socket

UID = 1
Port = 1

UDP_IP = '0.0.0.0'
UDP_PORT = 481
BUFFER_SIZE = 1024
MESSAGE = "I:"+str(UID)+":1:1:512"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
sock.sendto(str.encode(MESSAGE), (UDP_IP, UDP_PORT))
