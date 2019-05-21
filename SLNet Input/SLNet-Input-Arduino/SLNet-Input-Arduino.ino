#include <SPI.h>        
#include <Ethernet.h>
#include <EthernetUdp.h>        
#include <string.h>
#include <DmxSimple.h>
#include <DMXSerial.h>


String UID = "1";
IPAddress ip(192, 168, 0, 152);
IPAddress masterIP(192, 168, 0, 97);
byte dmx[512];

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

unsigned int localPort = 481;      // local port to listen on

// buffers for receiving and sending data
char packetBuffer[14];  //buffer to hold incoming packet,
char  ReplyBuffer[] = "ok";       // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
  
  DMXSerial.init(DMXReceiver);
}

void loop() {
  for(int i=0; i<512; i++)
  {
    if(dmx[i]!=DMXSerial.read(i+1))
    {
      dmx[i]=DMXSerial.read(i+1);
      Udp.beginPacket(masterIP, localPort);
      Udp.print("I:"+UID+":1:"+(i+1)+":"+dmx[i]);
      Udp.endPacket();
    }
  }
}

