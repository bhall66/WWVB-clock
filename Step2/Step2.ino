/**************************************************************************
       Title:   WWVB Clock (Step 2)
      Author:   Bruce E. Hall, w8bh.net
        Date:   17 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Canaduino WWVB Module from Universal-Solder.com
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   This is step 2 for building a WWVB Clock.
                In this sketch, pulses from the Canaduino module are
                converted to data (bits) and displayed.
                
                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>

#define RADIO_OUT          PA12
#define TITLE "WWVB Step #2: DECODING BITS"

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
     tft.fillRect(50,50,140,20,TFT_BLACK);             // erase previous value
     tft.drawNumber(pulseWidth,50,50,4);               // show pulse width on screen
     if (pulseWidth<300)                               // bit 0?
       tft.drawString("'0'",100,50,4);                  
     else if ((pulseWidth>330) && (pulseWidth<600))    // bit 1?
       tft.drawString("'1'",100,50,4);
     else if ((pulseWidth>630) && (pulseWidth<900))    // marker?
       tft.drawString("Mrk",100,50,4);
     else tft.drawString("Err",100,50,4);              // none of the above
  }
}
