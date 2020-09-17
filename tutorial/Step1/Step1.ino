/**************************************************************************
       Title:   WWVB Clock (Step 1)
      Author:   Bruce E. Hall, w8bh.net
        Date:   17 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Canaduino WWVB Module from Universal-Solder.com
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   This is the first step for build a working WWVB Clock.
                In this sketch, pulses from the Canaduino module are
                measured and displayed on the display.
                
                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>

#define RADIO_OUT          PA12
#define TITLE "WWVB Step #1: SHOW PULSE WIDTH"

TFT_eSPI tft = TFT_eSPI();

void setup() {
  tft.init();
  tft.setRotation(1);                                  // portrait screen orientation
  tft.fillScreen(TFT_BLACK);                           // start with blank screen
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(TITLE,10,10,2);                       // show title at top of screen
}

void loop() {
  int pulseWidth = pulseIn(RADIO_OUT,LOW,2000000);     // get width of pulse in uS
  pulseWidth /= 1000;                                  // convert uS to mS
  if (pulseWidth>90) {                                 // ignore noise
     tft.fillRect(50,50,100,20,TFT_BLACK);             // erase previous value
     tft.drawNumber(pulseWidth,50,50,4);               // show pulse width on screen
  }
}
