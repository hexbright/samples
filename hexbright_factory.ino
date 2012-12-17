/* 

  Factory firmware for HexBright FLEX 
  v2.4  Dec 6, 2012
  
  CHANGELOG :
  
  Dec 16, 2012
    * Added quick power off in all states. Press and hold pwr button to turn off flashlight.
    * Added SOS to blinky mode. To get there: press and hold pwr button to get blinky, then short press button again for SOS
  
*/

#include <math.h>
#include <Wire.h>

// Settings
#define OVERTEMP                340

// Pin assignments
#define DPIN_RLED_SW            2
#define DPIN_GLED               5
#define DPIN_PWR                8
#define DPIN_DRV_MODE           9
#define DPIN_DRV_EN             10
#define APIN_TEMP               0
#define APIN_CHARGE             3

// Modes
#define MODE_OFF                0
#define MODE_LOW                1
#define MODE_MED                2
#define MODE_HIGH               3
#define MODE_BLINKING           4
#define MODE_BLINKING_PREVIEW   5
#define MODE_SOS                6

#define MODE_SOS_S              0
#define MODE_SOS_O              1
#define MODE_SOS_S_             2

// State
byte mode = 0;
byte sos_mode = 0;
unsigned long btnTime = 0;
boolean btnDown = false;

void setup()
{
  // We just powered on!  That means either we got plugged 
  // into USB, or the user is pressing the power button.
  pinMode(DPIN_PWR,      INPUT);
  digitalWrite(DPIN_PWR, LOW);

  // Initialize GPIO
  pinMode(DPIN_RLED_SW,  INPUT);
  pinMode(DPIN_GLED,     OUTPUT);
  pinMode(DPIN_DRV_MODE, OUTPUT);
  pinMode(DPIN_DRV_EN,   OUTPUT);
  digitalWrite(DPIN_DRV_MODE, LOW);
  digitalWrite(DPIN_DRV_EN,   LOW);
  
  // Initialize serial busses
  Serial.begin(9600);
  Wire.begin();
  
  btnTime = millis();
  btnDown = digitalRead(DPIN_RLED_SW);
  mode = MODE_OFF;
  sos_mode = MODE_SOS_S;

  Serial.println("Powered up!");
}

void loop()
{
  static unsigned long lastTempTime;
  static unsigned long ditdah;
  static int ledState = LOW;
  
  unsigned long time = millis();
  static unsigned long previousTime = 0;
  
  // Check the state of the charge controller
  int chargeState = analogRead(APIN_CHARGE);
  if (chargeState < 128)  // Low - charging
  {
    digitalWrite(DPIN_GLED, (time&0x0100)?LOW:HIGH);
  }
  else if (chargeState > 768) // High - charged
  {
    digitalWrite(DPIN_GLED, HIGH);
  }
  else // Hi-Z - shutdown
  {
    digitalWrite(DPIN_GLED, LOW);    
  }
  
  // Check the temperature sensor
  if (time-lastTempTime > 1000)
  {
    lastTempTime = time;
    int temperature = analogRead(APIN_TEMP);
    Serial.print("Temp: ");
    Serial.println(temperature);
    if (temperature > OVERTEMP && mode != MODE_OFF)
    {
      Serial.println("Overheating!");

      for (int i = 0; i < 6; i++)
      {
        digitalWrite(DPIN_DRV_MODE, LOW);
        delay(100);
        digitalWrite(DPIN_DRV_MODE, HIGH);
        delay(100);
      }
      digitalWrite(DPIN_DRV_MODE, LOW);

      mode = MODE_LOW;
    }
  }

  // Do whatever this mode does
  switch (mode)
  {
  case MODE_BLINKING:
  case MODE_BLINKING_PREVIEW:
    digitalWrite(DPIN_DRV_EN, (time%300)<75);
    break;
  case MODE_SOS:
    // 200 ms is the frame for each dit "on", the larger this number the slower the SOS
    if (time - previousTime > 200) {
      previousTime = time;   
      switch (sos_mode)
      {     
      case MODE_SOS_S:
        if (ditdah <= 6) {
          ledState = (ledState == LOW) ? ledState = HIGH : ledState = LOW;
          ditdah++;
        }
        else {
          ditdah = 1;
          sos_mode = MODE_SOS_O;
        }
        break;
      case MODE_SOS_O:
        if (ditdah <= 12) {        
          if (ledState == LOW)
            ledState = HIGH;
          else if (ditdah % 4 == 0)
            ledState = LOW;
          ditdah++;
        }
        else {
          ditdah = 1;
          sos_mode = MODE_SOS_S_;
        }
        break;
      case MODE_SOS_S_:
        if (ditdah <= 6) {
          ledState = (ledState == LOW) ? ledState = HIGH : ledState = LOW;
          ditdah++;
        }
        else if (ditdah < 10)
          ditdah++;
        else {
          ditdah = 1;
          sos_mode = MODE_SOS_S;
        }
        break;      
      }
      
      digitalWrite(DPIN_DRV_EN, ledState);
    }
    break;
  }
  
  // Periodically pull down the button's pin, since
  // in certain hardware revisions it can float.
  pinMode(DPIN_RLED_SW, OUTPUT);
  pinMode(DPIN_RLED_SW, INPUT);
  
  // Check for mode changes
  byte newMode = mode;
  byte newBtnDown = digitalRead(DPIN_RLED_SW);
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
    if (btnDown && !newBtnDown && (time-btnTime)>500)
      newMode = MODE_OFF;      
    break;
  case MODE_MED:
    if (btnDown && !newBtnDown && (time-btnTime)>50)
      newMode = MODE_HIGH;
    if (btnDown && !newBtnDown && (time-btnTime)>500)
      newMode = MODE_OFF;           
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
    if (btnDown && !newBtnDown && (time-btnTime)>50) {
      newMode = MODE_SOS;
      sos_mode = MODE_SOS_S;
      // reset ditdah to 1, 1 based due to use of modulo
      ditdah = 1;
    }
    if (btnDown && !newBtnDown && (time-btnTime)>500)
      newMode = MODE_OFF;           
    break;
  case MODE_SOS:    
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
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, LOW);
      digitalWrite(DPIN_DRV_MODE, LOW);
      digitalWrite(DPIN_DRV_EN, LOW);
      break;
    case MODE_LOW:
      Serial.println("Mode = low");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, LOW);
      analogWrite(DPIN_DRV_EN, 64);
      break;
    case MODE_MED:
      Serial.println("Mode = medium");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, LOW);
      analogWrite(DPIN_DRV_EN, 255);
      break;
    case MODE_HIGH:
      Serial.println("Mode = high");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, HIGH);
      analogWrite(DPIN_DRV_EN, 255);
      break;
    case MODE_BLINKING:
    case MODE_BLINKING_PREVIEW:
      Serial.println("Mode = blinking");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, HIGH);
      break;
    case MODE_SOS:
      Serial.println("Mode = SOS");
      pinMode(DPIN_PWR, OUTPUT);
      digitalWrite(DPIN_PWR, HIGH);
      digitalWrite(DPIN_DRV_MODE, HIGH);
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

