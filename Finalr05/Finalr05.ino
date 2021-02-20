// Include application, user and local libraries
#include "SPI.h"
#include "TFT_22_ILI9225.h"
#include "SdsDustSensor.h"
//#include "dimmable_light.h"
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
//#define lowSpeed 120
//#define mediumSpeed 150
//#define highSpeed 230
SdsDustSensor sds(Serial2);


#define BUTTON_ON_OFF 4  
#define BUTTON_ION 5
#define BUTTON_MODE 18
#define BUTTON_FAN 19
//#define ION 32
//#define syncPin 21 //Chân zero đọc điểm 0 điện áp
//#define SPEED 33
// Include font definition files
#include <../fonts/FreeSans12pt7b.h>
#include <../fonts/FreeSans24pt7b.h>

#if defined (ARDUINO_ARCH_STM32F1)
#define TFT_RST PA1
#define TFT_RS  PA2
#define TFT_CS  PA0 // SS
#define TFT_SDI PA7 // MOSI
#define TFT_CLK PA5 // SCK
#define TFT_LED 0 // 0 if wired to +5V directly
#elif defined(ESP8266)
#define TFT_RST 4   // D2
#define TFT_RS  5   // D1
#define TFT_CLK 14  // D5 SCK
//#define TFT_SDO 12  // D6 MISO
#define TFT_SDI 13  // D7 MOSI
#define TFT_CS  15  // D8 SS
#define TFT_LED 2   // D4     set 0 if wired to +5V directly -> D3=0 is not possible !!
#elif defined(ESP32)
#define TFT_RST 26  // IO 26
#define TFT_RS  25  // IO 25
#define TFT_CLK 14  // HSPI-SCK
//#define TFT_SDO 12  // HSPI-MISO
#define TFT_SDI 13  // HSPI-MOSI
#define TFT_CS  15  // HSPI-SS0
#define TFT_LED 0 // 0 if wired to +5V directly
SPIClass hspi(HSPI);
#else
#define TFT_RST 8
#define TFT_RS  9
#define TFT_CS  10  // SS
#define TFT_SDI 11  // MOSI
#define TFT_CLK 13  // SCK
#define TFT_LED 3   // 0 if wired to +5V directly
#endif

#define TFT_BRIGHTNESS 200 // Initial brightness of TFT backlight (optional)

// Use hardware SPI (faster - on Uno: 13-SCK, 12-MISO, 11-MOSI)
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);
// Use software SPI (slower)
//TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_SDI, TFT_CLK, TFT_LED, TFT_BRIGHTNESS);

// Variables and constants
int16_t x=0, y=0, w, h;
//DimmableLight light(SPEED);
volatile bool STATE_ON_OFF = false;
volatile bool STATE_ION = false;
volatile unsigned char STATE_MODE = 2;
volatile unsigned char STATE_FAN = 1;
volatile unsigned long TIME_ION = 0;
float dust = 0;
float prevdust = 0;

// Hàm ngắt nút nhấn bật/tắt hệ thống. 

// Your WiFi credentials.
// Set password to "" for open networks.
char auth[] = "SbZVmb-aI2WE9KmsgA3o2DO1Gswrruqq";
char ssid[] = "NguyenHuy";
char pass[] = "12345678";

BlynkTimer timer;
WidgetBridge bridge1(V10);


BLYNK_CONNECTED() {
  // Request the latest state from the server
  Blynk.syncAll();
  bridge1.setAuthToken("uB4Xc65pdQhlTQynFAQfvfeBxJdU03Od"); // Token of the hardware B 
}

BLYNK_WRITE(V2){
  STATE_ON_OFF = param.asInt(); // assigning incoming value from pin V1 to a variable
  // process received value
}
BLYNK_WRITE(V3) {
  STATE_MODE = param.asInt();
}
BLYNK_WRITE(V4) {
  STATE_FAN = param.asInt();
}
BLYNK_WRITE(V5) {
  STATE_ION = param.asInt();
}

void blynkAnotherDevice() // Here we will send HIGH or LOW once per second
{
    bridge1.virtualWrite(V5, STATE_FAN);
    bridge1.virtualWrite(V6, STATE_ION);
    Blynk.virtualWrite(V1, dust);
    Blynk.virtualWrite(V2, STATE_ON_OFF);
    Blynk.virtualWrite(V3, STATE_MODE);
    Blynk.virtualWrite(V4, STATE_FAN);
    Blynk.virtualWrite(V5, STATE_ION);
 }

 void IRAM_ATTR _ON_OFF() { 
  static unsigned long last_interrupt_time = 0;
 unsigned long interrupt_time = millis();
 // If interrupts come faster than 220ms, assume it's a bounce and ignore
 if (interrupt_time - last_interrupt_time > 220)
 {
    STATE_ON_OFF = !STATE_ON_OFF;
    Serial.printf("STATE_ON_OFF = %u\n", STATE_ON_OFF);  
 }
  last_interrupt_time = interrupt_time ;  
}

