// Fidget Spinner counter and tachometer
// 1.09.2017 Pawel A. Hernik
// source code for the video:
// https://youtu.be/hTvJCCtt__8

/*
 Parts:
 - Hall sensor 3144
 - Neodymium magnet for fidget spinner
 - MAX7219 LED Matrix
 - Arduino Pro Mini/Nano
 - button
 
 3144 pinout:
 1 - VCC
 2 - GND
 3 - DATA (0 when close to magnet)
*/

#include <Arduino.h>

#define NUM_MAX 4
#define ROTATE  90

#define DIN_PIN 12 
#define CS_PIN  11
#define CLK_PIN 10

#include "max7219.h"
#include "fonts.h"

const int hallPin = 2; // pin 2 = int 0
const int buttonPin = 9;

// =======================================================================

volatile unsigned long cntTime=0;
volatile unsigned long cnt=0;

void doCount() // interrupt callback should be as short as possible!
{
  if(digitalRead(hallPin) == LOW)
  {
    cnt++;
    cntTime = millis();
  }
}
// =======================================================================
int oldState = HIGH;
long debounce = 30;
long b1Debounce = 0;
long b1LongPress = 0;

int checkButton()
{
  if (millis() - b1Debounce < debounce)
    return 0;
  int state = digitalRead(buttonPin);
  if (state == oldState) {
    if(state == LOW && (millis() - b1LongPress > 500) ) return -1;
    return 0;
  }
  oldState = state;
  b1Debounce = millis();
  if(state == LOW) b1LongPress = millis();
  return state == LOW ? 1 : 0;
}
// =======================================================================
void setup() 
{
  Serial.begin(9200);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 0);
  clr();
  refreshAll();
  digitalWrite(hallPin,HIGH);
  attachInterrupt(digitalPinToInterrupt(hallPin), doCount, FALLING);  // hall pin on interrupt 0 = pin 2
  pinMode(hallPin,INPUT_PULLUP);
  pinMode(buttonPin,INPUT_PULLUP);
  cntTime = millis();
}
// =======================================================================
unsigned long rpm=0,maxrpm=0;
int dispRotTime=0, rotTime=0;
unsigned long measureTime=0,curTime,startTime=0;
int dispCnt=0,measureCnt=0;
char txt[10];
const int resetTime = 2000;
const int minRotNum = 1;  // 1 - calc after every rotation
int mode = 0;

void loop()
{
  curTime = millis();
  if(curTime-cntTime>resetTime) { // reset when less than 30RPM (dt>2s)
    cnt = measureCnt = 0;
    rpm = 0;
  }
  if(cnt==1) startTime = cntTime;
  if(cnt-measureCnt>=minRotNum) {
    rpm = 60000L*(cnt-measureCnt)/(cntTime-measureTime);
    measureCnt = cnt;
    measureTime = cntTime;
  }
  rotTime = (cntTime-startTime)/1000; // time in seconds
  if(cnt>1 || !dispRotTime) {  // keep previous time on the OLED until new rotation starts
    dispRotTime = rotTime;
    dispCnt = cnt;
  }
  if(rpm>maxrpm) maxrpm=rpm;

  int modeBut = checkButton();
  if(modeBut < 0) mode = 0;
  if(modeBut > 0 ) if(++mode > 3) mode = 0;
  // dispCnt     - number of revolutions
  // rpm         - current RPM
  // maxrpm      - max RPM
  // dispRotTime - time
  clr();
  if(mode==0) showBigRPM(); else
  if(mode==1) showRPM_Max(); else
  if(mode==2) showCnt_RPM(); else
  if(mode==3) showCnt_Time();
  refreshAll();
}
// =======================================================================
void showCnt_Time()
{
  setCol(0,0x10);
  sprintf(txt,"% 4u",dispCnt);
  for(int i=0; i<4;i++) if(txt[i]!=' ') showDigit(txt[i]-0x20, 1+4*i, small3x7);
  sprintf(txt,"% 4u",dispRotTime);
  for(int i=0; i<4;i++) if(txt[i]!=' ') showDigit(txt[i]-0x20, 17+4*i, small3x7);
}
// =======================================================================
void showCnt_RPM()
{
  setCol(0,0x20);
  sprintf(txt,"% 4u",dispCnt);
  for(int i=0; i<4;i++) if(txt[i]!=' ') showDigit(txt[i]-0x20, 17+4*i, small3x7);
  sprintf(txt,"% 4u",rpm);
  for(int i=0; i<4;i++) if(txt[i]!=' ') showDigit(txt[i]-0x20, 1+4*i, small3x7);
}
// =======================================================================
void showRPM_Max()
{
  setCol(0,0x40);
  sprintf(txt,"% 4u",rpm);
  for(int i=0; i<4;i++) if(txt[i]!=' ') showDigit(txt[i]-0x20, 1+4*i, small3x7);
  sprintf(txt,"% 4u",maxrpm);
  for(int i=0; i<4;i++) if(txt[i]!=' ') showDigit(txt[i]-0x20, 17+4*i, small3x7);
}
// =======================================================================
void showBigRPM()
{
  setCol(0,0x80);
  sprintf(txt,"% 4u",rpm);
  for(int i=0; i<4;i++) if(txt[i]!=' ') showDigit(txt[i]-0x20, 2+6*i, dig5x8rn);
}
// =======================================================================
int dx=0; // actually not used here
int dy=0;
void showDigit(char ch, int col, const uint8_t *data)
{
  if (dy < -8 | dy > 8) return;
  int len = pgm_read_byte(data);
  int w = pgm_read_byte(data + 1 + ch * len);
  col += dx;
  for (int i = 0; i < w; i++)
    if (col + i >= 0 && col + i < 8 * NUM_MAX) {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if (!dy) scr[col + i] = v; else scr[col + i] |= dy > 0 ? v >> dy : v << -dy;
    }
}

// =======================================================================

void setCol(int col, byte v)
{
  if (dy < -8 | dy > 8) return;
  col += dx;
  if (col >= 0 && col < 8 * NUM_MAX)
      if (!dy) scr[col] = v; else scr[col] |= dy > 0 ? v >> dy : v << -dy;
}

// =======================================================================


