/**************************************************************************
       Title:   WWVB Clock (Step 10)
      Author:   Bruce E. Hall, w8bh.net
        Date:   17 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Canaduino WWVB Module from Universal-Solder.com
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   This is the step #10 for building a WWVB Clock.
                In this sketch, basic error checking is done on the
                received data, ignoring frames which lack proper marker
                bits.  The data frame is divided into 6 equal segments,
                10 bits each.
                
                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/


#include <TFT_eSPI.h>
#include <TimeLib.h>
#define TITLE "WWVB Step #10: SEGMENTS"

#define UTC  0                                     // Coordinated Universal Time
#define EST -5                                     // Eastern Standard Time
#define CST -6                                     // Central Standard Time
#define MST -7                                     // Mountain Standard Time
#define PST -8                                     // Pacific Standard Time
#define LOCALTIMEZONE EST                          // Set to your own time zone!

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
volatile byte newBit = NOBIT;                      // current bit
volatile byte pulseWidth = 0;                      // width (in 10mS units) of radio pulse

byte oldBit = NOBIT;                               // previous bit
byte frame[60];                                    // space to hold full minute of bits
int seg[6] = {-1,-1,-1,-1,-1,-1};                  // space to hold segment status
int frameIndex = 0;                                // current bit position in frame (0..59)


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

void showTime() {
  int x=20,y=200,h=0,m=0;                          // time is 00:00
  if (timeStatus()==timeSet) {                     // if time is valid then
    h=hour(), m=minute();                          // get hours and minutes
  } 
  if (h<10) x+= tft.drawChar('0',x,y,2);           // leading zero for hour
  x+= tft.drawNumber(h,x,y,2);                     // show hours
  x+= tft.drawChar(':',x,y,2);                     // hour:min separator
  if (m<10) x+= tft.drawChar('0',x,y,2);           // leading zero for minutes
  tft.drawNumber(m,x,y,2);                         // show minutes
}

void showDate() {
  int x=20,y=220;
  if (timeStatus()!=timeSet)                       // has date been set yet?
    tft.drawString("Waiting for Signal",x,y,2);    // no
  else {                                           // yes, date has been set
    int mo=month(), dy=day(), yr=year();           // get date components       
    x+=tft.drawNumber(mo,x,y,2);                   // show date as month/day/year
    x+=tft.drawChar('/',x,y,2);
    x+=tft.drawNumber(dy,x,y,2);
    x+=tft.drawChar('/',x,y,2);
    x+=tft.drawNumber(yr,x,y,2);
  }
}

void startNewFrame()
{
  frameIndex = 0;                    
  tft.fillRect(20,50,290,130,TFT_BLACK);           // clear bits on screen
  tft.fillRect(20,200,140,40,TFT_BLACK);           // clear previous time
  showTime();
  showDate();
}

void showBit(int frameIndex, int bitType) {        // display a bit on screen
  const int x=100,y=50,w=15,h=15;                  // screen position & bit size
  int color; 
  int xpos = 20;
  tft.setTextColor(TFT_YELLOW,TFT_BLACK);
  xpos += tft.drawString("Bit ",xpos,80,4);
  tft.drawNumber(frameIndex,xpos,80,4);            // show the bit number
  switch (bitType)                                 // color code the bit
  {
    case HIBIT:  color = TFT_CYAN; break;
    case LOBIT:  color = TFT_WHITE;  break;
    case MARKER: color = TFT_YELLOW; break;
    case ERRBIT: color = TFT_RED; break;
    default:     color = TFT_BLACK; break;
  }
  tft.fillRoundRect(x+20*(frameIndex%10),          // now draw the bit
      y+20*(frameIndex/10),w,h,4,color);           
}

void showSegments() {
  const int x=160,y=200,w=16,h=20,r=5;             // indicator position & size
  int color;
  for (int i=0; i<6; i++) {                        // for each segment:
    if (seg[i]<0) color = TFT_BLACK;               // unevaluated segments are black
    else if (seg[i]>0) color = TFT_GREEN;          // good segments are green
    else color = TFT_RED;                          // and bad segments are red
    tft.fillRoundRect(x+i*20,y,w,h,r,color);       // display segment indicator
  }
}

void getRadioTime()                                // decode time from current frame
{ 
   const byte daysInMonth[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
                               //JanFebMarAprMayJunJulAugSepOctNovDec   
   const int century=2000;
   int yr,mo,dy,hr,mn,leap,dst;
   leap=dy=hr=dst=leap=mn=0;
   yr=century;
   
   if (frame[1]==HIBIT) mn+=40;                    // decode minutes    
   if (frame[2]==HIBIT) mn+=20;
   if (frame[3]==HIBIT) mn+=10;
   if (frame[5]==HIBIT) mn+=8;
   if (frame[6]==HIBIT) mn+=4;
   if (frame[7]==HIBIT) mn+=2;
   if (frame[8]==HIBIT) mn+=1;

   if (frame[12]==HIBIT) hr+=20;                   // decode hours
   if (frame[13]==HIBIT) hr+=10;
   if (frame[15]==HIBIT) hr+=8; 
   if (frame[16]==HIBIT) hr+=4; 
   if (frame[17]==HIBIT) hr+=2; 
   if (frame[18]==HIBIT) hr+=1; 

   if (frame[22]==HIBIT) dy+=200;                  // decode days
   if (frame[23]==HIBIT) dy+=100;
   if (frame[25]==HIBIT) dy+=80; 
   if (frame[26]==HIBIT) dy+=40; 
   if (frame[27]==HIBIT) dy+=20; 
   if (frame[28]==HIBIT) dy+=10;    
   if (frame[30]==HIBIT) dy+=8; 
   if (frame[31]==HIBIT) dy+=4; 
   if (frame[32]==HIBIT) dy+=2; 
   if (frame[33]==HIBIT) dy+=1;  
   
   if (frame[45]==HIBIT) yr+=80;                   // decode years
   if (frame[46]==HIBIT) yr+=40;
   if (frame[47]==HIBIT) yr+=20;
   if (frame[48]==HIBIT) yr+=10;
   if (frame[50]==HIBIT) yr+=8;
   if (frame[51]==HIBIT) yr+=4;
   if (frame[52]==HIBIT) yr+=2;
   if (frame[53]==HIBIT) yr+=1;
   if (frame[55]==HIBIT) leap+=1;                  // get leapyear indicator
   if (frame[58]==HIBIT) dst+=1;                   // get DST indicator

   mo=1;                                           // convert day of year to month/day
   while (1) {                                     // for each mon, starting with Jan
      byte dim = daysInMonth[mo];                  // get # of days in this month
      if (mo == 2 && leap == 1) dim += 1;          // adjust for leap year, if necessary
      if (dy <= dim) break;                        // have we reached right month yet?
      dy -= dim;  mo += 1;                         // no, subtract all days in this month
   }

   setTime(hr,mn,0,dy,mo,yr);                      // set the arduino time
   adjustTime(61);                                 // adjust for 61 seconds of delay
   adjustTime(3600*LOCALTIMEZONE);                 // adjust for time zone (EST=-5, GMT=0);
   if (dst) adjustTime(3600);                      // adjust 1 hour forward daylight savings
}

bool validFrame() {                                // evaluate data in current frame
  for (int i=0; i<6; i++)                          // look at each segment
    if (seg[i]<1) return false;                    // and return false if any are bad
  return true;                                     
}

void clearSegments() {
  for (int i=0; i<6; i++)                         // and wipe segment data
     seg[i] = -1; 
}

void checkRadioData() {
  if (newBit!=NOBIT){                              // yes! we got a bit
   if (frameIndex>59) startNewFrame();             // time to start new frame
   
   if ((newBit==MARKER) && (oldBit==MARKER)) {     // is this the start of a new minute?
     if (validFrame()) getRadioTime();             // decode time from previous frame
     startNewFrame();
   }
   if (frameIndex==0) clearSegments();             // keep old segs until frame validation
        
   frame[frameIndex] = newBit;                     // save this bit!
   showBit(frameIndex,newBit);                     // yes, so show it

   if ((frameIndex%10)==9) {                       // are we at end of segment?
     seg[frameIndex/10] = (newBit==MARKER);        // seg OK if ends in a Marker bit
     showSegments();                               // show segment evaluation
   }
   
   frameIndex++;                                   // advance to next bit position
   oldBit = newBit;                                // remember current bit
   newBit = NOBIT;                                 // time to get another bit
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
  tft.setRotation(1);                              // portrait screen orientation
  tft.fillScreen(TFT_BLACK);                       // start with blank screen
  tft.setTextColor(TFT_YELLOW,TFT_BLACK);
  tft.drawString(TITLE,10,10,2);                   // show title at top of screen
  initTimer();                                     // start the counter 
  pinMode(PA11,INPUT_PULLUP);
  if (!digitalRead(PA11))                          // if pin PA11 is grounded,
    sync();                                        // synchonize with receiver bits
}

void loop() {
   checkRadioData();                               // collect bits and display them
}
