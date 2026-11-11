// uSLC1 stabilized laser controller - ATTiny version.  Control ONLY, no GUI, no EEPROM.
// Interrupts loosely based on http://forum.arduino.cc/index.php?topic=163393.0

int FirmwareVersion = 102;

#include <avr/io.h>
#include <avr/interrupt.h>

// Signals:
//   P-mode input: AD1
//   S-mode input: AD2
//   External modulation: AD3
//   Intensity input: AD3

//   Status LED: P1  Absolute Mode Difference (except State 7) and:
//     Startup: One 0.1 s flash every 6.4 s
//     Warmup:  One 0.1 s flash every 1.6 s
//     Heating: Two 0.1 s flashes every 1.6 s
//     Locking: 0.4 s on 0.4 s off
//     Cooling: Four 0.1 s flashes every 1.6 s
//     Locked:  None
//     Hang:    None (No way to get here)
//     Error:   Flashing at 5 Hz

//   Heater drive, yellow LED: P0

// Implementation: Preheat and track Pmode and Smode finding min and max; Wait until period < 20 seconds; Locking.

// Board   Arduino
// Pins    Pins          Notes
// -------------------------------------------------
//   0     P0/MOSI       (PWM)
//   1     P1/MISO       LED (PWM)
//   2     P2/SCK/A1
//   3     P3/A3         (USB D- with 1.5 k pullup)
//   4     P4/A2         (PWM) (USB D+)
//   5     P5/mRST/A0    (3 V max)

#define PmodeInputPin 1 // Green mode
#define SmodeInputPin 2 // Red mode
#define ExtModulation 3 // External modulation (future)
#define IntensitySet 3  // External intensity setting (future)

#define StatusLED 1     // Status LED
#define HeaterDrive 0   // Transistor drive signal

#define RTClockFrequency    980
#define RTClockFrequencyD10  98

void initTimerCounter1(void)
  {
    TCCR1 = 0;                  // stop the timer
    TCNT1 = 0;                  // zero the timer
    GTCCR = _BV(PSR1);          // reset the prescaler//
    OCR1C = 64;                 // set the compare value of 16 kHz/64 = 256 Hz
    TIMSK = _BV(OCIE1A);        // interrupt on Compare Match A

//start timer, ctc mode, prescaler clk
    TCCR1 = _BV(CTC1) | _BV(CS13) | _BV(CS10);  // 1001 for (16 MHz)/256 = 16 kHz
    sei();
  }

// volatile long StateParams[2][32];   // State-specific parameters  for defaults and user

volatile int State = 0;               // 0: Startup, 1: Warmup, 2: Heating, 3: Locking, 4: Cooling, 5: Locked, 6: Hangout, 7: Error.
int Pmode = 0;                        // P-polarized mode signal
long PmodeLong = 0;                   // 32 bit Pmode
int Smode = 0;                        // S-polarized mode signal
long SmodeLong = 0;                   // 32 bit Smode

// LCB - Lock Control Block

volatile int LCB[32]; // Active Locking Control Block
volatile int DCB[32]; // Default Locking Control Block - These do not change

// LCB/DCB/UCB format (Durations are in 10ths of seconds; Heater Drive values will be 0-255 for 0-100%; Voltages are 0-1023 for 0-5V; All else are integers

#define EEPROMFormat 0 // Not used for ATTiny
#define PmodeMin 1
#define SmodeMin 2
#define PmodeMax 3
#define SmodeMax 4
#define ModePeriod 5
#define ControlRegister 6
#define LockingTolerance 7
#define ReLockCount 8
#define ProportionalGain 9
#define IntegralGain 10
#define DifferentialGain 11
#define Duration0 12
#define Duration1 13
#define Duration2 14
#define Duration3 15
#define Duration4 16
#define Duration5 17
#define Duration6 18
#define Duration7 19
#define HeaterValue0 20
#define HeaterValue1 21
#define HeaterValue2 22
#define HeaterValue3 23
#define HeaterValue4 24
#define HeaterValue5 25
#define HeaterValue6 26
#define HeaterValue7 27
#define Spare1 28
#define Offset 29
#define Intensity 30
#define CheckSum 31

