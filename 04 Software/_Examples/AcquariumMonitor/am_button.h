#ifndef BUTTON_H
#define BUTTON_H

#define DEBOUNCE 30

class Button {
  public:
    
    Button(uint8_t pin) {
      _pin=pin;
      pinMode(_pin, INPUT_PULLDOWN);
      ignoreUntil=millis()+DEBOUNCE;
      state=false;
      clicked=false;
    }

    void handleInterrupt() {
      if(millis()<ignoreUntil)
        return;
        
      if(!state){
        // Button was not clicked
        if(digitalRead(_pin)==HIGH) {
          // Button is clicked
          state=true;
          clicked=true;
          ignoreUntil=millis()+DEBOUNCE;        
        }
      }
      else {
        // Button was clicked
        if(digitalRead(_pin)==LOW) {
          // Button is not clicked now
          state=false;
          ignoreUntil=millis()+DEBOUNCE;        
        }
      }    
    }
  
    bool isClicked() {
      if(clicked) {
        clicked=false;
        return true;
      }
      else
        return false;
    }
  
    uint8_t _pin;
    unsigned int ignoreUntil;
    bool clicked;
    bool state;
};


#endif
