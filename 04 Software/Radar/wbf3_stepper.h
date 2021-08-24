#ifndef STEPPER_H
#define STEPPER_H

class Stepper{
  public:
    Stepper(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4);
    void stepUp();
    void stepDown();
    void setIdle(); // Energy saving state, disabling all coils
    int currentPos() { return _currentPos; };

  private:
    uint8_t _pin1, _pin2, _pin3, _pin4;
    int _currentPos;
    void setOutputs();
};

Stepper::Stepper(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4) {
  
  _pin1 = pin1;
  _pin2 = pin2;
  _pin3 = pin3;
  _pin4 = pin4;
  
  _currentPos=0;

  pinMode(_pin1, OUTPUT);
  pinMode(_pin2, OUTPUT);
  pinMode(_pin3, OUTPUT);
  pinMode(_pin4, OUTPUT);
}

void Stepper::stepUp() {
  _currentPos++;
  setOutputs();
}

void Stepper::stepDown() {
  _currentPos--;
  setOutputs();
}

void Stepper::setOutputs() {

  int state = _currentPos % 8;
  if(state<0) state+=8;
  
  switch (state) {
    case 0:
      digitalWrite(_pin1, HIGH);
      digitalWrite(_pin2, LOW );
      digitalWrite(_pin3, LOW );
      digitalWrite(_pin4, LOW );
      break;

    case 1:
      digitalWrite(_pin1, HIGH);
      digitalWrite(_pin2, HIGH);
      digitalWrite(_pin3, LOW );
      digitalWrite(_pin4, LOW );
      break;

    case 2:
      digitalWrite(_pin1, LOW);
      digitalWrite(_pin2, HIGH);
      digitalWrite(_pin3, LOW );
      digitalWrite(_pin4, LOW );
      break;

    case 3:
      digitalWrite(_pin1, LOW);
      digitalWrite(_pin2, HIGH);
      digitalWrite(_pin3, HIGH);
      digitalWrite(_pin4, LOW );
      break;

    case 4:
      digitalWrite(_pin1, LOW);
      digitalWrite(_pin2, LOW);
      digitalWrite(_pin3, HIGH);
      digitalWrite(_pin4, LOW );
      break;

    case 5:
      digitalWrite(_pin1, LOW);
      digitalWrite(_pin2, LOW);
      digitalWrite(_pin3, HIGH);
      digitalWrite(_pin4, HIGH);
      break;

    case 6:
      digitalWrite(_pin1, LOW );
      digitalWrite(_pin2, LOW );
      digitalWrite(_pin3, LOW );
      digitalWrite(_pin4, HIGH);
      break;

    case 7:
      digitalWrite(_pin1, HIGH);
      digitalWrite(_pin2, LOW );
      digitalWrite(_pin3, LOW );
      digitalWrite(_pin4, HIGH);
      break;
  }  
}

void Stepper::setIdle() {
  digitalWrite(_pin1, LOW);
  digitalWrite(_pin2, LOW);
  digitalWrite(_pin3, LOW);
  digitalWrite(_pin4, LOW);
}


#endif // STEPPER_H
