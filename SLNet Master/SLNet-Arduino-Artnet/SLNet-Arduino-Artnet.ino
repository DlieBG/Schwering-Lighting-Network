#include <DMXSerial.h>
#include <Artnet.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const IPAddress ip(192, 168, 0, 151);
uint8_t mac[] = {0x04, 0xE9, 0xE5, 0x00, 0x69, 0xEC};

ArtnetReceiver artnet;
const uint16_t size = 512;
uint8_t data[size];
uint32_t universe_rec = 0;
uint32_t universe_send = 1;

int stopper = 100;

int raw = 5; //grün
int etc = 6; //gelb
int brk = 9; //rot

#define OLED_RESET 4 // not used / nicht genutzt bei diesem Display
Adafruit_SSD1306 display(OLED_RESET);


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
        analogWrite(etc, 255);
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
    analogWrite(etc, 255);
    analogWrite(brk, 255);
    
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    display.clearDisplay();
  
    // set text color / Textfarbe setzen
    display.setTextColor(WHITE);
    // set text size / Textgroesse setzen
    display.setTextSize(1);
    // set text cursor position / Textstartposition einstellen
    display.setCursor(1,0);
    // show text / Text anzeigen
    display.println("OLED - Display - Test");
    display.setCursor(14,56);
    display.println("blog.simtronyx.de");
    display.setTextSize(2);
    display.setCursor(34,15);
    display.println("Hello");
    display.setCursor(30,34);
    display.println("World!");
    display.display();

    delay(10000);
    
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