// Locking parameters

volatile int Duration = 120;
volatile int HeaterValue = 0;          // Heater value for state 6.
volatile int ReLockCounter = 1;        // Number of times to fine tune lock temperature

volatile long I_Accum = 0;             // Integrator "capacitor"
static int I_Limit = 0;                // Windup limit
static int P_Term = 0;                 // Proportional term
static int I_Term = 0;                 // Integral term
static int D_Term = 0;                 // Differential term
static int Control = 0;                // Controlled variable for heater
static int NoLaserLight = 0;           // Set to 1 if laser does not come on within 60 seconds.
static int LoopDifference = 0;         // Current Pmode-Smode
static int PreviousLoopDifference = 0; // Average Current Pmode-Smode
static int AverageLoopDifference = 0;  // Moving average of Mode Error
static int PreviousAverageLoopDifference = 0; // Previous Average Pmode-Smode
static int LoopDifferenceStamp = 0;    // Time at which Mode Error is sampled
volatile int AbsoluteLoopDifference = 0; // For LED only

volatile long LockTime = 0;            // Time when laser goes to state 5 in 0.1 s increments

volatile long RTClockTicks1 = 0;       // RTC LSB counter 1
volatile long TotalTime = 0;           // Total time in 0.1 s increments from reset
volatile long TotalTotalTime = 0;      // Total time in RT clock ticks from reset
volatile long CurrentTime = 0;         // Saved current time in seconds 
volatile int TotalTimeTickFlag = 0;    // Set to 1 if TotalTime crossed a 0.1 s boundary
volatile long TimeStamp = 0;           // Saved current time in seconds

int i = 0;

int Temp1 = 0;

volatile int Value0 = 0;
volatile int Value1 = 0;
volatile int Value2 = 0;
volatile int Value3 = 0;

void setup() {

  initTimerCounter1();

// Specify pin functions

  pinMode(StatusLED, OUTPUT);    // Status LED
  pinMode(HeaterDrive, OUTPUT);  // Heater, yellow LED pin 9

// Initialize pins

  digitalWrite(StatusLED, LOW);
  digitalWrite(HeaterDrive, LOW);

// Initialize DCB with default values

DCB[0] = 1;
DCB[PmodeMin] = 0;
DCB[SmodeMin] = 0;
DCB[PmodeMax] = 1023;
DCB[SmodeMax] = 1023;
DCB[ModePeriod] = 140;
DCB[LoopDifference] = 0;
DCB[LockingTolerance] = 8;
DCB[ReLockCount] = 1;
DCB[ProportionalGain] = 10;
DCB[IntegralGain] = 10;
DCB[DifferentialGain] = 0;
DCB[Duration0] = 1200;
DCB[Duration1] = 18000;
DCB[Duration2] = 300;
DCB[Duration3] = 100;
DCB[Duration4] = 150;
DCB[Duration5] = 3000;
DCB[Duration6] = 0;
DCB[Duration7] = 1200;
DCB[HeaterValue0] = 0;
DCB[HeaterValue1] = 255;
DCB[HeaterValue2] = 255;
DCB[HeaterValue3] = 127;
DCB[HeaterValue4] = 0;
DCB[HeaterValue5] = 127;
DCB[HeaterValue6] = 0;
DCB[HeaterValue7] = 0;
DCB[Spare1] = 0;
DCB[Offset] = 0;
DCB[Intensity] = 511;
DCB[CheckSum] = 0;

// Copy UCB or DCB to LCB

for (i = 0;i < 32; i++)     // Load Default Locking Parameters
  {
    LCB[i] = DCB[i];
  }
  
// Initial values

  State = 0;
  Duration = LCB[Duration0];
  HeaterValue = LCB[HeaterValue0];
}

