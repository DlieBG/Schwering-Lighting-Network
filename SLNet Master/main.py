import sql
import socket
import _thread as thread
 
BUFF = 1024
HOST = '0.0.0.0'# must be input parameter @TODO
PORT = 481 # must be input parameter @TODO


def send(IP, UID, Port, Channel, Value):
    MESSAGE = "O:"+str(UID)+":"+str(Port)+":"+str(Channel)+":"+str(Value)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 
    sock.sendto(str.encode(MESSAGE), (IP, PORT))

def updateUniversum(Universum, Channel, Value):
    for univ in Universum:
        univ = univ[0]
        for row in sql.UniversumToOutput(univ):
            UID = row[0]
            Port = row[1]
            IP = sql.getIP(UID)[0][0][0]
            print("Sende an "+str(UID)+":"+str(Port)+": "+str(Channel)+":"+str(Value))
            send(IP, UID, Port, Channel, Value)

def handler(data,addr):
    data = str(data).replace("'", "")[1:]
    if data[0] is "I": #Input
        parts = data.split(":")
        UID = parts[1]
        Port = parts[2]
        Channel = parts[3]
        Value = parts[4]
        ToUniversum = sql.InputToUniversum(UID, Port)

        #print("Neuer Dateneingang:")
        #print("UID: "+UID)
        #print("Port: "+Port)
        #print("Kanal: "+Channel)
        #print("Wert: "+Value)

        updateUniversum(ToUniversum, Channel, Value)

 
if __name__=='__main__':
    print("Willkommen im Schwering Lighting Network Master")
    print("Registrierte Inputs:")
    print(sql.Inputs())
    print("Registrierte Outputs:")
    print(sql.Outputs())

    ADDR = (HOST, PORT)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
    sock.bind(ADDR)
    while 1:
        data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
        handler(data, addr)
