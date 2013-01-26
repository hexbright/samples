/* 

  Factory firmware for HexBright FLEX 
  v2.4  Dec 6, 2012
  
*/

#include <math.h>
#include <Wire.h>

// Settings
#define OVERTEMP                340
#define TEMP_CHECK_INTERVAL     1000
// Modes
#define MODE_OFF                0
#define MODE_LOW                1
#define MODE_MED                2
#define MODE_HIGH               3
#define MODE_BLINKING           4
#define MODE_BLINKING_PREVIEW   5

// State
byte mode = 0;
unsigned long btnTime = 0;
boolean btnDown = false;

/* ------------------- HexBright Hardware Library Functions -------------------  */

class HexBright
{
  public:
    HexBright(void);
    void setGreenLED(bool);
    void setRedLED(bool);
    void setMainLED(bool);
    void setHighBrightness(bool);
    void setBrightnessValue(byte);
    int  getChargeStatus(void);
    int  getTemperature(void);
    int  getButtonState(void);
  private:
    bool redLEDState;

    // Pin assignments
    static const byte DPIN_RLED_SW  = 2;  // Red LED in the pushbutton / Pushbutton switch
    static const byte DPIN_GLED     = 5;  // Green LED in the pushbutton
    static const byte DPIN_PWR      = 8;  // Main LED power on/off
    static const byte DPIN_DRV_MODE = 9;  // Main LED brightness mode (low/high)
    static const byte DPIN_DRV_EN   = 10; // Main LED brightness value
    static const byte APIN_TEMP     = 0;  // Temperature sensor
    static const byte APIN_CHARGE   = 3;  // Charge status
};

HexBright::HexBright(void)
{
  redLEDState = false;
}

void HexBright::setGreenLED(bool on)
{
  pinMode(DPIN_GLED, OUTPUT);
  digitalWrite(DPIN_GLED, on ? HIGH : LOW);
}

void HexBright::setRedLED(bool on)
{
  pinMode(DPIN_RLED_SW, OUTPUT);
  redLEDState = on;
  digitalWrite(DPIN_RLED_SW, on ? HIGH : LOW);
}

void HexBright::setMainLED(bool on)
{
  pinMode(DPIN_PWR, OUTPUT);
  digitalWrite(DPIN_PWR, on ? HIGH : LOW);
}

void HexBright::setHighBrightness(bool on)
{
  pinMode(DPIN_DRV_MODE, OUTPUT);
  digitalWrite(DPIN_DRV_MODE, on ? HIGH : LOW);
}

void HexBright::setBrightnessValue(byte value)
{
  pinMode(DPIN_DRV_EN, OUTPUT);
  analogWrite(DPIN_DRV_EN, value);
}

int HexBright::getChargeStatus(void)
{
  return(analogRead(APIN_CHARGE));
}

int HexBright::getTemperature(void)
{
  return(analogRead(APIN_TEMP));
}

int HexBright::getButtonState(void)
{
  // Must disable the red LED first, otherwise button read
  // will read the state of the LED instead.
  pinMode(DPIN_RLED_SW, OUTPUT);
  digitalWrite(DPIN_RLED_SW, LOW);

  // Read the button state
  pinMode(DPIN_RLED_SW, INPUT);
  int val = digitalRead(DPIN_RLED_SW);

  // Set the red LED back to its original state
  setRedLED(redLEDState);

  return(val);
}

HexBright flashlight;

/* ------------ Common methods that can be reused in other firmwares ----------- */

void blinkOnCharge(unsigned long time)
{
  int chargeStatus = flashlight.getChargeStatus();

  if (chargeStatus < 128)      // Charging: blink
    flashlight.setGreenLED(time & 0x0100);
  else if (chargeStatus > 768) // Charged: steady
    flashlight.setGreenLED(true);
  else // Hi-Z - shutdown
    flashlight.setGreenLED(false);
}

