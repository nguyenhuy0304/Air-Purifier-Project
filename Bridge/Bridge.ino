/* Comment this out to disable prints and save space */
//NODEMCU, com9
#define BLYNK_PRINT Serial
#include "dimmable_light.h"
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif

#define ION D8
#define syncPin D5 //Chân zero đọc điểm 0 điện áp
#define SPEED D6
#define lowSpeed 120
#define mediumSpeed 150
#define highSpeed 200

DimmableLight light(SPEED);

char auth[] = "uB4Xc65pdQhlTQynFAQfvfeBxJdU03Od";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "NguyenHuy";
char pass[] = "12345678";

//BlynkTimer timer;
//WidgetBridge bridge1(V8);
//BLYNK_CONNECTED() {
//  bridge1.setAuthToken("SbZVmb-aI2WE9KmsgA3o2DO1Gswrruqq"); // Token of the hardware B
//  Blynk.syncAll();
//}
BLYNK_WRITE(V5) {
  int STATE_FAN = param.asInt();
  if(STATE_FAN==1)  //Quat tat
  {
    light.setBrightness(0);
    Serial.println("Quat tat");
  }
  if(STATE_FAN==2)//Quat chay toc do thap
  {
    light.setBrightness(lowSpeed); 
    Serial.println("Quat chay toc do thap"); 
  }
  if(STATE_FAN==3)//Quat chay toc do trung binh
  {
    light.setBrightness(mediumSpeed);
    Serial.println("Quat chay toc do trung binh"); 
  }
  if(STATE_FAN==4)//Quat chay toc do cao
  {
    light.setBrightness(highSpeed);
    Serial.println("Quat chay toc do cao"); 
  }
}
BLYNK_WRITE(V6) {
  int STATE_ION = param.asInt();
  digitalWrite(ION, STATE_ION);
}

//void blynkAnotherDevice() // Here we will send HIGH or LOW once per second
//{
//  Blynk.syncAll();
// }
// Timer for blynking


void setup()
{
  // Debug console
  Serial.begin(9600);
  Blynk.begin(auth,ssid,pass);
 // timer.setInterval(100L, blynkAnotherDevice);
  DimmableLight::setSyncPin(syncPin);
  //VERY IMPORTANT: Call this method to start internal light routine
  DimmableLight::begin();
  pinMode(ION, OUTPUT);
  digitalWrite(ION, LOW);
}


void loop()
{
  //timer.run();
  Blynk.run();
}
