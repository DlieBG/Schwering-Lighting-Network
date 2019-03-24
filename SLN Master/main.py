import sql
from socket import *
import _thread as thread
 
BUFF = 1024
HOST = '127.0.0.1'# must be input parameter @TODO
PORT = 481 # must be input parameter @TODO


def send(IP, UID, Port, Channel, Value):
    print("OK")

def updateUniversum(Universum, Channel, Value):
    for univ in Universum:
        univ = univ[0]
        for row in sql.UniversumToOutput(univ):
            UID = row[0]
            Port = row[1]
            IP = sql.getIP(UID)[0][0][0]
            print("Sende SLN Univers "+str(univ)+" an: "+str(UID)+", Port: "+str(Port))
            send(IP, UID, Port, Channel, Value)

def handler(clientsock,addr):
    while 1:
        try:
            data = clientsock.recv(BUFF)
            data = str(data).replace("'", "")[1:]
            print (data)
            if data[0] is "I": #Input
                parts = data.split(":")
                UID = parts[1]
                Port = parts[2]
                Channel = parts[3]
                Value = parts[4]
                ToUniversum = sql.InputToUniversum(UID, Port)

                print("Neuer Dateneingang:")
                print("UID: "+UID)
                print("Port: "+Port)
                print("Kanal: "+Channel)
                print("Wert: "+Value)

                updateUniversum(ToUniversum, Channel, Value)


            if not data:
                clientsock.close()
        except error:
            pass
        
 
if __name__=='__main__':
    print("Willkommen im Schwering Lighting Network Master")
    print("Registrierte Inputs:")
    print(sql.Inputs())
    print("Registrierte Outputs:")
    print(sql.Outputs())

    ADDR = (HOST, PORT)
    serversock = socket(AF_INET, SOCK_STREAM)
    serversock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
    serversock.bind(ADDR)
    serversock.listen(1)
    while 1:
        print ('Warte auf Verbindung')
        clientsock, addr = serversock.accept()
        print ('Verbindung mit:', addr)
        thread.start_new_thread(handler, (clientsock, addr))