
#include <Arduino.h>
#include "hal_rtc.h"
#include <Wire.h>
#include <TimeLib.h>

#define I2C_SDA 2
#define I2C_SCL 3


void setup()
{
  Serial.begin(115200);
  bool i2cResult = Wire.begin(I2C_SDA, I2C_SCL);
  delay(5000);
  
  Serial.printf("I2C Result: %d\n", i2cResult);
  initRTC();
  Serial.println("initRTC called");
}

void loop()
{
  printDateTime(readRTC());
  // TODO
  delay(1000);
}