void preventOverheating(unsigned long time)
{
  static unsigned long lastTempTime;

  // If it's too early to check the temp, return
  if (time - lastTempTime < TEMP_CHECK_INTERVAL) return;

  lastTempTime = time;
  int temperature = flashlight.getTemperature();
  Serial.print("Temp: ");
  Serial.println(temperature);

  if (mode == MODE_OFF) return; // We are off, nothing to do
  if (temperature < OVERTEMP) return; // Temperature nominal, carry on.

  Serial.println("Overheating!");

  for (int i = 0; i < 6; i++)
  {
    flashlight.setHighBrightness(false);
    delay(100);
    flashlight.setHighBrightness(true);
    delay(100);
  }
  flashlight.setHighBrightness(false);

  mode = MODE_LOW;
}

void setup()
{
  // We just powered on!  That means either we got plugged 
  // into USB, or the user is pressing the power button.
  flashlight.setMainLED(false);

  // Initialize GPIO
  flashlight.setHighBrightness(false);
  flashlight.setBrightnessValue(0);
  
  // Initialize serial busses
  Serial.begin(9600);
  Wire.begin();
  
  btnTime = millis();
  btnDown = flashlight.getButtonState();
  mode = MODE_OFF;

  Serial.println("Powered up!");
}

void loop()
{
  unsigned long time = millis();

  blinkOnCharge(time);
  preventOverheating(time);  

  // Do whatever this mode does
  switch (mode)
  {
  case MODE_BLINKING:
  case MODE_BLINKING_PREVIEW:
    if ((time % 300) < 75)
      flashlight.setBrightnessValue(0);
    else
      flashlight.setBrightnessValue(255);
    break;
  }
  
  // Check for mode changes
  byte newMode = mode;
  byte newBtnDown = flashlight.getButtonState();
  switch (mode)
  {
  case MODE_OFF:
    if (btnDown && !newBtnDown && (time-btnTime)>20)
      newMode = MODE_LOW;
    if (btnDown && newBtnDown && (time-btnTime)>500)
      newMode = MODE_BLINKING_PREVIEW;
    break;
  case MODE_LOW:
    if (btnDown && !newBtnDown && (time-btnTime)>50)
      newMode = MODE_MED;
    break;
  case MODE_MED:
    if (btnDown && !newBtnDown && (time-btnTime)>50)
      newMode = MODE_HIGH;
    break;
  case MODE_HIGH:
    if (btnDown && !newBtnDown && (time-btnTime)>50)
      newMode = MODE_OFF;
    break;
  case MODE_BLINKING_PREVIEW:
    // This mode exists just to ignore this button release.
    if (btnDown && !newBtnDown)
      newMode = MODE_BLINKING;
    break;
  case MODE_BLINKING:
    if (btnDown && !newBtnDown && (time-btnTime)>50)
      newMode = MODE_OFF;
    break;
  }

  // Do the mode transitions
  if (newMode != mode)
  {
    switch (newMode)
    {
    case MODE_OFF:
      Serial.println("Mode = off");
      flashlight.setMainLED(false);
      flashlight.setHighBrightness(false);
      flashlight.setBrightnessValue(0);
      break;
    case MODE_LOW:
      Serial.println("Mode = low");
      flashlight.setMainLED(true);
      flashlight.setHighBrightness(false);
      flashlight.setBrightnessValue(64);
      break;
    case MODE_MED:
      Serial.println("Mode = medium");
      flashlight.setMainLED(true);
      flashlight.setHighBrightness(false);
      flashlight.setBrightnessValue(255);
      break;
    case MODE_HIGH:
      Serial.println("Mode = high");
      flashlight.setMainLED(true);
      flashlight.setHighBrightness(true);
      flashlight.setBrightnessValue(255);
      break;
    case MODE_BLINKING:
    case MODE_BLINKING_PREVIEW:
      Serial.println("Mode = blinking");
      flashlight.setMainLED(true);
      flashlight.setHighBrightness(true);
      break;
    }

    mode = newMode;
  }

  // Remember button state so we can detect transitions
  if (newBtnDown != btnDown)
  {
    btnTime = time;
    btnDown = newBtnDown;
    delay(50);
  }
}
