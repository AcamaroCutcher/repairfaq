// Full Range Zeeman Magnet Driver 1
// v01 - Basic implementation
// v02 - Increase PWM frequency, rearrange pins for use with 2 line 16 character LCD
// v03 - ~4K PWM frequency, 4 turns total.
// v04 - Add Enable, 8 turns total.
// v05 - Compensate for dead zone quirk.
// v07 - Initial LCD display.
// v08 - Released version 1.0 control box. ;-)

// Signals:
//   Encoder: A=D2, B=D3, UP LED=D8, DOWN LED=D11.
//   H-Bridge (PWM): HBridgePos=D9, HBridgeNeg=D10.

#define FirmwareVersion 108

#include <LiquidCrystal.h>

const int numRows= 2;
const int numCols= 16;

LiquidCrystal lcd(12, 13, 4, 5, 6, 7);

#define LEDWidth 25

#define A           2  // D2 Encoder input A
#define B           3  // D3 Encoder input B
#define UP          8  // Up LED (was 4)
#define DOWN        11 // Down LED (was 5)

// H-Bridge Drive.  May want logical OR (via diodes) of 9 and 10 to H-Bridge ENABLE.  Without this, coil freewheeling 
//                   when not driven, which may be what we want.

#define HBridgePOS  9  // D9 H-Bridge positive drive
#define HBridgeNEG 10  // D10 H-Bridge negative drive

#define Resolution  4  // Results in 4 total turns
#define DeadZone 16    // Low current dead zone

// Encoder input definitions and interrupt routines 

bool oldPin1A = LOW;
bool oldPin1B = LOW;
bool newPin1A = LOW;
bool newPin1B = LOW;
bool Changed = HIGH;
int Error1 = 0;
int MaxCount = (256-DeadZone << Resolution) - 1; // Total +/- counts
int Value  = 0;
float DisplayValue = 0;
float Percent = 0;
int PWMValue = 0;
int LEDCount1 = 0;

float CurrentScale = 1400.0;
float GaussScale = 130.0;
float GaussZeroI = 268.0;
float GaussValue = 0;
int GaussInt = 0;

int Enable = A0;        // Front panel switch 5 V = enable
int CurrentSense = A1;  // Approximately 4.8 V / 3 A

void encoderupdate1A() {
  oldPin1A = newPin1A;
  oldPin1B = newPin1B;
  Changed = HIGH;

  newPin1A = digitalRead(A);
  newPin1B = digitalRead(B);

  if (oldPin1A == oldPin1B)
    {
      Value++;       // Increment position.
      if (Value > MaxCount) Value = MaxCount;
      digitalWrite(UP, HIGH);
      LEDCount1 = LEDWidth;
    }
  else
    {
      Value--;       // Decrement position.
      if (Value < -MaxCount) Value = -MaxCount;
      digitalWrite(DOWN, HIGH);
      LEDCount1 = LEDWidth;
    }
 }

void encoderupdate1B() {
  oldPin1A = newPin1A;
  oldPin1B = newPin1B;
  Changed = HIGH;

  newPin1A = digitalRead(A);
  newPin1B = digitalRead(B);

  if (oldPin1A == oldPin1B)
    {
      Value--;       // Decrement position.
      if (Value < -MaxCount) Value = -MaxCount;
      digitalWrite(DOWN, HIGH);
      LEDCount1 = LEDWidth;
    }
  else
    {
      Value++;       // Increment position.
      if (Value > MaxCount) Value = MaxCount;
      digitalWrite(UP, HIGH);
      LEDCount1 = LEDWidth;
    }
}

