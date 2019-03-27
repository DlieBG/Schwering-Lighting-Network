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
stopper = 100;

#Port der Schnittstelle
#port = "/dev/ttyACM1"

sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
sock.bind(('',6454))

dmxeq = {}

import serial
#if __name__ == "__main__":
#    ser = serial.Serial(port, 2000000) #Bitte die Schnittstelle aendern!
def chanval(chan, val):
	a = ""
#	ser.write("{0:0=3d}".format(chan)+" "+"{0:0=3d}".format(val))
	#sys.stdout.write("\r%d -> %d  \t [master@%f]" % (chan,val,0))
	#sys.stdout.flush()

print ("bschwering Software ArtNet to DMX512 ist aktiv!")

dmxstates = []
dmxinit = False

for i in xrange(1,514):
	dmxstates.append(-1)

def lhex(h):
    return ':'.join(x.encode('hex') for x in h)
def setDmxValue(i, val):
	if dmxstates[i] == -1:
		dmxstates[i] = val
	if dmxstates[i] != val:
		dmxstates[i] = val
		# DMX UPDATE!!! WOW!!!
		#print("Setze Kanal %d auf den Wert %d" % (i, ord(val)))
		MESSAGE = "I:0:1:%d:%d" % (i, ord(val))
		sendsock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
		sendsock.sendto(str.encode(MESSAGE), ('192.168.0.97', 481))
		chanval(i,ord(val))
	
try:
	while True:
		  data = sock.recv(10240)

		  if len(data) < 20:
			continue

		  if data[0:7] != "Art-Net" or data[7] != "\0":
			# artnet package
			continue60
		  
		  if ord(data[8]) != 0x00 or ord(data[9]) != 0x50:
			# OpDmx
			continue
		  
		  protverhi = ord(data[10])
		  protverlo = ord(data[11])
		  sequence  = ord(data[12])
		  physical  = ord(data[13])
		  subuni    = ord(data[14])
		  net       = ord(data[15])
		  lengthhi  = ord(data[16])
		  length    = ord(data[17])
		  dmx       = data[18:]
		  
		  if ord(dmx[stopper-1]) == 255:
                        print("ETC-Trick!")
		  else:
		  	for i in xrange(0,510):
				setDmxValue(i+1,dmx[i])

			
		  #print data[0:4]
		  
		  #print lhex(dmx)
except KeyboardInterrupt:
    pass
    
print "Programm beendet!"
 
 
