#include <DMXSerial.h>
#include <Artnet.h>

const IPAddress ip(192, 168, 1, 240);
uint8_t mac[] = {0x04, 0xE9, 0xE5, 0x00, 0x69, 0xEC};

ArtnetReceiver artnet;
uint32_t universe_rec = 0;
uint32_t universe_send = 1;

void setup()
{
    analogWrite(6, 255);
    delay(1000);
    analogWrite(6, 1);
    
    Ethernet.begin(mac, ip);
    artnet.begin();

    DMXSerial.init(DMXController);

    artnet.subscribe(universe_rec, [&](uint8_t* data, uint16_t size)
    {
        for (int i = 0; i < size; i++)
        {
            if(i==0)
                analogWrite(6, data[i]);

            DMXSerial.write(i+1, data[i]);
        }
    });
}

void loop()
{
    artnet.parse();
}
