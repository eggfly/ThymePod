/***************************************************
  This is an example for the Adafruit VS1053 Codec Breakout

  Designed specifically to work with the Adafruit VS1053 Codec Breakout
  ----> https://www.adafruit.com/products/1381

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

// define the pins used
#define CLK 40  // SPI Clock, shared with SD card
#define MISO 38 // Input data, from VS1053/SD card
#define MOSI 45 // Output data, to VS1053/SD card
// Connect CLK, MISO and MOSI to hardware SPI pins.
// See http://arduino.cc/en/Reference/SPI "Connections"

// These are the pins used for the breakout example
#define BREAKOUT_RESET 21 // VS1053 reset pin (output)
#define BREAKOUT_CS 47    // VS1053 chip select pin (output)
#define BREAKOUT_DCS 42   // VS1053 Data/command select pin (output)
// These are the pins used for the music maker shield
// #define SHIELD_RESET -1 // VS1053 reset pin (unused!)
// #define SHIELD_CS 7     // VS1053 chip select pin (output)
// #define SHIELD_DCS 6    // VS1053 Data/command select pin (output)

// #define VS1053_SCLK
// #define VS1053_MOSI
// #define VS1053_MISO
#define VS1053_SDCS 39

// These are common pins between breakout and shield
// #define CARDCS 4 // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 41 // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer =
    // create breakout-example object!
    Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, VS1053_SDCS);
// create shield-example object!
// Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

void printDirectory(File dir, int numTabs);

void setup_screen();
void loop_screen();

void setup()
{
    pinMode(BREAKOUT_DCS, OUTPUT);
    digitalWrite(BREAKOUT_DCS, HIGH);
    // 上面其实需要硬件拉低, 重新 PCB 设计的时候不要忘了
    Serial.begin(115200);
    setup_screen();

    SPI.begin(CLK, MISO, MOSI, -1);
    Serial.println("Adafruit VS1053 Simple Test");

    if (!SD.begin(VS1053_SDCS, SPI))
    {
        Serial.println(F("SD failed, or not present"));
        while (1)
            ; // don't do anything more
    }
    // list files
    printDirectory(SD.open("/"), 0);

    if (!musicPlayer.begin())
    { // initialise the music player
        Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
        while (1)
            ;
    }
    Serial.println(F("VS1053 found"));

    // Set volume for left, right channels. lower numbers == louder volume!
    // 100声音小，70声音大，越小越大
    musicPlayer.setVolume(60, 60);
    // Timer interrupts are not suggested, better to use DREQ interrupt!
    // musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int

    // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
    // audio playing
    musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int

    // Play one file, don't return until complete
    // Serial.println(F("Playing ygzjhyl.mp3"));
    // musicPlayer.playFullFile("/ygzjhyl.mp3");
    // Play another file in the background, REQUIRES interrupts!
    Serial.println(F("Start Playing..."));
    musicPlayer.startPlayingFile("/server/康姆士乐团-你要如何，我们就如何.mp3");
}

void loop()
{
    loop_screen();
    // File is playing in the background
    if (musicPlayer.stopped())
    {
        Serial.println("Done playing music");
        while (1)
        {
            delay(10); // we're done! do nothing...
        }
    }
    auto decodeTime = musicPlayer.decodeTime();
    Serial.printf("Decode time: %d\n", decodeTime);
    if (Serial.available())
    {
        char c = Serial.read();

        // if we get an 's' on the serial console, stop!
        if (c == 's')
        {
            musicPlayer.stopPlaying();
        }

        // if we get an 'p' on the serial console, pause/unpause!
        if (c == 'p')
        {
            if (!musicPlayer.paused())
            {
                Serial.println("Paused");
                musicPlayer.pausePlaying(true);
            }
            else
            {
                Serial.println("Resumed");
                musicPlayer.pausePlaying(false);
            }
        }
    }

    delay(1);
}

/// File listing helper
void printDirectory(File dir, int numTabs)
{
    while (true)
    {

        File entry = dir.openNextFile();
        if (!entry)
        {
            // no more files
            // Serial.println("**nomorefiles**");
            break;
        }
        for (uint8_t i = 0; i < numTabs; i++)
        {
            Serial.print('\t');
        }
        Serial.print(entry.name());
        if (entry.isDirectory())
        {
            Serial.println("/");
            printDirectory(entry, numTabs + 1);
        }
        else
        {
            // files have sizes, directories do not
            Serial.print("\t\t");
            Serial.println(entry.size(), DEC);
        }
        entry.close();
    }
}
