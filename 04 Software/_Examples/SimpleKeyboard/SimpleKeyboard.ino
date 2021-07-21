#include <U8g2lib.h>
#include <Wire.h>
#include <string.h>

using namespace std;

#define PIN_KEY_LEFT   4
#define PIN_KEY_RIGHT 12
#define PIN_KEY_TOP   14
#define KEY_TRESHOLD  50

U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R2, 22, 21, U8X8_PIN_NONE);  

bool touchLeft  = false;
bool touchRight = false;
bool touchTop   = false;

hw_timer_t * timer = NULL;
portMUX_TYPE keyboardMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onCheckKeys() {
  static volatile int keyLeftCounter  = 0;
  static volatile int keyRightCounter = 0;
  static volatile int keyTopCounter   = 0;
  
  if (touchRead(PIN_KEY_LEFT )<KEY_TRESHOLD) keyLeftCounter++;  else keyLeftCounter =0; 
  if (touchRead(PIN_KEY_RIGHT)<KEY_TRESHOLD) keyRightCounter++; else keyRightCounter=0; 
  if (touchRead(PIN_KEY_TOP  )<KEY_TRESHOLD) keyTopCounter++;   else keyTopCounter  =0; 

  portENTER_CRITICAL_ISR(&keyboardMux);
  if(keyLeftCounter ==3) touchLeft =true;
  if(keyRightCounter==3) touchRight=true;
  if(keyTopCounter  ==3) touchTop  =true;
  portEXIT_CRITICAL_ISR(&keyboardMux); 
}

void setup() {
  // put your setup code here, to run once:
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onCheckKeys, true);
  timerAlarmWrite(timer, 10000, true); // 10
  timerAlarmEnable(timer);

  u8g2.initDisplay();
  delay(500);
  u8g2.begin();  
}

void loop() {
  static int L,R,T;
  
  // put your main code here, to run repeatedly:
  portENTER_CRITICAL(&keyboardMux);
  if(touchLeft ) { L++; touchLeft =false; }
  if(touchRight) { R++; touchRight=false; }
  if(touchTop  ) { T++; touchTop  =false; }
  portEXIT_CRITICAL(&keyboardMux);

  u8g2.setDrawColor(0);
  u8g2.clearBuffer();  
  
  u8g2.setFont(u8g2_font_t0_13_mf);
  u8g2.setFontPosCenter();
  u8g2.setFontMode(1);
  u8g2.setDrawColor(1);
  u8g2.setCursor(  4,20); u8g2.print(L);
  u8g2.setCursor( 60, 8); u8g2.print(T);
  u8g2.setCursor(110,20); u8g2.print(R);
  u8g2.setCursor( 58,24); u8g2.print(touchRead(PIN_KEY_RIGHT));
  
  u8g2.sendBuffer();
  
}
