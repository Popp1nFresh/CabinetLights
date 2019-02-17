//***********************************************//
// Feb. 23nd 2016
// - Initial Commit
// Nov. 27th 2017
// - Changed PWM min/max
// Feb. 16th 2019
// - Added low to high ramp-up option
// - TODO: could be a bit much happening in a single loop, especially with the ramp up
//   which might cause an occasional flicker of brightness upon button press for off-to-low/min
//   transitions.
//
// Control brightness level of cabinets lights.
// Peter T.
//
//***********************************************//

// IO Pins
const int BUTTON_INTERRUPT_PIN = 2; // the number of the pushbutton pin
const int PWM_PIN = 10;

// Brightness levels -- PWM
const int PWM_MAX = 140; // Based on personal pref for eyesight comfort level.
const int PWM_LOW = 20; // originally 25
const int PWM_MIN = 16; // 14 is the first visible light with this particular PWM freq. & LED controller.
const int PWM_OFF = 0;

// Brightness levels -- Switch Statement
const int LEVEL_OFF = 0;
const int LEVEL_MIN = 1;
const int LEVEL_LOW = 2;
const int LEVEL_HIGH = 3;

// Auto Off
const int SEC_BEFORE_NEXT_BUTN_PUSH_GOES_TO_OFF = 2500;
// TODO: Change to an unsigned long
long last_interrupt_time = 100; // Assume x-milli sec since turn on of the Arduino.
int nextActivePin = LEVEL_MIN;
// After a predetermined # of seconds the next button push should go to off
bool goToOff = true;
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
void setup()
{
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);

  setPwmFrequency(PWM_PIN, 8); //  ~4kHZ
  analogWrite(PWM_PIN, PWM_OFF);

  Current_PWM_Level = PWM_OFF;
  // Default set next active pin as low
  nextActivePin = LEVEL_MIN;
}

//*******************//
// LOOP
//*******************//
void loop()
{
  checkForOffScenario();
  ChangeLEDStatus(false, -1);
  Delay(5);
}

void checkForOffScenario()
{
  // "random" non-functioning which happens about every 3 months is probably actually millis overflowing (at around 50 days - days since sketch started).
  nowTime = millis();
  if (goToOff == false && Current_PWM_Level != PWM_OFF && nowTime - last_interrupt_time > SEC_BEFORE_NEXT_BUTN_PUSH_GOES_TO_OFF)
  {
    // The next pin to go active will be 'OFF'
    nextActivePin = LEVEL_OFF;
    goToOff = true;
  }
}

//***********************//
// Delay for given time
//***********************//
void Delay(int pauseLength)
{
  unsigned long startTime = millis() + pauseLength;
  while (startTime >= millis())
  {
    // spin
  }
}

//**************************//
// Delay With Current Level //
//**************************//
bool DelayWithCurrentLevel(int pauseLength, int currentLevel)
{
  unsigned long startTime = millis() + pauseLength;
  while (startTime >= millis())
  {
    // spin
    // TODO: possibly check every x seconds instead of every iteration.
    if (DebounceSwitch())
    {
      return true;
    }
  }
  return false;
}

//*******************//
// FADE
//*******************//
void Fade(int currentLevel, int nextLevel)
{
  // Fade to brighter level
  if (currentLevel <= nextLevel)
  {
    while (currentLevel <= nextLevel)
    {
      analogWrite(PWM_PIN, currentLevel++);
      Delay(2);
    }
    return;
  }
  // Fade to darker level
  if (currentLevel >= nextLevel)
  {
    while (currentLevel >= nextLevel)
    {
      analogWrite(PWM_PIN, currentLevel--); // using minus minus after Seems to stop bright flashes.
      Delay(4);
    }
    return;
  }
}

bool FadeSlow(int currentLevel, int nextLevel)
{
  // Fade to brighter level
  if (currentLevel <= nextLevel)
  {
    while (currentLevel <= nextLevel)
    {
      analogWrite(PWM_PIN, ++currentLevel);
      // Delay will return true for an early exit
      if (DelayWithCurrentLevel(4200, currentLevel)) // Slow ramp with changes every 4.2 seconds.
      {
        // exit early due to button press
        checkForOffScenario();
        ChangeLEDStatus(true, currentLevel);
        return true; // early exit due to button push
      }
    }
    return false; // normal exit
  }

  // Fade to darker level
  if (currentLevel >= nextLevel)
  {
    while (currentLevel >= nextLevel)
    {
      // Don't bother checking for debounce here.
      analogWrite(PWM_PIN, currentLevel--); // using minus minus after Seems to stop bright flashes.
      Delay(4);
    }
    return false; // normal exit
  }
}

//*******************//
// CHANGE LED STATUS
//*******************//
void ChangeLEDStatus(bool forceDebounce, int currentPWMLevel)
{
  // Check the switch or allow a forced changed of the LED level
  // such as from an "earlyExit".
  if (forceDebounce || DebounceSwitch())
  {

    // Account for exiting the 'delay' func early so as not to flash the lights
    // if there is an "earlyExit" durring a ramp-up.
    if (currentPWMLevel >= 0)
    {
      Current_PWM_Level = currentPWMLevel;
    }

    // Turn on the next LED level
    switch (nextActivePin)
    {
    case LEVEL_OFF:
      // Set nextActivePin (next option) here in case the user exits the delay loop early
      Fade(Current_PWM_Level, PWM_OFF);
      Current_PWM_Level = PWM_OFF;
      analogWrite(PWM_PIN, PWM_OFF); // Why is this needed and it (final value) does not seem to 'stick' in the while loops (above)?
      nextActivePin = LEVEL_MIN;
      break;
    case LEVEL_MIN: // Special case min brightness
      nextActivePin = LEVEL_LOW;
      Current_PWM_Level = PWM_MIN;
      // If an early exit due to button press, then just return
      if (FadeSlow(Current_PWM_Level, PWM_MAX))
      {
        analogWrite(PWM_PIN, Current_PWM_Level);
        break;
      }
      Current_PWM_Level = PWM_MAX; // since this brightened to max set to max
      analogWrite(PWM_PIN, PWM_MAX); // Slowly brighten so end at MAX
      break;
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
  state = (state << 1) | !keyPressed() | 0xe000;
  return state == 0xf000;
}
