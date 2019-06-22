#include <DMXSerial.h>
#include <Artnet.h>

const IPAddress ip(192, 168, 0, 151);
uint8_t mac[] = {0x04, 0xE9, 0xE5, 0x00, 0x69, 0xEC};

ArtnetReceiver artnet;
const uint16_t size = 512;
uint8_t data[size];
uint32_t universe_rec = 0;
uint32_t universe_send = 1;

int stopper = 100;

void setup()
{
    Ethernet.begin(mac, ip);
    artnet.begin();

    DMXSerial.init(DMXController);

    artnet.subscribe(universe_rec, [&](uint8_t* data, uint16_t size)
    {
        if(data[stopper-1]!=0||data[stopper]!=100)
        {
          //ETC-Trick!
        }
        else
        {
          for (int i = 0; i < size; i++)
            {
                if(i==429)
                    analogWrite(9, data[i]);
                if(i==303)
                    analogWrite(6, data[i]);
    
                DMXSerial.write(i+1, data[i]);
            }
        }
    });
}

void loop()
{
    artnet.parse();

    /*for(int i=0; i<512; i++)
    {
        data[i]=125;
        //data[i] = DMXSerial.read(i+1);
    }
    
    artnet.set(universe_send, data, size);
    artnet.streaming();*/
}
