#include <DMXSerial.h>
#include <Artnet.h>
#include <Ethernet.h>
#include <SPI.h>

Artnet artnet;

byte dmx[512];

// Change ip and mac address for your setup
byte ip[] = {192, 168, 0, 151};
byte mac[] = {0x04, 0xE9, 0xE5, 0x00, 0x69, 0xEC};

void setup()
{
  artnet.begin(mac, ip);
  
  DMXSerial.init(DMXController);
}

void loop()
{
  uint16_t r = artnet.read();
  if(r == ART_POLL)
  {
    
  }
  if (r == ART_DMX)
  {
    if(artnet.getDmxFrame()[99]==0&&artnet.getDmxFrame()[100]==100)
    {
      for (int i = 0 ; i < artnet.getLength() ; i++)
      {       
        int channel = i+1;
        int value = artnet.getDmxFrame()[i];

        if(dmx[i] != artnet.getDmxFrame()[i])
        {
          dmx[i] = artnet.getDmxFrame()[i];
          
          if(channel==1||channel==430)
            analogWrite(9, value);
          if(channel==2||channel==304)
            analogWrite(6, value);
  
          DMXSerial.write(channel, value);
        }
      }
    }
  }
}