void setup()
{
  pinMode(2, INPUT);    // Channel 1 A
  pinMode(3, INPUT);    // Channel 1 B
  pinMode(8, OUTPUT);   // Channel 1 Up
  pinMode(11, OUTPUT);  // Channel 1 Down
  
  pinMode(9, OUTPUT);   // H-Bridge Positive (PWM)
  pinMode(10, OUTPUT);  // H-Bridge Negative (PWM)

  attachInterrupt(0, encoderupdate1A, CHANGE);
  attachInterrupt(1, encoderupdate1B, CHANGE);

  lcd.begin(numCols, numRows);

  lcd.setCursor(0,0);
  lcd.print("Zeemagnet Driver");
  lcd.setCursor(0,1);
  lcd.print("Version: ");
  lcd.print(FirmwareVersion*.01);
  delay(2000);

  lcd.setCursor(0,0);
  lcd.print("I:      A      %");
  lcd.setCursor(0,1);
  lcd.print("Avg Field:     G");
  
// Set PWM frequency to ~31 kHz for pins 9 and 10

 TCCR1B = TCCR1B & 0b11111000 | 1;
}

void loop()
  {
    if (analogRead(Enable) > 511)
      {
        if (Value > 0)
          {
            PWMValue = ((Value >> Resolution) + DeadZone) & 255;
            analogWrite(HBridgePOS,PWMValue); // H-Bridge positive drive
            analogWrite(HBridgeNEG,0);        // H-Bridge negative zeroed
          }
        else if (Value < 0)
          {
            PWMValue = ((-Value >> Resolution) + DeadZone) & 255;
            analogWrite(HBridgeNEG,PWMValue); // H-Bridge negative drive
            analogWrite(HBridgePOS,0);        // H-Bridge positive drive zeroes
          }
        else
          {
            analogWrite(HBridgeNEG,0);  // H-Bridge negative drive zeroed
            analogWrite(HBridgePOS,0);  // H-Bridge positive drive zeroed
          }
      }
    else
      {
        analogWrite(HBridgeNEG,0);  // H-Bridge negative drive zeroed
        analogWrite(HBridgePOS,0);  // H-Bridge positive drive zeroed
      }
    if (LEDCount1 > 0)
      {
        LEDCount1--;
          if (LEDCount1 == 0)
            {
              digitalWrite(UP,LOW);
              digitalWrite(DOWN,LOW);
            }
      }
    if (Changed == HIGH)
      {
        Changed = LOW;

        lcd.setCursor(3,0);
        DisplayValue = Value / CurrentScale; // Current display
        
        if (DisplayValue > 0)
          {
            lcd.print("+");
            lcd.print(DisplayValue);
          }
        else if (DisplayValue < 0)
          {
            lcd.print(DisplayValue);
          }
        else lcd.print(" 0.00");

        lcd.setCursor(10,0); // PWM percent 
        Percent = (100.0 * Value) / MaxCount;

        if (Percent < 0) Percent = -Percent;
        
        if ((Percent < 99.95) && (Percent >= 10.0))
          {
             lcd.print(" ");
             lcd.print(Percent,1);
          }
        else if ((Percent < 9.95) && (Percent >= 0.0))
          {
             lcd.print("  ");
             lcd.print(Percent,1);
          }
        else if (Percent == 100.0)
          {
            lcd.print("100.0");
          }
        
        lcd.setCursor(11,1); // Gauss display
        GaussValue = GaussZeroI + (DisplayValue * GaussScale);
        GaussInt = GaussValue;
        if (GaussInt >= 100)
          {
            lcd.print("+");
            lcd.print(GaussInt);
          }
        else if (GaussInt >= 10)
          {
            lcd.print(" +");
            lcd.print(GaussInt);
          }
        else if (GaussInt > 0)
          {
            lcd.print("  +");
            lcd.print(GaussInt);
          }
        else if (GaussInt == 0)
          {
            lcd.print("   0");
          }
        else if (GaussInt <= -100)
          {
            lcd.print("");
            lcd.print(GaussInt);
          }
        else if (GaussInt <= -10)
          {
            lcd.print(" ");
            lcd.print(GaussInt);
          }
        else
          {
            lcd.print("  ");
            lcd.print(GaussInt);   
          }           
        }
        
 
}