ISR(TIMER1_COMPA_vect)
  {
     RTClockTicks1++;
     TotalTotalTime++;
     
     if (RTClockTicks1 >= RTClockFrequencyD10)
       {
         TotalTime++;
         TotalTimeTickFlag = 1;
         RTClockTicks1 = 0;
       }

 // Acquire and monitor polarized modes, and compute Mode Error

    PreviousLoopDifference = LoopDifference;
    PreviousAverageLoopDifference = LoopDifference;

// Mode gain and offset correction

    PmodeLong = analogRead(PmodeInputPin) - LCB[PmodeMin];
    PmodeLong = (PmodeLong * 1024) / (LCB[PmodeMax] - LCB[PmodeMin]);

    SmodeLong = analogRead(SmodeInputPin) - LCB[SmodeMin];
    SmodeLong = (SmodeLong * 1024) / (LCB[SmodeMax] - LCB[SmodeMin]);

    Pmode = PmodeLong;
    Smode = SmodeLong;
    
    if (Pmode < 0) Pmode = 0;
    if (Smode < 0) Smode = 0;

    if (Pmode > 1023) Pmode = 1023;
    if (Smode > 1023) Smode = 1023;

    if ((LCB[ControlRegister] & 0x2) == 0x2) LoopDifference = (Pmode - LCB[Intensity]); 
    else if ((LCB[ControlRegister] & 0x4) == 0x4) LoopDifference = (Smode - LCB[Intensity]); 
    else LoopDifference = (Pmode - Smode) + LCB[Offset];
    
    if (LoopDifference < -1023) LoopDifference = -1023;
    if (LoopDifference >  1023) LoopDifference =  1023;

    if (LoopDifference < 0) AbsoluteLoopDifference = -LoopDifference;
    else AbsoluteLoopDifference = LoopDifference;

// Check for Lock Side

    if ((LCB[ControlRegister] & 0x1) == 0x1) LoopDifference = -LoopDifference; // Red/blue side

    AverageLoopDifference = ((15 * AverageLoopDifference) + LoopDifference) >> 4; // 16 point moving average

// State machine

Value0 = TotalTime - CurrentTime;

    switch(State) {
    
// Startup - State 0 - Check for laser emission
 
    case 0:
    
// HeaterValue = 0; // Heater off

Value1 = 0;
Value2 = 0;
Value3 = 0;

    if (((TotalTime - CurrentTime) > 20) && ((Pmode > 100) || (Smode > 100)))
      {
        NoLaserLight = 0;
        TimeStamp = TotalTime;
        CurrentTime = TotalTime;
        Duration = LCB[Duration1];
        HeaterValue = LCB[HeaterValue1]; 
        ReLockCounter = LCB[ReLockCount];
        State = 1; // Go to warmup
      }
    else if ((TotalTime - CurrentTime) > Duration)
      {
        NoLaserLight = 1; // Laser does not turn on
        CurrentTime = TotalTime;
        Duration = LCB[Duration7];
        HeaterValue = LCB[HeaterValue7];
        State = 7;
      }
    break;

// Warmup - State 1 - Increase temperature of laser tube while monitoring mode sweep rate

   case 1:

Value1 = TotalTime - TimeStamp;
Value2 = LoopDifference;
Value3 = LoopDifferenceStamp;

    if (LoopDifference < 0)
      {
        TimeStamp = TotalTime; // Yes, reset reference time
      }    
    else if ((TotalTime - TimeStamp) > (LCB[ModePeriod] >> 1)) // Mode sweep slow enough to go to locking state?
      {
        LoopDifferenceStamp = LoopDifference;
        CurrentTime = TotalTime;
        Duration = LCB[Duration3];
        HeaterValue = LCB[HeaterValue3];
        State = 3; // Tube hot enough to start locking
      }
    else if ((TotalTime - CurrentTime) > Duration)
      {
        CurrentTime = TotalTime;
        Duration = LCB[Duration7];
        HeaterValue = LCB[HeaterValue7];
        State = 7;
      }
    break;

// Heating - State 2 - Increase temperature of laser tube

    case 2:

Value1 = 0;
Value2 = 0;
Value3 = 0;

    if ((TotalTime - CurrentTime) > Duration)
      {
        LoopDifferenceStamp = 0;
        TimeStamp = TotalTime;
        AverageLoopDifference = 0;
        CurrentTime = TotalTime;
        Duration = LCB[Duration3];
        HeaterValue = LCB[HeaterValue3];
        State = 3;
      }
    break;
    
// Locking - State 3 - Wait until laser locks, then fine tune heater voltage

    case 3:
      
    Temp1 = LoopDifference * LCB[ProportionalGain];
    if (Temp1 < -2048) HeaterValue = 0;
    else if (Temp1 >  2047) HeaterValue = 255;
    else HeaterValue = (Temp1 >> 4) + 128;

Value1 = TotalTime - TimeStamp;
Value2 = AverageLoopDifference;
Value3 = LoopDifferenceStamp;

    if (((AverageLoopDifference - LoopDifferenceStamp) > LCB[LockingTolerance]) || ((AverageLoopDifference - LoopDifferenceStamp) < -LCB[LockingTolerance])) // Is Error large?
      {
        TimeStamp = TotalTime; // It's not locked, reset reference time
        LoopDifferenceStamp = AverageLoopDifference;
      }    
    else if ((Pmode < 51) || (Pmode > 972) || (Smode < 51) || (Smode > 972))
      { // Mode signals out of range
        CurrentTime = TotalTime;
        Duration = LCB[Duration7];
        HeaterValue = LCB[HeaterValue7];
        State = 7;
      }
    else if ((TotalTime - TimeStamp) > Duration) // Is Error within locked bounds for long enough to call it locked
      {
        if (AverageLoopDifference < -LCB[LockingTolerance])
          {
            CurrentTime = TotalTime;
            Duration = LCB[Duration2];
            HeaterValue = LCB[HeaterValue2];
            State = 2; // Heat for fixed time
          }
        else if (AverageLoopDifference > LCB[LockingTolerance])
          {
            CurrentTime = TotalTime;
            Duration = LCB[Duration4];
            HeaterValue = LCB[HeaterValue4];
            State = 4; // Cool for fixed time
          }
        else
          {
            I_Accum = 0;
            PreviousLoopDifference = 0;
            LockTime = TotalTime;
            CurrentTime = TotalTime;
            Duration = LCB[Duration5];
            HeaterValue = LCB[HeaterValue5];
            State = 5; // Locked in Lock State :)
          }
      }
    else if ((TotalTime - CurrentTime) > 3600)
      {
        CurrentTime = TotalTime;
        Duration = LCB[Duration7];
        HeaterValue = LCB[HeaterValue7];
        State = 7;
      }
    break;

// Cooldown - State 4 - Decrease temperature of laser tube

    case 4:
  
Value1 = 0;
Value2 = 0;
Value3 = 0;

if ((TotalTime - CurrentTime) > Duration)
      {
        LoopDifferenceStamp = 0;
        TimeStamp = TotalTime;
        AverageLoopDifference = 0;
        CurrentTime = TotalTime;
        Duration = LCB[Duration3];
        HeaterValue = LCB[HeaterValue3];
        State = 3;
      }
      
    break;

// Locked - State 5 - Stay here forever once laser has locked at 50 percent +/-12.5 percent heater after checking ReLockCount times

    case 5:

Value1 = AverageLoopDifference;
Value2 = I_Accum;
Value3 = ReLockCounter;

// Proportional term

    P_Term = LCB[ProportionalGain] * LoopDifference;

// Integration with I term limiting.
    I_Limit = LCB[LockingTolerance] << 3;
    
    I_Accum += LoopDifference;

    if (I_Accum < -(I_Limit)) I_Accum = -(I_Limit);
    else if (I_Accum > I_Limit) I_Accum = (I_Limit);

    I_Term = LCB[IntegralGain] * I_Accum;
 
// Differentiation
    D_Term = LCB[DifferentialGain] * (LoopDifference - PreviousLoopDifference);
 
// save current error as previous error for next iteration
    PreviousLoopDifference = LoopDifference;

// Summation of terms

    Control = P_Term + I_Term + D_Term;

// Convert to heater PWM and limit
    if (Control < -4096) HeaterValue = 0;
    else if (Control >  4095) HeaterValue = 255;
    else HeaterValue = (Control >> 5) + 128;
  
    if (((TotalTime - CurrentTime) > Duration) && (ReLockCounter > 0)) // Go to locking state to readjust after some time - usually infinity. :)
      { // Time to fine tune lock point
        ReLockCounter--;
        LoopDifferenceStamp = 0;
        TimeStamp = TotalTime;
        AverageLoopDifference = 0;
        CurrentTime = TotalTime;
        Duration = LCB[Duration3];
        State = 3;
      }
    else if ((Pmode < 51) || (Pmode > 972) || (Smode < 51) || (Smode > 972))
      { // Mode signals out of range
        CurrentTime = TotalTime;
        Duration = LCB[Duration7];
        HeaterValue = LCB[HeaterValue7];
        State = 7;
      }
    break;

// Hangout - State 6 - Test hangs in this state

    case 6:

Value1 = 0;
Value2 = 0;
Value3 = 0;

// Nothing to do here except hang out
    
    break;

// Error - State 7 - Currently only if tube never lights
    
    case 7:

Value1 = 0;
Value2 = 0;
Value3 = 0;

if ((analogRead(PmodeInputPin) > 100) || (analogRead(SmodeInputPin) > 100))
      {
        NoLaserLight = 0; // Laser is on
        CurrentTime = TotalTime;
        TimeStamp = TotalTime;
        Duration = LCB[Duration1];
        HeaterValue = LCB[HeaterValue1];
        State = 1; // Laser is on, go to Warmup
      }
    else if ((TotalTime - CurrentTime) > Duration)
      {
        NoLaserLight = 1; // Laser does not turn on but let's be optimistic! :)
        CurrentTime = TotalTime;
        Duration = LCB[Duration0];
        HeaterValue = LCB[HeaterValue0];
        State = 0; // Try again
      }
    break;    
    }  

    analogWrite(HeaterDrive,HeaterValue); // Load heater drive PWM
  }
  
