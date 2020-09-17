/**************************************************************************
       Title:   WWVB Clock (Step 5)
      Author:   Bruce E. Hall, w8bh.net
        Date:   17 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Canaduino WWVB Module from Universal-Solder.com
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   This is the step #5 for building a WWVB Clock.
                In this sketch, the timer is synchronized with the 
                incoming radio signal, so that the timer begins
                at the start of each second.
                
                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/


#include <TFT_eSPI.h>
#define TITLE "WWVB Step #5: SIGNAL SYNCHRONIZATION"

#define RADIO_OUT PA12                             // microcontoller pin for radio output
#define ERRBIT 4                                   // value for an error bit
#define MARKER 3                                   // value for a marker bit
#define NOBIT  2                                   // value for no bit
#define HIBIT  1                                   // value for bit '1'
#define LOBIT  0                                   // value for bit '0'

TFT_eSPI tft = TFT_eSPI();                         // display object            
TIM_TypeDef *instance = TIM3;                  
HardwareTimer *timer=new HardwareTimer(instance);  // timer object

volatile byte sampleCounter = 100;                 // remaining samples in current second
volatile byte newBit = NOBIT;                      // decoded bit
volatile byte pulseWidth = 0;                      // width (in 10mS units) of radio pulse
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
  pulseWidth += !digitalRead(RADIO_OUT);           // if output line low, add to pulse width
  sampleCounter--;                                 // count down remaining samples
  if (!sampleCounter)                              // full second of sampling?
   {                                               // if so,
    if ((pulseWidth>63)&&(pulseWidth<90))          // long pulses are markers
      newBit = MARKER;
    else if ((pulseWidth>33)&&(pulseWidth<60))     // intermediate pulses are bit=1
      newBit = HIBIT;
    else if ((pulseWidth>5)&&(pulseWidth<30))      // short pulses are bit=0
      newBit = LOBIT;
    else newBit = ERRBIT;
    sampleCounter = 100;                           // reset sampleCounter 
    pulseWidth = 0;                                // start new pulse
  }  
}

void sync() {
  int pw;                                          // pulse width, in milliseconds
  timer->pause();                                  // stop the timer
  do                                               // listen to radio  
    pw = pulseIn(RADIO_OUT,LOW,2000000)/1000;      // get width of low-going pulse in mS    
  while ((pw<650)||(pw>900));                      // and wait for a Mark
  delay(200);                                      // mark is done; wait to start new bit
  sampleCounter = 100;                             // reset sampleCounter for 1 second
  timer->setCount(0);                              // when timer resumes, do full 10mS
  timer->resume();                                 // restart sampling every 10mS                           
}

void setup() {
  tft.init();
  tft.setRotation(1);                               // portrait screen orientation
  tft.fillScreen(TFT_BLACK);                        // start with blank screen
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(TITLE,10,10,2);                    // show title at top of screen
  tft.drawString("Waiting...",50,50,4);             // waiting for sync; could be a while!
  initTimer();                                      // start the counter 
  sync();                                           // synchonize with receiver bits
}

void loop() {
  if (newBit!=NOBIT) {                              // bit received?      
    int x=50, y=50;                                 // screen position       
    tft.fillRect(x,y,200,20,TFT_BLACK);             // erase previous value
    displayCount++;
    x+=tft.drawNumber(displayCount,x,y,4);          // count the bits
    x+=tft.drawString(": ",x,y,4);
    switch (newBit) {                               // put decoded bit on screen
       case LOBIT: {tft.drawString("Bit 0",x,y,4);
          break; } 
       case HIBIT: {tft.drawString("Bit 1",x,y,4);
          break; }
       case MARKER: {tft.drawString("Mark",x,y,4);
          break; }
       case ERRBIT: tft.drawString("Err",x,y,4);
    } 
    newBit = NOBIT;                                 // wait for next bit         
  }
}
