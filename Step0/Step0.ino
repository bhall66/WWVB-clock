/**************************************************************************
       Title:   WWVB Clock (Step 0)
      Author:   Bruce E. Hall, w8bh.net
        Date:   17 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Canaduino WWVB Module from Universal-Solder.com
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   "Hello World"  Use this sketch to verify your hardware
                connections, your ability to upload sketches, and that you
                have a working display module.
                
                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>
#define TITLE "Hello, World!"

TFT_eSPI tft = TFT_eSPI();                         // display object            

void setup() {
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation
  tft.fillScreen(TFT_BLUE);                        // start with empty screen
  tft.setTextColor(TFT_YELLOW);                    // yellow on blue text
  tft.drawString(TITLE,50,50,4);                   // display text
}

void loop() {
}
