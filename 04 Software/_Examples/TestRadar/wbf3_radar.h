
#include "wbf1_pins.h"
#include "wbf2_stepper.h"

/* 
This module takes care of rotating the antenna and storing ultrasonic measurements

The antenna can make a total sweep of 250°
We want a headroom of ~3° between the end stops
The 28byj48 stepper motor has 2038 steps per revolution
This means we want the sweep to be 2038*(250-3)/360 = 1400 steps
We choose 71 ultrasonic measurements with 20 steps in between each measurement
https://lastminuteengineers.com/28byj48-stepper-motor-arduino-tutorial/

The radar runs on three interrupt routines:

onRotate      :  one step left or right, checks 
                  if a measurement needs to be triggered 
                  if direction needs to be changed
onRisingEcho  :  measures time if the echo goes high
onFallingEcho :  measures time if the echo goes low, measures and stores distance

*/

//=====================
// DEFINES
//=====================
#define NUM_SAMPLES                  71
#define STEPS_BETWEEN_SAMPLES        48
#define MICROSECONDS_BETWEEN_STEPS 1000
#define CLIPPING_VALUE             3000

//=====================
// GLOBAL VARIABLES
//=====================
uint8_t sampleID = 0;
uint8_t stepInSample = 0;
bool sweepDirectionPos = true;
volatile int64_t echoStartTimer = 0;
uint16_t radarMeasurements[NUM_SAMPLES]; // Distances between 0 and 400 mm
Stepper stepper(PIN_STEP_A, PIN_STEP_B, PIN_STEP_C, PIN_STEP_D);
uint8_t radarDebug = 0;

hw_timer_t * timer = NULL;
portMUX_TYPE radarMux = portMUX_INITIALIZER_UNLOCKED;

//=====================
// HELPER FUNCTIONS
//=====================
void IRAM_ATTR startMeasurement() {

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(PIN_SONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_SONIC_TRIG, LOW);
  portENTER_CRITICAL_ISR(&radarMux);
  radarMeasurements[sampleID]=0; // Assume invalid measurement until echo responds
  radarDebug=1;
  portEXIT_CRITICAL_ISR(&radarMux);
}

void IRAM_ATTR onStepperInterrupt() {
  portENTER_CRITICAL_ISR(&radarMux);
  
  if(sweepDirectionPos) {
    stepper.stepUp();
    stepInSample++;
    if(stepInSample>STEPS_BETWEEN_SAMPLES) {
      stepInSample=0;
      sampleID++;
      if(sampleID>NUM_SAMPLES-2) sweepDirectionPos=false;
      startMeasurement();
    }
  }
  else {
    stepper.stepDown();
    stepInSample++;
    if(stepInSample>STEPS_BETWEEN_SAMPLES) {
      stepInSample=0;
      sampleID--;
      if(sampleID<1) sweepDirectionPos=true;
      startMeasurement();
    }
  }
  
  portEXIT_CRITICAL_ISR(&radarMux);
}

void IRAM_ATTR onEchoInterrupt() {
  portENTER_CRITICAL_ISR(&radarMux);
  
  if(digitalRead(PIN_SONIC_ECHO)) {
    // Rising edge
    radarDebug=2;
    echoStartTimer= esp_timer_get_time();
  }
  else {
    radarDebug=radarDebug==2 ? 4 : 3;
    volatile int64_t microSecondsPassed = (esp_timer_get_time() - echoStartTimer);
    if(microSecondsPassed>CLIPPING_VALUE) microSecondsPassed=CLIPPING_VALUE;
    radarMeasurements[sampleID] = (uint16_t) (0.34*microSecondsPassed/2);
  }

  portEXIT_CRITICAL_ISR(&radarMux);
}


void initRadar() {
  pinMode(PIN_SONIC_TRIG, OUTPUT); // Sets the trigPin as an Output
  pinMode(PIN_SONIC_ECHO, INPUT);  // Sets the echoPin as an Input

  digitalWrite(PIN_SONIC_TRIG, LOW);

  // Setup a repeated timer interrupt to sweep the radar
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onStepperInterrupt, true);
  timerAlarmWrite(timer, MICROSECONDS_BETWEEN_STEPS, true);
  timerAlarmEnable(timer);

  attachInterrupt(PIN_SONIC_ECHO, &onEchoInterrupt,  CHANGE);
}
