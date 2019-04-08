#include <Artnet.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <DmxSimple.h>

Artnet artnet;

// Change ip and mac address for your setup
byte ip[] = {192, 168, 0, 151};
byte broadcast[] = {10, 0, 1, 255};
byte mac[] = {0x04, 0xE9, 0xE5, 0x00, 0x69, 0xEC};

void setup()
{
  Serial.begin(115200);
  artnet.begin(mac, ip);
  artnet.setBroadcast(broadcast);

  DmxSimple.usePin(3);
  DmxSimple.maxChannel(512);
}

void loop()
{
  uint16_t r = artnet.read();
  if(r == ART_POLL)
  {
    Serial.println("POLL");
  }
  if (r == ART_DMX)
  {
    if(artnet.getDmxFrame()[99]==0&&artnet.getDmxFrame()[100]==100)
    {
      for (int i = 0 ; i < artnet.getLength() ; i++)
      {
        int channel = i+1;
        int value = artnet.getDmxFrame()[i];

        DmxSimple.write(channel, value);
        
        if(channel==1||channel==430)
          analogWrite(9, value);
        if(channel==2||channel==304)
          analogWrite(6, value);
      }
    }
  }
}

