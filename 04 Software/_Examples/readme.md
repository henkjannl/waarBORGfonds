# Examples

## AcquariumMonitor
Dit is een programma die gebruik maakt van hetzelfde schermpje als de radar, het 1.3" 240x240 kleurenschermpje met ST7789VW driverchip.

De bibliotheek die ik gebruik is TFT_eSPI van Bodmer, te installeren als library in de Arduino IDE.

[https://github.com/Bodmer/TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)

Bodmer is een heel goed ondersteunde bibliotheek voor kleurenschermpjes (zoals U8G2 dat voor monochrome displays is).

Het enige onelegante van TFT_eSPI vind ik dat je in een file in de library zelf moet aangeven wat je configuratie is. Als je dus twee projecten hebt met verschillende schermpjes / pinouts moet je steeds voordat je compileert de juiste configuratie aangeven in de directory waar de library staat. Was eleganter geweest als dat in de projectdirectory was (zoals bij U8G2), maar á la.

* In de Arduino preferences kun je zien waar de Arduno library staat
* Vanuit daar doorbladeren naar TFT_eSPI, bijvoorbeeld `C:\Users\Username\Documents\Arduino\libraries\TFT_eSPI`
* Je kunt daar `User_Setup.h` aanpassen of overschrijven
* De file `github\henkjannl\waarBORGfonds\04 Software\_Examples\User_Setup.h` heeft als het goed is de juiste beschrijving van de configuratie.

Het schermpje heeft heeft een RGB565 kleurenschema, d.w.z. 5 bits voor rood, 6 bits voor groen en 5 bits voor blauw. In de file  `github\henkjannl\waarBORGfonds\04 Software\_Examples\Color converter RGB565.xlsx` heb ik een vertaling voor RGB888 naar RGB565. Ik kan dan in Inkscape een mooie kleur kiezen, bijvoorbeeld `A3FF12`, die dan door de Excel wordt vertaald in `0xA7E2`.

Voorbeeldcode voor de applicatie (er staan ook veel voorbeelden in de Arduino IDE als de library installeert):

```c++
#include <SPI.h>
#include <TFT_eSPI.h>  

#define CLR_BACKGROUND   0x0000   // 00, 00, 00 = black
#define CLR_DARK         0x2945   // 28, 28, 28 = dark grey
#define CLR_TEXT         0xFFFF   // FF, FF, FF = white
#define CLR_LABELS       0x8410   // 80, 80, 80 = grey
#define CLR_PUMP_ON      0x0400   // 00, 80, 00 = green
#define CLR_PUMP_OFF     0xF800   // FF, 00, 00 = red

// Initialize screen
tft.init();
tft.fillScreen(TFT_BLACK);
tft.setRotation(3);

tft.fillScreen(CLR_BACKGROUND);
tft.fillRect(0, 30,240,186,CLR_BACKGROUND); // Don't clear icons and pump bar

// Box for time
tft.fillRoundRect(44, 40, 152, 48, 8, CLR_DARK);

// Box for pressure
tft.fillRoundRect(4, 100, 152, 48, 8, CLR_DARK);

// Box for time pump switched
tft.fillRoundRect(4, 160, 152, 48, 8, CLR_DARK);

tft.setTextColor(CLR_LABELS);
tft.setTextDatum(TL_DATUM);
tft.drawString("mbar", 165, 100, 4);
tft.drawString("min", 165, 160, 4);
```

## Verder over AquariumMonitor

Dit is een sensor met twee barometers, eentje meet de druk in een leiding vlak achter een compressor, de andere de omgevingsdruk. Als de compressor stuk gaat stuurt de sensor na een uur een melding via Telegram. Het komt namelijk wel voor dat de compressor een half uur wordt uitgezet als de vissen worden gevoerd, dit moet geen sloot aan meldingen genereren.

Wat hier in zit:
* de gevoelige data (Telegram token, WiFi password) staat in de file `config.jsn` die niet op Github wordt gezet. Deze file staat op SPIFFS zodat ik geen passwords in de code hoef te zetten. Toen ik dat nog niet had was binnen 2 dagen mijn Telegram bot gehackt (!).
* de applicatie maakt gebruik van FreeRTOS. Dit heeft best een leercurve maar is een feest om te gebruiken. Documentatie is goed en het is geweldig als je meerdere processen parallel kunt laten lopen zonder dat dingen (schermpje, knopjes) gaan haperen. De nogal rare man op [https://youtu.be/F321087yYy4](https://youtu.be/F321087yYy4) legt het heel duidelijk uit.
*  er zijn vier parallelle threads:
    - measure thread: meet de waarde van beide barometers
    - display thread: geeft resultaten weer op het scherm
    - keyboard thread: kijkt of de gebruiker op een knop heeft gedrukt
    - telegram thread: stuurt een statusbericht of een alarm o.i.d.
* de threads communiceren met elkaar via een globale struct die beveiligd is met een mutex

## SimpleKeyboard
Onderstaande code implementeert tiptoetsen op capacitieve pinnen 4, 12 en 14. 
* De timer interrupt `onCheckKeys()` wordt iedere 10 ms aangeroepen
* Deze gebruikt een mutex `portMUX_TYPE keyboardMux` uit de FreeRTOS library om exclusieve toegang te regelen tot de gedeelde globale variabelen
* In het hoofdprogramma kunnen dan de variabelen touchLeft, touchRight en touchTop worden uitgelezen die aangeven dat er een toets is ingedrukt.

Geen ingewikkelde library, paar regeltjes code het werkt naar mijn ervaring erg betrouwbaar. 
Er bestaat een kans dat de ultrasoonlibrary ook met een interrupt werkt en dat beide interruptroutines elkaar beïnvloeden. Dit heb ik niet getest.

```c++
#define PIN_KEY_LEFT   4
#define PIN_KEY_RIGHT 12
#define PIN_KEY_TOP   14
#define KEY_TRESHOLD  50

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
}

void loop() {
  static int L,R,T;
  
  portENTER_CRITICAL(&keyboardMux);
  if(touchLeft ) { L++; touchLeft =false; }
  if(touchRight) { R++; touchRight=false; }
  if(touchTop  ) { T++; touchTop  =false; }
  portEXIT_CRITICAL(&keyboardMux);
}
```

Ik weet eerlijk gezegd niet zeker of hier de mutex nodig is, omdat de shared resource maar één bit is. Het probleem bij een 32 bits integer is dat het hoofdprogramma bijvoorbeeld de eerste twee bytes schrijft, dan de interuptroutine alle vier de bytes, en dan het hoofdprogramma de laatste twee bytes. Dit is een typisch conflict waar de mutex een oplossing is, maar of het bij een boolean ook nodig is weet ik niet.

