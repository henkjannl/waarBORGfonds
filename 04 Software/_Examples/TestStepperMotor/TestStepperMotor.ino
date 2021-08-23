// Bounce.pde
// -*- mode: C++ -*-
//
// Make a single stepper bounce from one limit to another
//
// Copyright (C) 2012 Mike McCauley
// $Id: Random.pde,v 1.1 2011/01/05 01:51:01 mikem Exp mikem $

#include <AccelStepper.h>

#define PIN_STEPA 26
#define PIN_STEPB 27
#define PIN_STEPC 32
#define PIN_STEPD 33

#define SWEEP_AMPLITUDE 1536

// Define a stepper and the pins it will use
//AccelStepper stepper; // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
AccelStepper stepper(AccelStepper::FULL4WIRE, PIN_STEPA, PIN_STEPB, PIN_STEPC, PIN_STEPD, true);

bool turnCW= true;
unsigned long logTrigger=0;

void setup()
{  
  Serial.begin(115200);
  // Change these to suit your stepper if you want
  stepper.setMaxSpeed(100); // 
  stepper.setAcceleration(50);
  stepper.moveTo(SWEEP_AMPLITUDE);
}

void loop()
{
    // 
    if(millis()>logTrigger) {
      logTrigger=millis()+500;
      Serial.println(stepper.currentPosition());
    }
    // If at the end of travel go to the other end
    if (stepper.distanceToGo() == 0) {
      stepper.moveTo(turnCW ? -SWEEP_AMPLITUDE : SWEEP_AMPLITUDE);  
      turnCW=!turnCW;
    }

    stepper.run();
}
