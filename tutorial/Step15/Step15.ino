/**************************************************************************
       Title:   WWVB Clock (Step 15)
      Author:   Bruce E. Hall, w8bh.net
        Date:   17 Sep 2020
    Hardware:   Blue Pill Microcontroller, 2.8" ILI9341 TFT display,
                Canaduino WWVB Module from Universal-Solder.com
    Software:   Arduino IDE 1.8.13; STM32 from github.com/SMT32duino
                TFT_eSPI and TimeLib libraries (install from IDE)
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   This is the step #15 for building a WWVB Clock.
                In this sketch, the displayed time now includes
                AM/PM and time zone indicators: "11:34:56 AM EST"
                
                See w8bh.net for a detailed, step-by-step tutorial

 **************************************************************************/

 
#include <TFT_eSPI.h>
#include <TimeLib.h>
#define TITLE "WWVB Test #15: AM/PM & Time Zone"

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
#define SYNCTIMEOUT 300                            // seconds until sync gives up

#define TIMECOLOR TFT_CYAN
#define DATECOLOR TFT_YELLOW
#define ZONECOLOR TFT_GREEN
#define LABEL_FGCOLOR TFT_YELLOW
#define LABEL_BGCOLOR TFT_BLUE

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
bool doingBits = false;                            // start with time display, not bits
bool useAMPM = true;                               // 12 vs 24 hour time
int  tz = LOCALTIMEZONE;                           // current time zone
time_t t = 0;                                      // time that display last updated
time_t goodTime = 0;                               // time of last receiver decode
byte dst = 0;                                      // daylight saving time flag


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
  int x=20,y=65,h=0,m=0,f=7;                       // time is 00:00; font 7
  tft.setTextColor(TIMECOLOR, TFT_BLACK);          // set time color
  if (timeStatus()==timeSet) {                     // if time is valid then
    h=hour(), m=minute();                          // get hours and minutes
  } 
  if (useAMPM && (h>12)) h-=12;                    // adjust hr for 12hr display  
  if (h<10) x+= tft.drawChar('0',x,y,f);           // leading zero for hour
  x+= tft.drawNumber(h,x,y,f);                     // show hours
  x+= tft.drawChar(':',x,y,f);                     // hour:min separator
  if (m<10) x+= tft.drawChar('0',x,y,f);           // leading zero for minutes
  tft.drawNumber(m,x,y,f);                         // show minutes
}

void showSeconds() {
  int x=162,y=65,f=7;                              // screen position & font                              
  int s=second();                                  // get current seconds
  tft.setTextColor(TIMECOLOR, TFT_BLACK);          // set time color          
  x += tft.drawChar(':',x,y,f);                    // show ":"
  if (s<10) x+= tft.drawChar('0',x,y,f);           // add leading zero if needed
  x+= tft.drawNumber(s,x,y,f);                     // show seconds
}

