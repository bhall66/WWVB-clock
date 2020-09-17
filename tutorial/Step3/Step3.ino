/**************************************************************************
       Title:   WWVB Clock (Step 3)
      Author:   Bruce E. Hall, w8bh.net
        Date:   17 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Canaduino WWVB Module from Universal-Solder.com
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   This is step #3 for build a working WWVB Clock.
                In this sketch, a hardware timer generates a 
                100Hz interrupt, which will eventually be used to
                sample the incoming WWVB signal.
                
                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/

#include <TFT_eSPI.h>

#define TITLE "WWVB Step #3: HARDWARE INTERRUPT"

TFT_eSPI tft = TFT_eSPI();

TIM_TypeDef   *instance = TIM3;
HardwareTimer *timer = new HardwareTimer(instance);

volatile byte sampleCounter = 100;                 // remaining samples in current second
volatile bool sampleComplete = false;              // 1 second sample flag
int displayCount = 0;                              // something to show on screen

void initTimer() {                                 // set up 100Hz interrupt
  timer->pause();                                  // pause the timer
  timer->setCount(0);                              // start count at 0
  timer->setOverflow(100,HERTZ_FORMAT);            // set counter for 100Hz overflow rate
  timer->attachInterrupt(timerHandler);            // overflow results in interrupt
  timer->resume();                                 // restart counter with these parameters
}

void timerHandler()                                // called every 10mS
{
  sampleCounter--;                                 // count down remaining samples
  if (!sampleCounter)                              // full second of sampling?
   {                                               // if so,
    sampleComplete = true;                         // flag it, and
    sampleCounter = 100;                           // reset sampleCounter
  }  
}

void setup() {
  tft.init();
  tft.setRotation(1);                               // portrait screen orientation
  tft.fillScreen(TFT_BLACK);                        // start with blank screen
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(TITLE,10,10,2);                    // show title at top of screen
  initTimer();                                      // start the counter 
}

void loop() {
  if (sampleComplete) {                             // has a full second passed?       
    displayCount++;                                 // yes, so increment counter
    tft.fillRect(50,50,140,20,TFT_BLACK);           // erase previous value
    tft.drawNumber(displayCount,50,50,4);           // show count on screen 
    sampleComplete = false;                         // wait until next second is flagged   
  }
}
