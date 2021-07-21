#include <Arduino.h>
#include "am_button.h"

using namespace std;


// ======== DEFINES ================

// ======== GLOBAL VARIABLES ============= 
Button btnNext(26);
Button btnConfirm(27);

// ======== INTERRUPT HANDLERS   ============= 

// Handler for the next button
void IRAM_ATTR nextPressed() {
  btnNext.handleInterrupt();
}

// Handler for the confirm button
void IRAM_ATTR confirmPressed() {
  btnConfirm.handleInterrupt();
}

void taskKeyboard(void * parameter );

void setupKeyboard() {

  // Attach pins to intterrupt handlers
  attachInterrupt(btnNext._pin,    nextPressed,    CHANGE);
  attachInterrupt(btnConfirm._pin, confirmPressed, CHANGE);

  xTaskCreatePinnedToCore(
    taskKeyboard,            // The function containing the task
    "TaskKeyboard",          // Name for mortals
    12000,                   // Stack size 
    NULL,                    // Parameters
    2,                       // Priority, 0=lowest, 3=highest
    NULL,                    // Task handle
    ARDUINO_RUNNING_CORE);   // Core to run the task on

}

void taskKeyboard(void *parameter) {

  while(true) {

    // Decide what to do if the black button is clicked
    if (btnNext.isClicked()) {
      
      portENTER_CRITICAL(&dataAccessMux);

      if(data.awake) {
        switch(data.screen) {
          case scMain:
            //data.screen=scGraph;
            data.screen=scMessage; // Skip graph screen until it is implemented
            break;
    
          case scGraph:
            data.screen=scMessage;
            break;
    
          case scMessage:
            data.screen=scMain;
            break;
    
          case scMessageSent:
            data.screen=scMain;
            break;

          case scSensorMalfunction:
            data.screen=scMain;
            break;
          
        } // Switch screen
      } // If awake

      // Black button is pressed, we will not sleep for another hour
      data.sleepCountdown=3600;
      data.awake=true;

      portEXIT_CRITICAL(&dataAccessMux);
    } // If button pressed
    
    // Decide what to do if the confirm button is clicked
    if (btnConfirm.isClicked()) {
      Serial.println("Confirm button clicked");

      portENTER_CRITICAL(&dataAccessMux);

      // Since the button is pressed, we will not sleep for another hour
      data.sleepCountdown=3600;

      if(data.awake) {
        if(data.screen==scMessage) {
          data.sendTestMessage=mrRequestMessage; // Tell the telegram process to send a test message
          data.screen=scMessageSent;
          data.messageSentCountDown=2;
        } // screen=scMessage
      } // awake

      portEXIT_CRITICAL(&dataAccessMux);
      
    }

    portENTER_CRITICAL(&dataAccessMux);
    data.keyboardHighWaterMark= uxTaskGetStackHighWaterMark( NULL );
    portEXIT_CRITICAL(&dataAccessMux);
    //Serial.printf("Keyboard: %d bytes of stack unused\n", data.keyboardHighWaterMark);

    vTaskDelay(20);
  } // while
}