// Hàm ngắt của nút nhấn bật/tắt Ion.  
void IRAM_ATTR _ION() { 
  static unsigned long last_interrupt_time = 0;
 unsigned long interrupt_time = millis();
 // If interrupts come faster than 220ms, assume it's a bounce and ignore
 if (interrupt_time - last_interrupt_time > 220)
 {
   if(STATE_ON_OFF &&(STATE_MODE==2)) STATE_ION = !STATE_ION;
  Serial.printf("STATE_ION = %u\n", STATE_ION);
 }
  last_interrupt_time = interrupt_time ;
}

/* Hàm ngắt của nút nhấn chuyển đổi giữa các chế độ.
 Mặc định:  
 STATE_MODE = 1: Chế độ Auto
 STATE_MODE = 2: Chế độ Manual 
 STATE_MODE = 3: Chế độ Sleep 
 */
void IRAM_ATTR _MODE() { 
  static unsigned long last_interrupt_time = 0;
 unsigned long interrupt_time = millis();
 // If interrupts come faster than 220ms, assume it's a bounce and ignore
 if (interrupt_time - last_interrupt_time > 220)
 {
  if(STATE_ON_OFF)
  {
    if(STATE_MODE < 3)  STATE_MODE++;
    else STATE_MODE = 1;  
  }  
  Serial.printf("STATE_MODE = %u\n", STATE_MODE);
 }
  last_interrupt_time = interrupt_time ; 
}

/* Hàm ngắt của nút nhấn chuyển đổi tốc độ quạt
 Ứng với STATE_FAN = 1: chế độ quạt LOW
 Ứng với STATE_FAN = 2: chế độ quạt MEDIUM
 Ứng với STATE_FAN = 3: chế độ quạt HIGH
 */
void IRAM_ATTR _FAN() { 
  static unsigned long last_interrupt_time = 0;
 unsigned long interrupt_time = millis();
 // If interrupts come faster than 220ms, assume it's a bounce and ignore
 if (interrupt_time - last_interrupt_time > 220)
 {
  if(STATE_ON_OFF && (STATE_MODE == 2))
  {
    if(STATE_FAN < 4) STATE_FAN++;
    else STATE_FAN = 1;      
  }
    Serial.printf("STATE_FAN = %u\n", STATE_FAN); 
 }
  last_interrupt_time = interrupt_time ;  
}
void _basicDisplay() {
  tft.setOrientation(1);
  tft.setFont(Terminal6x8);
  tft.setBackgroundColor(COLOR_BLACK);
  tft.drawText(80, 3, "* Project *", COLOR_CYAN);
  tft.drawLine(15,12,205,12, COLOR_BLUE);
  tft.drawText(80, 166, "<Air Clean>", COLOR_CYAN);
  tft.drawLine(15,162,205,162, COLOR_BLUE);
  }
  
  void _pm25Display()
  {
  String s1 = "PM2.5:  ";
  String s2 = "";
  s2+=dust;
  tft.setGFXFont(&FreeSans12pt7b); // Set current font
  tft.fillRectangle(85, 18, 180, 45, COLOR_BLACK);
  tft.setFont(Terminal6x8);
  tft.drawGFXText(0, 40, s1, COLOR_WHITE); // Print string

  if ( dust<50) //ti le bui thap
  {
    if(dust!=prevdust)
    {
      tft.fillRectangle(85, 18, 180, 45, COLOR_BLACK);
      tft.drawGFXText(90, 40, s2, COLOR_GREEN);
      prevdust=dust;
    } else tft.drawGFXText(90, 40, s2, COLOR_GREEN);
  }

  if (dust >= 50 && dust < 100) //ti le bui trung binh
  {
    if(dust!=prevdust)
    {
      tft.fillRectangle(85, 18, 180, 45, COLOR_BLACK);
      tft.drawGFXText(90, 40, s2, COLOR_ORANGE);
      prevdust=dust;
    } else tft.drawGFXText(90, 40, s2, COLOR_ORANGE);
  }

    if (dust >= 100) //ti le bui cao
  {
    if(dust!=prevdust)
    {
      tft.fillRectangle(85, 18, 180, 45, COLOR_BLACK);
      tft.drawGFXText(90, 40, s2, COLOR_RED);
      prevdust=dust;
    } else tft.drawGFXText(90, 40, s2, COLOR_RED);
  }
    }
    
