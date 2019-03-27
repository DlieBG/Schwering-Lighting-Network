#include <SPI.h>        
#include <Ethernet.h>
#include <EthernetUdp.h>        
#include <string.h>


String UID = "2";


byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 151);

unsigned int localPort = 481;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  //buffer to hold incoming packet,
char  ReplyBuffer[] = "ok";       // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  // start the Ethernet and UDP:
  digitalWrite(8, HIGH);
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);

  Serial.begin(9600);
  digitalWrite(8, LOW);
}

void loop() {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    for( int i = 0; i < sizeof(packetBuffer);  ++i )
      packetBuffer[i] = (char)0;
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    
    char* token = strtok(packetBuffer, ":");
    String typ = token;
    token= strtok(0, ":");
    String uid = token;
    token = strtok(0, ":");
    String port = token;
    token = strtok(0, ":");
    String channel = token;
    token = strtok(0, ":");
    String value = token;

    if(uid.equals(UID))
    {
      analogWrite(channel.toInt(), value.toInt());
    }
  }
}

