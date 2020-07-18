#include <DMXSerial.h>
#include <Artnet.h>

const IPAddress ip(192, 168, 1, 240);
uint8_t mac[] = {0x04, 0xE9, 0xE5, 0x00, 0x69, 0xEC};

ArtnetReceiver artnet;
const uint16_t size = 512;
uint8_t data[size];
uint32_t universe_rec = 0;
uint32_t universe_send = 1;

int stopper = 500;

int raw = 5; //grün
int etc = 6; //gelb
int brk = 9; //rot

void output(uint8_t* data, uint16_t size)
{
    if(stopper>0)
    {
      if(data[stopper-1]!=0||data[stopper]!=100)
      {
        //Break durch ETC-Trick
        analogWrite(etc, 0);
        analogWrite(brk, 255);
        delay(50);
        analogWrite(brk, 0);
      }
      else
      {
        //Output mit ETC-Trick
        analogWrite(etc, data[0]);
        analogWrite(raw, 0);
        
        for (int i = 0; i < size; i++)
            DMXSerial.write(i+1, data[i]);
      }
    }
    else
    {
      //Output raw
      analogWrite(etc, 0);
      analogWrite(raw, 255);
        
      for (int i = 0; i < size; i++)
          DMXSerial.write(i+1, data[i]);
    }
}

void setup()
{  
    analogWrite(raw, 255);
    analogWrite(etc, 2);
    analogWrite(brk, 255);
    
    Ethernet.begin(mac, ip);
    artnet.begin();

    DMXSerial.init(DMXController);

    artnet.subscribe(universe_rec, output);
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
