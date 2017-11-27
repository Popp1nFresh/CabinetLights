//***********************************************//
// Feb. 23nd 2016
// - Initial Commit
// Nov. 27th 2017
// - Changed PWM min/max
//
// Control brightness level of cabinets lights.
//
//***********************************************//

// IO Pins
const int BUTTON_INTERRUPT_PIN = 2;     // the number of the pushbutton pin
const int PWM_PIN = 10;

// Brightness levels -- PWM
const int PWM_MAX = 160;
const int PWM_LOW = 20; // originally 25
const int PWM_MIN = 14;
const int PWM_OFF = 0; // 1 - zero seems to cause a flash

// Brightness levels -- Switch Statement
const int LEVEL_OFF = 0;
const int LEVEL_MIN = 1;
const int LEVEL_LOW = 2;
const int LEVEL_HIGH = 3;

// Auto Off
const int SEC_BEFORE_NEXT_BUTN_PUSH_GOES_TO_OFF = 2500;
const unsigned long DEBOUNCE_TIME = 100;

long last_interrupt_time = DEBOUNCE_TIME; // Assume 100 sec since turn on
int nextActivePin = LEVEL_LOW;
// After a predetermined # of seconds the next button push should go to off
bool goToOff = true;
unsigned long interruptCount = 0;
// Track where we are
static int Current_PWM_Level = PWM_OFF;
unsigned long nowTime = 0;

// https://playground.arduino.cc/Code/PwmFrequency
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

//*******************//
// SETUP
//*******************//
void setup() {
  pinMode(3,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
  pinMode(5,INPUT_PULLUP);
  pinMode(6,INPUT_PULLUP);
  pinMode(7,INPUT_PULLUP);
  pinMode(8,INPUT_PULLUP);
  pinMode(9,INPUT_PULLUP);
  pinMode(11,INPUT_PULLUP);
  pinMode(12,INPUT_PULLUP);
  pinMode(13,INPUT_PULLUP);  

  setPwmFrequency(PWM_PIN, 8); //  ~4kHZ
  analogWrite(PWM_PIN, PWM_OFF);
  
  Current_PWM_Level = PWM_OFF;
  // Default set next active pin as low
  nextActivePin = LEVEL_LOW;
}


//*******************//
// LOOP
//*******************//
void loop() {

  nowTime = millis();
  if(goToOff == false && Current_PWM_Level != PWM_OFF && nowTime - last_interrupt_time > SEC_BEFORE_NEXT_BUTN_PUSH_GOES_TO_OFF)
  {
    // The next pin to go active will be 'OFF'
    nextActivePin = LEVEL_OFF;
    goToOff = true;
  }
  
  ChangeLEDStatus();
  Delay(5);
}

//***********************//
// Delay for given time
//***********************//
void Delay(int pauseLength)
{
  unsigned long startTime = millis() + pauseLength;
  while(startTime >= millis()) { 
    // spin
  }
}

//*******************//
// FADE
//*******************//
void Fade(int currentLevel, int nextLevel){

  // Fade to brighter level 
  if(currentLevel <= nextLevel)
  {
    while(currentLevel <= nextLevel)
    {
      analogWrite(PWM_PIN, currentLevel++);
      Delay(2);
    }
    return;
  }
  // Fade to darker level
  if(currentLevel >= nextLevel)
  {
    while(currentLevel >= nextLevel)
    {
      analogWrite(PWM_PIN, currentLevel--); // using minus minus after Seems to stop bright flashes.
      Delay(4);
    }
    return;
  }
}


//*******************//
// CHANGE LED STATUS
//*******************//
void ChangeLEDStatus()
{
 if(DebounceSwitch())
  {
  // Turn on the next LED level
      switch(nextActivePin){
      case LEVEL_OFF:
        Fade(Current_PWM_Level, PWM_OFF);
        Current_PWM_Level = PWM_OFF;
        analogWrite(PWM_PIN, PWM_OFF); // Why is this needed and it does not seem to 'stick' in the while loops (above)?
        nextActivePin = LEVEL_MIN;
        break;
      case LEVEL_MIN: // Special case min brightness
//        Fade(Current_PWM_Level, PWM_MIN);
//        Current_PWM_Level = PWM_MIN;
//        nextActivePin = LEVEL_LOW;
//        break;  
      case LEVEL_LOW:
        Fade(Current_PWM_Level, PWM_LOW);
        Current_PWM_Level = PWM_LOW;
        analogWrite(PWM_PIN, PWM_LOW);
        nextActivePin = LEVEL_HIGH;
        break;            
     case LEVEL_HIGH:
        Fade(Current_PWM_Level, PWM_MAX);
        Current_PWM_Level = PWM_MAX;
        analogWrite(PWM_PIN, PWM_MAX);
        nextActivePin = LEVEL_OFF;
        break;     
    }
    last_interrupt_time = nowTime;
    // Do not automatically go to off just yet.
    goToOff = false;
  }
}


//*******************//
// KEY PRESSED
//*******************//
bool keyPressed()
{
  return digitalRead(BUTTON_INTERRUPT_PIN) == LOW;
}


//*******************//
// DEBOUNCE SWITCH
//*******************//
bool DebounceSwitch()
{
  static unsigned int state = 0;
  state = (state<<1) | !keyPressed() | 0xe000;
  return state == 0xf000;
}

