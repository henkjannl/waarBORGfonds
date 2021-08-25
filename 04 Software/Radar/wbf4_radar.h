#ifndef RADAR_H
#define RADAR_H

#include "wbf1_pins.h"
#include "wbf3_stepper.h"

/* 
This module takes care of rotating the antenna and storing ultrasonic measurements

The radar runs on two interrupt routines:
  onRotate        :  one step left or right, checks 
                        if a measurement needs to be triggered 
                        if direction needs to be changed
  onEchoInterrupt :  handles the echo response 
                        measures time if the echo goes high
                        measures time if the echo goes low, measures and stores distance

*/

#define ENABLE_STEPPER true      // For debug purposes
#define DISTANCE_SCALE 0.34/2    // mm per µs

//=====================
// GLOBAL VARIABLES
//=====================
uint8_t sampleID = 0;
uint8_t stepInSample = 0;
bool sweepDirectionPos = true;
volatile int64_t echoStartTimer = 0;
Stepper stepper(PIN_STEP_A, PIN_STEP_B, PIN_STEP_C, PIN_STEP_D);
uint8_t radarDebug = 0;

hw_timer_t * timer = NULL;

//=====================
// HELPER FUNCTIONS
//=====================

void IRAM_ATTR triggerMeasurement() {

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(PIN_SONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_SONIC_TRIG, LOW);
  portENTER_CRITICAL_ISR(&radarMux);
  radarDebug=1;
  portEXIT_CRITICAL_ISR(&radarMux);
}

void IRAM_ATTR onStepperInterrupt() {
  portENTER_CRITICAL_ISR(&radarMux);

  if(2*stepper.currentPos()==RADAR_ARRAY_SIZE*STEPS_BETWEEN_SAMPLES) radarSweepFinished=true;

  if(sweepDirectionPos) {
    if(ENABLE_STEPPER and radarSpinning) 
    {
      stepper.stepUp();
      stepInSample++;
      if(stepInSample>STEPS_BETWEEN_SAMPLES) 
      {
        triggerMeasurement();
        stepInSample=0;
        sampleID++;
        if(sampleID>RADAR_ARRAY_SIZE-2) sweepDirectionPos=false;
      }
    }
  }
  else {
    if(ENABLE_STEPPER and radarSpinning) 
    {
      stepper.stepDown();
      stepInSample++;
      if(stepInSample>STEPS_BETWEEN_SAMPLES) 
      {
        triggerMeasurement();
        stepInSample=0;
        sampleID--;
        if(sampleID<1) sweepDirectionPos=true;
      }
    }
  }
  
  portEXIT_CRITICAL_ISR(&radarMux);
}

void IRAM_ATTR onEchoInterrupt() {
  
  if(digitalRead(PIN_SONIC_ECHO)) {
    // Rising edge, the signal starts to come in
    radarDebug=2;
    radarMeasurements[sampleID]=3000.0; 
    echoStartTimer= esp_timer_get_time();
  }
  else {
    // Falling edge, the length of the signal determines the distance
    portENTER_CRITICAL_ISR(&radarMux);
    //if(radarDebug==2)
      radarMeasurements[sampleID] = DISTANCE_SCALE * (esp_timer_get_time() - echoStartTimer);
    portEXIT_CRITICAL_ISR(&radarMux);

    // if we are coming from state 2 (OK), the new state is 4, otherwise the new state is 3 (not OK)   
    radarDebug=(radarDebug==2) ? 4 : 3;
  }

  
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

  // Setup a hardware interrupt to respond to changes in the echo signal
  attachInterrupt(PIN_SONIC_ECHO, &onEchoInterrupt,  CHANGE);
}



#endif // RADAR_H
