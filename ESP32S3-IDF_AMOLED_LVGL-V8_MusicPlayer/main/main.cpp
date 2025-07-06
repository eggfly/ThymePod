#include <Arduino.h>

#include "Audio.h" //https://github.com/schreibfaul1/ESP32-audioI2S

extern "C"
{
    void lvgl_amoled_init(void);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ; // wait for serial port to connect
    }
    lvgl_amoled_init();
}

void loop()
{
    while (true)
    {
        Serial.println("loop");
        delay(10000);
    }
}