void loop()
{

// Status LED behavior based on State
  
  if (TotalTimeTickFlag == 1)
    {
      TotalTimeTickFlag = 0;
      State = 4;
      switch(State)
        { // AbsoluteLoopDifference with 0.1 s flash every 6.4 s
          case 0:
            {
              if ((TotalTime & 0x3f) == 0x3f) digitalWrite(StatusLED, HIGH);
              else analogWrite(StatusLED, AbsoluteLoopDifference);    
            }  
          break;   

          case 1:
            { // AbsoluteLoopDifference with 0.1 s flash every 1.6 s
              if ((TotalTime & 0xf) == 0) digitalWrite(StatusLED, HIGH);
              else analogWrite(StatusLED, AbsoluteLoopDifference);    
            }  
          break;   

          case 2:
            { // AbsoluteLoopDifference with two 0.1 s flashes every 1.6 s
              if (((TotalTime & 0xc) == 0) && ((TotalTime & 0x1) == 0x1)) digitalWrite(StatusLED, HIGH);
              else analogWrite(StatusLED, AbsoluteLoopDifference);    
            }  
          break;  

          case 3:
            { // AbsoluteLoopDifference with 0.4 s flash every 0.8 s
              if (((TotalTime & 0x4) == 0 )) digitalWrite(StatusLED, HIGH);
              else analogWrite(StatusLED, AbsoluteLoopDifference);    
            }  
          break;  
          
          case 4:
            { // AbsoluteLoopDifference with four 0.1 s flashes every 1.6 s
              if (((TotalTime & 0x8) == 0) && ((TotalTime & 0x1) == 0x1)) digitalWrite(StatusLED, HIGH);
              else analogWrite(StatusLED, AbsoluteLoopDifference);    
            }  
          break;  

          case 5:
            { // AbsoluteLoopDifference
              analogWrite(StatusLED, AbsoluteLoopDifference);    
            }  
          break;  

          case 6:
            { // AbsoluteLoopDifference
              analogWrite(StatusLED, AbsoluteLoopDifference);    
            }  
          break;  

          case 7:
            { // Flashing at 5 Hz
              if ((TotalTime & 0x1) == 0x1) digitalWrite(StatusLED, HIGH);
              else digitalWrite(StatusLED, LOW);    
            }  
          break;  
       }
  }
}
