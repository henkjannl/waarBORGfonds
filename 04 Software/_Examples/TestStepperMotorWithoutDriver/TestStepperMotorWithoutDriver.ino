//====================================================================================
//                                    Libraries
//====================================================================================

//====================================================================================
//                                    Constants
//====================================================================================
#define STROKE  3072

//====================================================================================
//                                         Stepper definition
//====================================================================================

// TODO: more elegant to create a class in a separate file
// TODO: more elegant to create an interrupt service routine
class SimpleStepper {
  public:
    SimpleStepper(int pin1, int pin2, int pin3, int pin4);
    void moveRel(int stroke);
    void setOutputs();
    void setIdle();
    void run();
    bool moving() { return _moving; };
    int currentPos() { return _currentPos; };

  private:
    uint8_t _pin1, _pin2, _pin3, _pin4;
    bool _moving;
    int _originPos, _currentPos, _targetPos;
};

SimpleStepper::SimpleStepper(int pin1, int pin2, int pin3, int pin4) {
  
  _pin1 = pin1;
  _pin2 = pin2;
  _pin3 = pin3;
  _pin4 = pin4;
  
  _moving=false;
  _currentPos=0;

  pinMode(_pin1, OUTPUT);
  pinMode(_pin2, OUTPUT);
  pinMode(_pin3, OUTPUT);
  pinMode(_pin4, OUTPUT);
}

void SimpleStepper::moveRel(int stroke) {
  _moving=true;
  _originPos=_currentPos;
  _targetPos+=stroke;
}

void SimpleStepper::setOutputs() {

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
  
  // Give the motor some time to respond
  delayMicroseconds(1000);
}

void SimpleStepper::setIdle() {
  _moving=false;
  digitalWrite(_pin1, LOW);
  digitalWrite(_pin2, LOW);
  digitalWrite(_pin3, LOW);
  digitalWrite(_pin4, LOW);
}

void SimpleStepper::run() {

  // Directly go back if the stepper state is idle
  if (!_moving) return;
  
  // Move the stepper backward or forward
  if(_currentPos<_targetPos) {
      _currentPos++;
      setOutputs();
    }

  if(_currentPos>_targetPos) {
      _currentPos--;
      setOutputs();
  }

  if(_currentPos==_targetPos) {
    _moving=false;
    setIdle();
  }
}

//====================================================================================
//                                    Global variables
//====================================================================================
SimpleStepper stepper(26, 27, 32, 33);
bool moveCW = true;

//====================================================================================
//                                    Setup and Loop
//====================================================================================

void setup()
{
  // Initialize debug channel
  Serial.begin(115200);

  delay(100);
}




void loop() {
  static unsigned long debugTrigger;
  
  // put your main code here, to run repeatedly:
  if(!stepper.moving()) {
    stepper.moveRel(moveCW ? STROKE : -STROKE);
    moveCW = !moveCW ;
  }
  stepper.run();

  if(debugTrigger<millis()) {
    debugTrigger=millis()+500;
    Serial.println(stepper.currentPos());
  }
}
