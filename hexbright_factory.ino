/* 

  Factory firmware for HexBright FLEX 
  v2.4  Dec 6, 2012
  
*/

#include <math.h>
#include <Wire.h>

// Settings
#define OVERTEMP                340
#define TEMP_CHECK_INTERVAL     1000
#define CHARGE_LEVEL_LOW        128
#define CHARGE_LEVEL_HIGH       768
// Pin assignments
#define DPIN_RLED_SW            2    // Pushbutton switch
#define DPIN_GLED               5    // Green LED in the pushbutton
#define DPIN_PWR                8    // Main LED power on/off
#define DPIN_DRV_MODE           9    // Main LED brightness mode (low/high)
#define DPIN_DRV_EN             10   // Main LED brightness value
#define APIN_TEMP               0    // Temperature sensor
#define APIN_CHARGE             3    // Charge meter
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

void setGreenLED(bool on)
{
  pinMode(DPIN_GLED, OUTPUT);
  digitalWrite(DPIN_GLED, on ? HIGH : LOW);
}

void setMainLED(bool on)
{
  pinMode(DPIN_PWR, OUTPUT);
  digitalWrite(DPIN_PWR, on ? HIGH : LOW);
}

void setHighBrightness(bool on)
{
  pinMode(DPIN_DRV_MODE, OUTPUT);
  digitalWrite(DPIN_DRV_MODE, on ? HIGH : LOW);
}

void setBrightnessValue(byte value)
{
  pinMode(DPIN_DRV_EN, OUTPUT);
  analogWrite(DPIN_DRV_EN, value);
}

int getChargeLevel(void)
{
  return(analogRead(APIN_CHARGE));
}

int getTemperature(void)
{
  return(analogRead(APIN_TEMP));
}

int getButtonState(void)
{
  // Pull down the button's pin, since
  // in certain hardware revisions it can float.
  pinMode(DPIN_RLED_SW, OUTPUT);
  pinMode(DPIN_RLED_SW, INPUT);

  return(digitalRead(DPIN_RLED_SW));
}

/* ------------ Common methods that can be reused in other firmwares ----------- */

void blinkOnCharge(unsigned long time)
{
  int chargeLevel = getChargeLevel();

  if (chargeLevel < CHARGE_LEVEL_LOW) // Low charge; blink
    setGreenLED(time & 0x0100);
  else if (chargeLevel > CHARGE_LEVEL_HIGH) // Full charge: steady
    setGreenLED(true);
  else // Hi-Z - shutdown
    setGreenLED(false);
}

void preventOverheating(unsigned long time)
{
  static unsigned long lastTempTime;

  // If it's too early to check the temp, return
  if (time - lastTempTime < TEMP_CHECK_INTERVAL) return;

  lastTempTime = time;
  int temperature = getTemperature();
  Serial.print("Temp: ");
  Serial.println(temperature);

  if (mode == MODE_OFF) return; // We are off, nothing to do
  if (temperature < OVERTEMP) return; // Temperature nominal, carry on.

  Serial.println("Overheating!");

  for (int i = 0; i < 6; i++)
  {
    setHighBrightness(false);
    delay(100);
    setHighBrightness(true);
    delay(100);
  }
  setHighBrightness(false);

  mode = MODE_LOW;
}

void setup()
{
  // We just powered on!  That means either we got plugged 
  // into USB, or the user is pressing the power button.
  setMainLED(false);

  // Initialize GPIO
  setHighBrightness(false);
  setBrightnessValue(0);
  
  // Initialize serial busses
  Serial.begin(9600);
  Wire.begin();
  
  btnTime = millis();
  btnDown = getButtonState();
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
      setBrightnessValue(0);
    else
      setBrightnessValue(255);
    break;
  }
  
  // Check for mode changes
  byte newMode = mode;
  byte newBtnDown = getButtonState();
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
      setMainLED(false);
      setHighBrightness(false);
      setBrightnessValue(0);
      break;
    case MODE_LOW:
      Serial.println("Mode = low");
      setMainLED(true);
      setHighBrightness(false);
      setBrightnessValue(64);
      break;
    case MODE_MED:
      Serial.println("Mode = medium");
      setMainLED(true);
      setHighBrightness(false);
      setBrightnessValue(255);
      break;
    case MODE_HIGH:
      Serial.println("Mode = high");
      setMainLED(true);
      setHighBrightness(true);
      setBrightnessValue(255);
      break;
    case MODE_BLINKING:
    case MODE_BLINKING_PREVIEW:
      Serial.println("Mode = blinking");
      setMainLED(true);
      setHighBrightness(true);
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