void _modeDisplay(){
  String s3 = "Mode :";
  tft.drawGFXText(0, 75, s3, COLOR_WHITE);
     if(STATE_MODE==1)  //Che do Auto
     {
       tft.fillRectangle(80, 45, 160, 80, COLOR_BLACK);
       tft.drawCircle(122, 65, 14, COLOR_GREEN);
       //tft.setFont(Terminal6x8); 
       String s3 = "A";
       tft.drawGFXText(115, 73, s3, COLOR_GREEN);
     }

     if(STATE_MODE==2)  //Che do Manual
     {
       tft.fillRectangle(80, 45, 160, 80, COLOR_BLACK);
       tft.drawCircle(122, 65, 14, COLOR_BLUE);
       //tft.setFont(Terminal6x8); 
       String s3 = "M";
       tft.drawGFXText(115, 73, s3, COLOR_BLUE);
     }

     if(STATE_MODE==3)  //Che do Sleep
     {
       tft.fillRectangle(80, 45, 160, 80, COLOR_BLACK);
       String s4 = "zZz";
       tft.drawGFXText(100, 73, s4, COLOR_GRAY);
     }
  }
  
  void _fanDisplay()
  {
     String s4 = "Fan    :";
    tft.drawGFXText(0, 113, s4, COLOR_WHITE);
    if(STATE_FAN==1)
    {
      tft.fillRectangle(95, 91, 120, 113, COLOR_BLACK);//0 fan
    }
    if(STATE_FAN==2)//Quat chay toc do thap
    {
      tft.fillRectangle(95, 91, 120, 113, COLOR_BLACK);//0 fan
      tft.fillRectangle(95, 101, 100, 113, COLOR_GREEN);//1 fan
    }

    if(STATE_FAN==3)//Quat chay toc do thap
    {
      tft.fillRectangle(95, 91, 120, 113, COLOR_BLACK);//0 fan
      tft.fillRectangle(95, 101, 100, 113, COLOR_GREEN);//1 fan
      tft.fillRectangle(105, 96, 110, 113, COLOR_GREEN); //2fan
    }

    if(STATE_FAN==4)//Quat chay toc do thap
    {
        tft.fillRectangle(95, 91, 120, 113, COLOR_BLACK);//0 fan
        tft.fillRectangle(95, 101, 100, 113, COLOR_GREEN);//1 fan
        tft.fillRectangle(105, 96, 110, 113, COLOR_GREEN); //2fan
        tft.fillRectangle(115, 91, 120, 113, COLOR_GREEN); //3fan
    }
  }
    
void _ionDisplay()
  {
   String s5 = "I-On   :";
    tft.drawGFXText(0, 148, s5, COLOR_WHITE);
    if(STATE_ION == true)    // On
    {
      tft.fillRectangle(95, 135, 115, 150, COLOR_GREEN);
      tft.fillRectangle(115, 135, 125, 150, COLOR_WHITE);
    }

    if(STATE_ION == false)    // Off
    {
      tft.fillRectangle(95, 135, 105, 150, COLOR_WHITE);
      tft.fillRectangle(105, 135, 125, 150, COLOR_GRAY);
    }
  }
  
void setup() {
  hspi.begin();
  tft.begin(hspi);
  tft.clear();
  Serial.begin(9600);
  sds.begin();
  
  Serial.println(sds.queryFirmwareVersion().toString()); // prints firmware version
  Serial.println(sds.setActiveReportingMode().toString()); // ensures sensor is in 'active' reporting mode
  Serial.println(sds.setCustomWorkingPeriod(1).toString()); // sensor sends data every 1 minutes
  
  // Draw first string in big font
    pinMode(BUTTON_ON_OFF, INPUT_PULLUP);
    pinMode(BUTTON_ION, INPUT_PULLUP);
    pinMode(BUTTON_MODE, INPUT_PULLUP);
    pinMode(BUTTON_FAN, INPUT_PULLUP);   
    attachInterrupt(digitalPinToInterrupt(BUTTON_ON_OFF), _ON_OFF, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_ION), _ION, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_MODE), _MODE, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_FAN), _FAN, FALLING);
    //pinMode(ION, OUTPUT);
    
    //DimmableLight::setSyncPin(syncPin);
    // VERY IMPORTANT: Call this method to start internal light routine
    //DimmableLight::begin();
    Blynk.begin(auth,ssid,pass);
    timer.setInterval(100L, blynkAnotherDevice);
//    STATE_ION=false;
//    STATE_MODE=2;
//    STATE_FAN=1;
//    STATE_ON_OFF=false;
}

void loop() {
  PmResult pm = sds.readPm();
  if (pm.isOk()) 
  {
    Serial.print("PM2.5 = ");
    Serial.print(pm.pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm.pm10);
    // if you want to just print the measured values, you can use toString() method as well
    dust=pm.pm25;
  }
  
  if(STATE_ON_OFF == false)
  {
  STATE_ION=false;
  STATE_MODE=2;
  STATE_FAN=1;
  tft.clear();
  }
    
  if(STATE_ON_OFF)
  {    
    if(STATE_MODE == 1) //Che do auto, chay theo nong do bui PM2.5
    {
      if (dust<50)//Nong do bui thap, chay quat o toc do thap
      {
        STATE_FAN=2;
        STATE_ION=false;
      }
      if ((dust >= 50) && (dust < 100))
      {
        STATE_FAN=3;
        STATE_ION=true;
      }
      if (dust >= 100)
      {
        STATE_FAN=4;
        STATE_ION=true;
      }
    }
    if(STATE_MODE == 3)//Che do sleep
    {
      STATE_FAN=2;
      STATE_ION=false;
    }
    _basicDisplay();
    _pm25Display();
    _modeDisplay();
    _fanDisplay();
    _ionDisplay();
  }
    Blynk.run();
    timer.run();
}