void showDate() {
  int x=20,y=130,f=4;
  const char* days[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
  tft.setTextColor(DATECOLOR, TFT_BLACK);
  if (timeStatus()!=timeSet)                       // has date been set yet?
    tft.drawString("Waiting for Signal",x,y,f);    // no, inform user
  else {                                           // yes, date has been set
    int mo=month(), dy=day(), yr=year();           // get date components  
    int dw = weekday(t);                           // get day of the week
    tft.fillRect(x,y,295,26,TFT_BLACK);            // erase previous date  
    x+=tft.drawString(days[dw-1],x,y,f);           // show day of week
    x+=tft.drawString(", ",x,y,f);                 // and     
    x+=tft.drawNumber(mo,x,y,f);                   // show date as month/day/year
    x+=tft.drawChar('/',x,y,f);
    x+=tft.drawNumber(dy,x,y,f);
    x+=tft.drawChar('/',x,y,f);
    x+=tft.drawNumber(yr,x,y,f);
  }
}

void showAMPM () {
  int x=250,y=90,ft=4;
  int hr=hour(t);
  tft.setTextColor(TIMECOLOR,TFT_BLACK); 
  if (!useAMPM) tft.fillRect(x,y,50,20,TFT_BLACK);
  else if (hr<12) tft.drawString("AM",x,y,ft);
  else tft.drawString("PM",x,y,ft);
}

void showTimeZone () {
  int x=250,y=50,ft=4;
  char c1,c2;
  tft.setTextColor(ZONECOLOR,TFT_BLACK); 
  if (tz==UTC)
    tft.drawString("UTC",x,y,ft);
  else {
    switch(tz) {
      case EST: c1='E'; break;
      case CST: c1='C'; break;
      case MST: c1='M'; break;
      case PST: c1='P'; break;
      default:  c1='?';      
    }
    if (dst) c2='D'; else c2='S';
    x+=tft.drawChar(c1,x,y,ft);
    x+=tft.drawChar(c2,x,y,ft);
    x+=tft.drawChar('T',x,y,ft);
    x+=tft.drawChar(' ',x,y,ft);
  }
}

void eraseBits() {
  tft.fillRect(20,50,290,130,TFT_BLACK);
}

void startNewFrame()
{
  frameIndex = 0;                  
  if (doingBits) eraseBits();                      // erase bits on screen
}

void showBit(int frameIndex, int bitType) {        // display a bit on screen
  const int x=100,y=50,w=15,h=15;                  // screen position & bit size
  int color; 
  int xpos = 20;
  if (!doingBits) return;                          // if in 'bit' mode, continue
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

int timeSinceDecode() {                            // return minutes since time decoded
  return (t-goodTime)/60;               
}

void showClockStatus () {
  int color,x=20,y=200,w=80,h=20,ft=2;             // screen position and size
  char st[20];                                     // string buffer
  if (!goodTime) return;                           // haven't decoded time yet
  int minPassed = timeSinceDecode();               // how long ago was last decode? 
  itoa(minPassed,st,10);                           // convert number to a string
  strcat(st," min.");                              // like this: "10 min."
  tft.setTextColor(TFT_BLACK);                    
  tft.fillRect(x,y,w,h,TFT_BLACK);                 // erase previous status
  if (minPassed<60) color=TFT_GREEN;               // green is < 1 hr old
  else if (minPassed<720) color=TFT_ORANGE;        // orange is 1-12 hr old                 
  else color=TFT_RED;                              // red is >12 hr old
  tft.fillRoundRect(x,y,80,20,5,color);            // show status indicator
  tft.drawString(st,x+10,y+2,ft);                  // and time since last good
}

void newScreen() {
  tft.fillScreen(TFT_BLACK);                       // start with empty screen
  tft.fillRoundRect(2,6,316,32,10,LABEL_BGCOLOR);  // put title bar at top
  tft.drawRoundRect(2,6,316,234,10,TFT_WHITE);     // draw edge around screen
  tft.setTextColor(LABEL_FGCOLOR,LABEL_BGCOLOR);   // set label colors
  tft.drawString(TITLE,50,12,2);                   // show title at top
  tft.drawString("  Status  ",20,180,2);           // label for clock status
  tft.drawString("  Current Minute  ",160,180,2);  // label for segment status
}

void getRadioTime()                                // decode time from current frame
{ 
   const byte daysInMonth[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
                               //JanFebMarAprMayJunJulAugSepOctNovDec   
   const int century=2000;
   int yr,mo,dy,hr,mn,leap;
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
   adjustTime(3600*tz);                            // adjust for time zone (EST=-5, GMT=0);
   if (dst) adjustTime(3600);                      // adjust 1 hour forward daylight savings
   goodTime = now();                               // remember when time was decoded.
}

bool validFrame() {                                // evaluate data in current frame
  for (int i=0; i<6; i++)                          // look at each segment
    if (seg[i]<1) return false;                    // and return false if any are bad
  return true;                                     
}

void clearSegments() {
  for (int i=0; i<6; i++)                          // and wipe segment data
     seg[i] = -1; 
}

bool validSegment() {                              // true if current segment is good
  bool goodData = true;                            // assume everything is good :)
  byte start = frameIndex-9;                       // start at beginning of segment
  for (int i=start; i<start+8; i++)                // look at all bits in segment
    if (frame[i]==ERRBIT) goodData = false;        // if error found, segment is bad
  return goodData && (newBit==MARKER);             // seg must also end on marker
}

void checkRadioData() {
  if (newBit!=NOBIT){                              // do nothing until bit received
    if (frameIndex>59) startNewFrame();            // time to start new frame
    if ((newBit==MARKER) && (oldBit==MARKER)) {    // is this the start of a new minute?
      if (validFrame()) getRadioTime();            // decode time from previous frame
      startNewFrame();
    }
    if (frameIndex==0) clearSegments();            // keeps old segs til frame validation
    
    frame[frameIndex] = newBit;                    // save this bit
    showBit(frameIndex,newBit);                    // and show it
    
    if ((frameIndex%10)==9) {                      // are we at end of segment?
     seg[frameIndex/10] = validSegment();          // validate segment & save result
     showSegments();                               // show segment evaluation
    }
    
    frameIndex++;                                  // advance to next bit position
    oldBit = newBit;                               // remember current bit
    newBit = NOBIT;                                // wait until next bit received
  }  
}

void sync() {
  int pw;                                          // pulse width, in milliseconds
  int quitTime = millis()+(SYNCTIMEOUT*1000);      // know when to fold 'em
  timer->pause();                                  // stop the timer
  do {                                             // listen to radio
    updateTimeDisplay();                           // keep time displayed during sync  
    pw = pulseIn(RADIO_OUT,LOW,2000000)/1000;      // get width of low-going pulse in mS    
  } while ((millis()<quitTime)                     // leave if timed out
    && ((pw<650)||(pw>900)));                      // otherwise wait for a Mark
  delay(200);                                      // mark is done; wait to start new bit
  sampleCounter = 100;                             // reset sampleCounter for 1 second
  timer->setCount(0);                              // when timer resumes, do full 10mS
  timer->resume();                                 // restart sampling every 10mS                           
}

void doSync() {                                    // sync with visual status update 
  int c=TFT_WHITE,x=20,y=200,w=80,h=20,f=2;        // screen position & size
  tft.setTextColor(TFT_BLACK,c);
  tft.fillRoundRect(x,y,w,h,5,c);
  tft.drawString("NO SYNC",x+10,y+2,f);            // show sync in progress
  sync();
  tft.fillRect(x,y,w,h,TFT_BLACK);                 // erase sync message
  showClockStatus();                               // restore clock status, if any
}  

bool needSync() {
  int flag = timeSinceDecode()%60;                 // haven't sync'd for a while?
  return flag==30;                                 // once per hour @ 30 min mark
}


void updateTimeDisplay() {
  if (doingBits) return;                           // bit vs. time display
  time_t timeNow = now();                          // check the time now
  if (timeNow!=t) {                                // are we in a new second yet?
    if (minute() != minute(t)) {                   // are we in a new minute?
      if (hour() != hour(t)) {                     // are we in a new hour?   
         showDate();                               // new hour, so update date
         showAMPM();                               // and AM/PM status
         showTimeZone();                          // and dst status
      }
      showTime();                                  // new minute, so update time
      showClockStatus();                           // and status
    }
    showSeconds();                                 // new second, so show it
    t = timeNow;                                   // remember the displayed time
  }  
}

void setup() {
  tft.init();
  tft.setRotation(1);                              // portrait screen orientation
  newScreen();                                     // show title & labels
  initTimer();                                     // start the counter 
  doSync();                                        // synchonize with receiver bits
}

void loop() {
   updateTimeDisplay();                            // keep display current
   if (needSync()) doSync();                       // sync if data is stale
   checkRadioData();                               // collect data & update time
}
