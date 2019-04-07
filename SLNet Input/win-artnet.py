#!/usr/bin/env python

import json
import math
import socket
import struct
import zlib
import sys
import pickle
import subprocess
#from termcolor import colored

#Programmstop, wenn dieser Channel = 255 gesetzt wird!
stopper = 100

#Port der Schnittstelle
#port = "/dev/ttyACM1"

sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
ADDR = ('0.0.0.0', 6454)
sock.bind(ADDR)

dmxeq = {}

#if __name__ == "__main__":
#    ser = serial.Serial(port, 2000000) #Bitte die Schnittstelle aendern!
def chanval(chan, val):
	a = ""
#	ser.write("{0:0=3d}".format(chan)+" "+"{0:0=3d}".format(val))
	#sys.stdout.write("\r%d -> %d  \t [master@%f]" % (chan,val,0))
	#sys.stdout.flush()

print ("bschwering Software ArtNet to SLNet!")

dmxstates = []
dmxinit = False

for i in range(1,514):
	dmxstates.append(-1)

def lhex(h):
    return ':'.join(x.encode('hex') for x in h)
def setDmxValue(i, val):
	if dmxstates[i] != val or dmxstates[i] == -1:
		dmxstates[i] = val
		# DMX UPDATE!!! WOW!!!
		print("Sende %d:%d zum Master" % (i, val))
		MESSAGE = "I:0:1:%d:%d" % (i, val)
		sendsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
		sendsock.sendto(str.encode(MESSAGE), ('127.0.0.1', 481))
		chanval(i,val)
	
try:
	while True:
		data, addr = sock.recvfrom(1024)

		if len(data) < 20:
			continue

		if "Art-Net" not in str(data[0:7]) or "0" not in str(data[7]):
		# artnet package
			continue
		
		if data[8] != 0 or data[9] != 80:
		# OpDmx
			continue
		
		protverhi = data[10]
		protverlo = data[11]
		sequence  = data[12]
		physical  = data[13]
		subuni    = data[14]
		net       = data[15]
		lengthhi  = data[16]
		length    = data[17]
		dmx       = data[18:]

		
		
		if dmx[stopper-1] != 0 or dmx[stopper] != 100:
			print("ETC-Trick!")
		else:
			for i in range(0,510):
				setDmxValue(i+1,dmx[i])

		
		#print data[0:4]
		
		#print lhex(dmx)
except KeyboardInterrupt:
    pass
    
print("Programm beendet!")
 
 
