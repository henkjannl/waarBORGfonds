#include <Arduino.h>
#include "am_data.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>

#include <time.h>       

using namespace std;

// ======== DEFINES ================


// ======== GLOBAL VARIABLES ============= 

// Pump sensor on one I2C bus
TwoWire I2C_1 = TwoWire(0);
Adafruit_BMP280 pumpSensor(&I2C_1);
Adafruit_Sensor *pumpPressure = pumpSensor.getPressureSensor();

// Ambient sensor on another I2C bus
TwoWire I2C_2 = TwoWire(1);
Adafruit_BMP280 ambientSensor(&I2C_2);
Adafruit_Sensor *ambientPressure = ambientSensor.getPressureSensor();

// Counter firing each second
volatile int oneSecCounter;
hw_timer_t * oneSecTimer = NULL;
portMUX_TYPE oneSecTimerMux = portMUX_INITIALIZER_UNLOCKED;


// ======== INTERRUPT HANDLERS   ============= 

// One sec timer interrupt 
void IRAM_ATTR onOneSecTimer() {
  portENTER_CRITICAL_ISR(&oneSecTimerMux);
  oneSecCounter++;
  portEXIT_CRITICAL_ISR(&oneSecTimerMux);
}

void taskMeasure(void * parameter );

void setupMeasure() {

  // Setup 1 sec interrupt counter
  oneSecTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(oneSecTimer, &onOneSecTimer, true);
  timerAlarmWrite(oneSecTimer, 1000000, true);
  timerAlarmEnable(oneSecTimer);

  // Initialize pump sensor
  I2C_1.begin(21, 22, 100000); 
  
  if (!pumpSensor.begin(0x76)) {
    Serial.println("Could not find pump sensor, check wiring!");
    data.sensorMalfunction=true;
  }  
  
  pumpSensor.setSampling(
    Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
    Adafruit_BMP280::SAMPLING_X4,     /* Pressure oversampling */
    Adafruit_BMP280::FILTER_X8,       /* Filtering. */
    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  // Initialize ambient sensor
  I2C_2.begin(32, 33, 100000); 
  
  if (!ambientSensor.begin(0x76)) {
    Serial.println("Could not find ambient sensor, check wiring!");
    data.sensorMalfunction=true;
  }  

  ambientSensor.setSampling(
    Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
    Adafruit_BMP280::SAMPLING_X4,     /* Pressure oversampling */
    Adafruit_BMP280::FILTER_X8,       /* Filtering. */
    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  xTaskCreatePinnedToCore(
    taskMeasure,            // The function containing the task
    "TaskMeasure",          // Name for mortals
    16000,                  // Stack size 
    NULL,                   // Parameters
    2,                      // Priority, 0=lowest, 3=highest
    NULL,                   // Task handle
    ARDUINO_RUNNING_CORE);  // Core to run the task on

  // Plan when to record the first sample
  data.timeToNextSample=config.sampleInterval;
}

void taskMeasure(void * parameter ){
  float pumpPressure, ambientPressure, diffPressure;
  static float mina=1e6, maxa=-1e6, minp=1e6, maxp=-1e6, mind=1e6, maxd=-1e6;
  static bool prevPumpRunning;
  time_t rawtime;
  struct tm * timeinfo;

  while(true) {

    if(oneSecCounter>0) {
      portENTER_CRITICAL(&oneSecTimerMux);
      oneSecCounter--;
      portEXIT_CRITICAL(&oneSecTimerMux);

      // Read sensor values
      ambientPressure=ambientSensor.readPressure();
      pumpPressure=pumpSensor.readPressure();
      diffPressure=pumpPressure-ambientPressure;

      // Determine min max for logfile    
      if(mina>ambientPressure ) mina=ambientPressure;
      if(maxa<ambientPressure ) maxa=ambientPressure;
      if(minp>pumpPressure    ) minp=pumpPressure;
      if(maxp<pumpPressure    ) maxp=pumpPressure;
      if(mind>diffPressure    ) mind=diffPressure;
      if(maxd<diffPressure    ) maxd=diffPressure;

      // Apply changes to the global data structure
      portENTER_CRITICAL(&dataAccessMux);

      // Update the global timer
      data.secondsSinceStart++;
      if(data.reconnectCountdown>0) data.reconnectCountdown--;

      // Manage sleep
      if(data.sleepCountdown>0) data.sleepCountdown--;
      data.awake=(data.sleepCountdown>0);

      // Display sent message dialog for limited amount of time
      if(data.messageSentCountDown>0) data.messageSentCountDown--;

      // Measure pump pressure and ambient pressure
      data.ambientPressure=ambientPressure;
      data.pumpPressure=pumpPressure;
      data.pressureDifference=data.pumpPressure-data.ambientPressure;

      // Raise an alarm if we don't trust the sensor values
      data.sensorMalfunction=( (data.ambientPressure<90000) or (data.ambientPressure>110000) or
                               (data.pumpPressure   <90000) or (data.pumpPressure   >120000) );
                               
      // Only report once, in case of a poor wire connection
      if(data.sensorMalfunction && (data.sendSensorMalfunctionMessage==mrIdle) ) {
        data.sendSensorMalfunctionMessage=mrRequestMessage;
        data.screen=scSensorMalfunction;
      };            

      // Check if the pump is running
      data.pumpRunning=(data.pressureDifference>config.pressureThreshold);

      // Record changes in running of pump
      if( (data.pumpRunning != prevPumpRunning) && data.timeSynched ) {
        tTransition transition;

        transition.secondsSinceStart=data.secondsSinceStart;

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(transition.time,sizeof(transition.time),"%H:%M:%S",timeinfo);
          
        transition.state=data.pumpRunning ? seCompressorOn : seCompressorOff;

        data.report.push_back(transition);
        
        Serial.printf("Item for report logged. Total log %d items\n", data.report.size());
      }

      // Remove message if they are older than 24 hours
      while((data.report.size()>0) && (data.report.front().secondsSinceStart<(data.secondsSinceStart-24*60*60))) {
        data.report.pop_front();
      }

      // Check how long ago the compressor started or stopped running
      data.lastPumpChange++;

      // Reset the counter if there was a transition
      if( (data.pumpRunning != prevPumpRunning) ) data.lastPumpChange=0;

      // If the pump is running, redraw the request to send an alarm message
      if(data.pumpRunning) data.sendAlarmMessage=mrIdle;

      // Request an alarm message from Telegram if the pump was switched off too long
      if(!data.pumpRunning & (data.lastPumpChange>=config.pumpOffTimeOut) ) data.sendAlarmMessage=mrRequestMessage;

      // Record sample to logfile 
      if(data.timeToNextSample>0) data.timeToNextSample--;
      
      if(data.timeToNextSample==0) {

        // Store sample
        tDataPoint dataPoint;

        dataPoint.secondsSinceStart=data.secondsSinceStart;
        dataPoint.mina=mina; dataPoint.maxa=maxa;
        dataPoint.minp=minp; dataPoint.maxp=maxp;
        dataPoint.mind=mind; dataPoint.maxd=maxd;
        
        data.dataPoints.push_back(dataPoint);
        Serial.printf("Sample %d recorded\n", data.dataPoints.size());

        // Plan next sample
        mina=1e6; maxa=-1e6; minp=1e6; maxp=-1e6; mind=1e6; maxd=-1e6;

        data.timeToNextSample=config.sampleInterval;        
      } // take sample

      // Limit 4 hours of logging
      while(data.dataPoints.size()>48) data.dataPoints.pop_front();

      /* Keep a watch on how much memory is used by the measurement thread */
      data.measureHighWaterMark= uxTaskGetStackHighWaterMark(NULL);
      
      //Serial.printf("Measurement: %d bytes of stack unused\n", data.measureHighWaterMark);

      portEXIT_CRITICAL(&dataAccessMux);

      prevPumpRunning=data.pumpRunning;
    } // if oneSec

    vTaskDelay(100);

  } // while true
}
