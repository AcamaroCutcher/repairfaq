// Controller firmware for Sam's Stabilized HeNe Laser 3.  Compatible with SP 117/A and MG STP-901 laser heads.
// V01 - Modified Atmega Nano 3.0 code for Pro Micro ATMEGA32U4

// Signals:

//   P-mode input: A0
//   S-mode input: A1
//   External modulation A2
//   Intensity input: A3

//   Frequency/Intensity lock select: (2) (SCL for I2C, not used)
//   RTCInterruptPin: (3) (SDA for I2C, not used)
//   Locked blue LED: (4)
//   P-mode green LED: (5) (PWM)
//   S-mode orange LED: (6) (PWM)
//   Error bright red LED: (7)
//   State bit 0 LED: (16) (SPI MOSI, not used)
//   State bit 1 LED: (14) (SPI MISO, not used)
//   State bit 2 LED: (15) (SPI SCLK, not used)
//   Heater drive, yellow LED: (9) PWM
//   Heartbeat LED: (10)

// Implementation: Preheat and track Pmode and Smode finding min and max; Wait until period < 20 seconds; Locking.
// Real time clock provided using internal counter to generate interrupt which increments TimeAccumulator.

int FirmwareVersion = 103;
int EEPROMFormatValid = 103; // Magic number in UCB to confirm valid EEPROM/UCB format

#include <avr/io.h>
#include <avr/interrupt.h>
#include <EEPROM.h>

#define PmodeInputPin 0  // Analog pin 0 green mode
#define SmodeInputPin 1  // Analog pin 1 red (or orange) mode
#define ExtMod/Int    2  // External modulation/intensity (future)

#define RTClockOut 3     // Real time clock output and interrupt
#define FreqIntSelect 2  // Frequency-/Intsnsity locking selection (future)
#define LockedLED 4      // Lock status for system without any other indicators or GUI
#define StateBit0LED 16  // State LEDs: Idle: 0, Startup: 1, Warmup: 2, Locking: 3, Locked: 4, Error: 7.
#define StateBit1LED 14
#define StateBit2LED 15
#define HeaterDrive 9    // Transistor drive signal
#define PmodeLED 5       // Green LED tracks Pmode input - offset
#define SmodeLED 6       // Red LED tracks Smode input - osffset
#define ErrorLED 7       // Error status
#define HeartBeatLED 10  // On-board LED

#define RTClockFrequency    980
#define RTClockFrequencyD10  98

// display values to send

struct dataPoint                // stucture for one data packet; The FIFO queue is a queue of these structs
  {
    int displayPmode;           // P-Mode 0-1023
    int displaySmode;           // P-Mode 0-1023
    int displayHeater;          // Heater drive 0-255
    long displaySequenceNumber; // Sequence number
    int displayLowSpeedCode;    // Select type of low speed data
    long displayLowSpeedData;   // Low speed data
  };

#define FIFO_SIZE 25 // size of circular buffer

struct dataPoint dataPoint[FIFO_SIZE]; // circular buffer of data to be sent to USB

volatile int head = 0;  // beginning of circular buffer; empty when both head and tail are the same value
volatile int tail = 0;  // end of circular buffer; full when tail is one slot before head we are full

static char outbuffer[300];  // Space for the routine to convert numbers to text to send
static char inbuffer[50];    // Space text received from GUI

static int bufferOverflowCounter = 0;

static int LowSpeedCode = 0;
static long LowSpeedData = 0;
static long SequenceNumber = 0;
static int LowSpeedCodeSelect = 0;

// volatile long StateParams[2][32];   // State-specific parameters  for defaults and user

volatile int State = 0;               // 0: Startup, 1: Warmup, 2: Heating, 3: Locking, 4: Cooling, 5: Locked, 6: Hangout, 7: Error.
int Pmode = 0;                        // P-polarized mode signal
long PmodeLong = 0;                   // 32 bit Pmode
static int PreviousPmode = 0;
int Smode = 0;                        // S-polarized mode signal
long SmodeLong = 0;                   // 32 bit Smode
static int PreviousSmode = 0;

// LCB - Lock Control Block

volatile int LCB[32]; // Active Locking Control Block
volatile int DCB[32]; // Default Locking Control Block - These do not change
volatile int UCB[32]; // User Locking Control Block - These will start off being the same as DCB

// LCB/DCB/UCB format (Durations are in 10ths of seconds; Heater Drive values will be 0-255 for 0-100%; Voltages are 0-1023 for 0-5V; All else are integers

#define EEPROMFormat 0
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

volatile long LockTime = 0;            // Time when laser goes to state 5 in 0.1 s increments

volatile long RTClockTicks1 = 0;       // RTC LSB counter 1
volatile long TotalTime = 0;           // Total time in 0.1 s increments from reset
volatile long TotalTotalTime = 0;      // Total time in RT clock ticks from reset
volatile long CurrentTime = 0;         // Saved current time in seconds 
volatile int TotalTimeTickFlag = 0;    // Set to 1 if TotalTime crossed a 0.1 s boundary
volatile long TimeStamp = 0;           // Saved current time in seconds

int i = 0;
int j = 0;
int k = 0;

int Temp1 = 0;
int Temp2 = 0;

volatile int Command = 0;
volatile int Param1 = 0;
volatile int Param2 = 0;
volatile int Param3 = 0;

volatile int Value0 = 0;
volatile int Value1 = 0;
volatile int Value2 = 0;
volatile int Value3 = 0;

volatile int nda;
volatile bool NewCommand = false;
volatile bool ReadbackLCB = false;
static int ReadbackIndex = -1;
static int UCBValid = 0;

void setup()
{
//  initTimerCounter1();

// Specify pin functions

  pinMode(RTClockOut, OUTPUT);   // Real time clock interrupt generator pin 3 - constant 980 Hz
  pinMode(FreqIntSelect, INPUT); // Frequency-/Intensity lock select 2
  pinMode(LockedLED, OUTPUT);    // Lock LED for systems without any other indicators or GUI pin 4
  pinMode(StateBit0LED, OUTPUT); // State bit 0 pin 16
  pinMode(StateBit1LED, OUTPUT); // State bit 1 pin 14
  pinMode(StateBit2LED, OUTPUT); // State bit 2 pin 15
  pinMode(HeaterDrive, OUTPUT);  // Heater, yellow LED pin 9
  pinMode(PmodeLED, OUTPUT);     // P-mode red LED pin 5
  pinMode(SmodeLED, OUTPUT);     // S-mode green LED pin 6
  pinMode(ErrorLED, OUTPUT);     // Error LED pin 7
  pinMode(HeartBeatLED, OUTPUT); // HeartBeat LED pin 10

// Initialize pins

  digitalWrite(LockedLED, LOW);
  digitalWrite(StateBit0LED, LOW);
  digitalWrite(StateBit1LED, LOW);
  digitalWrite(StateBit2LED, LOW);
  digitalWrite(HeaterDrive, LOW);
  digitalWrite(PmodeLED, LOW);
  digitalWrite(SmodeLED, LOW);
  digitalWrite(ErrorLED, LOW);
  digitalWrite(HeartBeatLED, LOW);

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

UCBValid = 0;
EEPROM.get(0, i);
if (i == EEPROMFormatValid) // This value indicates data is valid (First firmware rev this format)
  {
    for (i = 0;i < 32; i++)
      {
        EEPROM.get(i<<1, LCB[i]); // Address in bytes for EEPROM.get regardless of datatype for value
      }
    UCBValid = 1;
  }
else
  { 
    for (i = 0;i < 32; i++)     // Load Default Locking Parameters
      {
        LCB[i] = DCB[i];
      }
  }
  
// Initial values

  State = 0;
  Duration = LCB[Duration0];
  HeaterValue = LCB[HeaterValue0];

//  Set up real time clock to squarewave at 490 Hz, interrupt at 980 Hz

  attachInterrupt(0, RTCInterrupt, CHANGE); 
  analogWrite(RTClockOut,128);

// Open USB serial port

Serial.begin(57600); // Buad rate (probably doesn't matter for USB)
//Serial.begin(115200); // Buad rate (probably doesn't matter for USB)
}

void RTCInterrupt()
  {
     RTClockTicks1++;
     TotalTotalTime++;
     
     if (RTClockTicks1 >= RTClockFrequencyD10)
       {
         TotalTime++;
         TotalTimeTickFlag = 1;
         RTClockTicks1 = 0;
         digitalWrite(HeartBeatLED, !digitalRead(HeartBeatLED)); // Heartbeat 5 Hz
       }

    interrupts(); // Enable interrupts to permit UART to function

 // Acquire and monitor polarized modes, and compute Mode Error

    PreviousPmode = Pmode;
    PreviousSmode = Smode;
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

// Loop Difference LED update

    if (LoopDifference > 0) analogWrite(PmodeLED,(LoopDifference >> 2)); // Green LED
    else analogWrite(PmodeLED,(0));

    if (LoopDifference < 0) analogWrite(SmodeLED,(-LoopDifference >> 2)); // Red LED
    else analogWrite(SmodeLED,(0));

// Mode LED update - substitute for the ones below if mode instead of loop difference display is preferred

//    analogWrite(PmodeLED,(Pmode >> 2)); // Green LED
//    analogWrite(SmodeLED,(Smode >> 2)); // Red LED

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
        digitalWrite(ErrorLED, HIGH); 
        State = 7;
      }
    break;

// Warmup - State 1 - Increase temperature of laser tube while monitoring mode sweep rate

   case 1:

Value1 = TotalTime - TimeStamp;
Value2 = LoopDifference;
Value3 = LoopDifferenceStamp;

    if (LoopDifference < 0)

//    if ((LoopDifference < 0) && (PreviousLoopDifference >= 0)) // Zero crossing + to -
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
        digitalWrite(ErrorLED, HIGH); 
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
        digitalWrite(ErrorLED, HIGH); 
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
        digitalWrite(ErrorLED, HIGH); 
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
        digitalWrite(ErrorLED, HIGH); 
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
        digitalWrite(ErrorLED, LOW); 
        State = 1; // Laser is on, go to Warmup
      }
    else if ((TotalTime - CurrentTime) > Duration)
      {
        NoLaserLight = 1; // Laser does not turn on but let's be optimistic! :)
        CurrentTime = TotalTime;
        Duration = LCB[Duration0];
        HeaterValue = LCB[HeaterValue0];
        digitalWrite(ErrorLED, LOW); 
        State = 0; // Try again
      }
    break;    
    }  

    analogWrite(HeaterDrive,HeaterValue); // Load heater drive PWM

// End of control code

// Update COMM data if necessary
  
    noInterrupts();

    if ((TotalTotalTime & 0xf) == 0xf) // Update queue on every 16th interrupt, 1/61.25th of a second
      {
        LowSpeedCode = 0;
        LowSpeedData = 0;
        SequenceNumber++;
        LowSpeedCodeSelect = SequenceNumber & 0xf;

        if (ReadbackLCB == false)
          {
            if (LowSpeedCodeSelect == 0) // Send time (redundant, serial number also is time but....)
              {
                LowSpeedCode = 12;
                LowSpeedData = TotalTime;
              }
            else if (LowSpeedCodeSelect == 1) // Send sample frequency
              {
                LowSpeedCode = 8;
                LowSpeedData = 6125;
              }
            else if (LowSpeedCodeSelect == 2) // Send firmware version
              {
                LowSpeedCode = 10;
                LowSpeedData = FirmwareVersion;
              }
            else if (LowSpeedCodeSelect == 3) // Send state
              {
                LowSpeedCode = 11;
                LowSpeedData = State;
              }
            else if (LowSpeedCodeSelect == 4) // Send lock time
              {
                LowSpeedCode = 15;
                LowSpeedData = LockTime;
              }
            else if (LowSpeedCodeSelect == 5) // Send command
              {
                LowSpeedCode = 101;
                LowSpeedData = Command;
              }
            else if (LowSpeedCodeSelect == 6) // Send parameter 1
              {
                LowSpeedCode = 102;
                LowSpeedData = Param1;
              }
            else if (LowSpeedCodeSelect == 7) // Send parameter 2
              {
                LowSpeedCode = 103;
                LowSpeedData = Param2;
              }
            else if (LowSpeedCodeSelect == 8) // Send parameter 3
              {
                LowSpeedCode = 104;
                LowSpeedData = Param3;
              }
            else if (LowSpeedCodeSelect == 9)  // Send Diagnostic Value 0
              {
                LowSpeedCode = 110;
                LowSpeedData = Value0;
              }
            else if (LowSpeedCodeSelect == 10) // Send Diagnostic Value 1
              {
                LowSpeedCode = 111;
                LowSpeedData = Value1;
              }
            else if (LowSpeedCodeSelect == 11) // Send Diagnostic Value 2
              {
                LowSpeedCode = 112;
                LowSpeedData = Value2;
              }
            else if (LowSpeedCodeSelect == 12) // Send Diagnostic Value 3
              {
                LowSpeedCode = 113;
                LowSpeedData = Value3;
              }
            else if (LowSpeedCodeSelect == 13) // UCB status 1 if valid
              {
                LowSpeedCode = 105;
                LowSpeedData = UCBValid;
              }
          }   

        else // Read back entire LCB
          {
            LowSpeedCode = ReadbackIndex + 130;
            LowSpeedData = LCB[ReadbackIndex];
        
            if (ReadbackIndex == 31)
              {
                 ReadbackLCB = false;
                 ReadbackIndex = -1;
              }
            else
              {
                ReadbackIndex++;
              }
          }
              
        if (!(((0==tail)&&((FIFO_SIZE-1)==head))||(head==(tail-1)))) // check if buffer is full; either tail is at beginning of array and head is at end, or head is right before tail
          {
// head points to the element to store next data

            dataPoint[head].displayPmode = Pmode;
            dataPoint[head].displaySmode = Smode;
            dataPoint[head].displayHeater = HeaterValue;
            dataPoint[head].displaySequenceNumber = SequenceNumber;
            dataPoint[head].displayLowSpeedCode = LowSpeedCode;
            dataPoint[head].displayLowSpeedData = LowSpeedData;

// increment head
            if ((FIFO_SIZE-1)==head)
              head = 0;
            else
              head = head +1;
          }
        else
          {
            bufferOverflowCounter++;  // buffer is full. Increment counter
          }
      }
   interrupts();
} 
static String readString;   //main captured String

volatile char c;

volatile int ind1;
volatile int ind2;
volatile int ind3;
volatile int ind4;

volatile int sn;

void loop()
  { 
    if ((State & 0x1) == 0x1) digitalWrite(StateBit0LED, HIGH);
    else digitalWrite(StateBit0LED, LOW);
    if ((State & 0x2) == 0x2) digitalWrite(StateBit1LED, HIGH);
    else digitalWrite(StateBit1LED, LOW);
    if ((State & 0x4) == 0x4) digitalWrite(StateBit2LED, HIGH);
    else digitalWrite(StateBit2LED, LOW);

    if (State == 5) digitalWrite(LockedLED, HIGH);
    else digitalWrite(LockedLED, LOW);

    if (State == 7) digitalWrite(ErrorLED, HIGH);
    else digitalWrite(ErrorLED, LOW);
    
    int length = 0;
    if (head != tail) // only send a new data packet if the queue is not empty; probably superfluous since serial.println blocks
      {
       noInterrupts();

        length += sprintf(outbuffer,"%d ",dataPoint[tail].displayPmode);
        length += sprintf(outbuffer+length,"%d ",dataPoint[tail].displaySmode);
        length += sprintf(outbuffer+length,"%d ",dataPoint[tail].displayHeater);
        length += sprintf(outbuffer+length,"%lu ",dataPoint[tail].displaySequenceNumber);
        length += sprintf(outbuffer+length,"%d ",dataPoint[tail].displayLowSpeedCode);
        length += sprintf(outbuffer+length,"%ld",dataPoint[tail].displayLowSpeedData);

// update queue
        if ((FIFO_SIZE-1)==tail)  // check for wrap around
          tail = 0;               // update tail location
        else  
          tail = tail +1;         // update tail location        
  
        interrupts();

// Send data
        Serial.println(outbuffer);
      }

// Read commands from GUI - expect a string: Command:Param1:Param2:Param3*

    nda = Serial.available();
    if (nda > 0)
      {
        c = Serial.read();  // gets one byte from serial buffer
        if (c == '*') 
          {       // do stuff
            ind1 = readString.indexOf(':');                               //finds location of first :
            Command = (readString.substring(0, ind1)).toInt();        //captures first data String
            ind2 = readString.indexOf(':', ind1+1);                       //finds location of second :
            Param1 = (readString.substring(ind1+1, ind2+1)).toInt();      //captures second data String
            ind3 = readString.indexOf(':', ind2+1);
            Param2 = (readString.substring(ind2+1, ind3+1)).toInt();
            ind4 = readString.indexOf(':', ind3+1);
            Param3 = (readString.substring(ind3+1, ind4+1)).toInt();
            NewCommand = true;
                     
            readString = "";  //clears variables for new input
          } 
        else
          {     
            readString += c; //makes the string readString
          }
      }

    if (NewCommand == true)
      {
        if ((Command >= 10) && (Command < 18)) // State parameter command
          {           
            switch (Command - 10)
              {
                case 0:
                  {
                    LCB[Duration0] = Param1;
                  }
                break;
        
                case 1:
                  {
                    LCB[Duration1] = Param1;
                    LCB[ModePeriod] = Param2;
                  }
                break;

                case 2:
                  {
                    LCB[Duration2] = Param1;
                    LCB[HeaterValue2] = Param2;
                  }
                break;
    
                case 3:
                  {
                    LCB[Duration3]  = Param1;
                    LCB[LockingTolerance] = Param2;
                  }
                break;

                case 4:
                  {
                    LCB[Duration4] = Param1;
                    LCB[HeaterValue4] = Param2;
              }
                break;

                case 5:
                  {
                    LCB[Duration5] = Param1;
                    LCB[ReLockCounter] = Param2;
                  }
                break;

                case 6:
                  {
                    LCB[Duration6] = Temp1;
                    LCB[HeaterValue6] = Param2;
                  }
                break;

                case 7:
                  {
                    LCB[Duration7] = Temp1;
                  }
                break;
              }
          }
          
        else if ((Command >= 20) && (Command < 28)) // State change command
          {
            State = Command - 20;
            CurrentTime = TotalTime;
            TimeStamp = TotalTime;
            AverageLoopDifference = 0;
            Duration = Param1;
           
            switch (Command - 20)
              {
                case 0:
                  {
                    LCB[Duration0] = Duration;
                    HeaterValue = LCB[HeaterValue0];
                  }
                break;
        
                case 1:
                  {
                    LCB[Duration1] = Duration;
                    LCB[ModePeriod] = Param2;
                    HeaterValue = LCB[HeaterValue1];
                  }
                break;

                case 2:
                  {
                    LCB[Duration2] = Duration;
                    HeaterValue = Param2;
                    LCB[HeaterValue2] = HeaterValue;
                  }
                break;
    
                case 3:
                  {
                    LoopDifferenceStamp = LoopDifference;
                    LCB[Duration3] = Duration;
                    LCB[LockingTolerance] = Param2;
                  }
                break;

                case 4:
                  {
                    LCB[Duration4] = Duration;
                    HeaterValue = Param2;
                    LCB[HeaterValue4] = HeaterValue;
                  }
                break;

                case 5:
                  {
                    LCB[Duration5] = Duration;
                    ReLockCounter = Param2;
                    LCB[ReLockCount] = ReLockCounter;
                    I_Accum = 0;
                    LockTime = TotalTime;
                  }
                break;

                case 6:
                  {
                    LCB[Duration6] = Duration;
                    HeaterValue = Param2;
                    LCB[HeaterValue6] = HeaterValue;
                  }
                break;

                case 7:
                  {
                    LCB[Duration7] = Duration;
                    HeaterValue = LCB[HeaterValue7];
                  }
                break;
              }
          }

        else if ((Command >= 30) && (Command < 50)) // Pmode and Smode limits and PID gains 
          {
        
            switch (Command)
              {
                case 31:
                  {
                    LCB[PmodeMin] = Param1;
                  }
                break;

                case 32:
                  {
                    LCB[PmodeMax] = Param1;
                  }
                break;

                case 33:
                  {
                    LCB[SmodeMin] = Param1;
                  }
                break;

                case 34:
                  {
                    LCB[SmodeMax] = Param1;
                  }
                break;

                case 36:
                  {
                    LCB[ProportionalGain] = Param1;
                  }
                break;

                case 37:
                  {
                    LCB[IntegralGain] = Param1;
                  }
                break;

                case 38:
                  {
                    LCB[DifferentialGain] = Param1;
                  }
                break;

                case 40:
                  {
                    LCB[Offset] = Param1;
                    LCB[Intensity] = Param2;
                  }

                break;
                
                case 41:
                  {
                    LCB[ControlRegister] = Param1;
                    State = 3;
                    CurrentTime = TotalTime;
                    TimeStamp = TotalTime;
                    AverageLoopDifference = 0;
                    LoopDifferenceStamp = LoopDifference;
                    Duration = LCB[Duration3];
                  }
                break;
              }
          }
        else if ((Command >= 10000) && (Command < 11000)) // Data transfer and EEPROM commands
          {
            switch (Command)
              {
                case 10101: // Load Default Parameters: Reload LCB from DCB and readback to GUI
                  {
                    for (i = 0;i < 32; i++)
                      {
                        LCB[i] = DCB[i];
                      }
                    ReadbackIndex = 0;
                    ReadbackLCB = true;
                  }
                break;     

               case 10202: // Load User Parameters: Reload LCB from UCB and readback to GUI
                  {
                    UCBValid = 0;
                    EEPROM.get(0, i);
                    if (i == EEPROMFormatValid) // 1234 indicates data is valid
                      {
                        for (i = 0;i < 32; i++)
                          {
                            EEPROM.get(i<<1, LCB[i]); // Address in bytes for EEPROM.get regardless of datatype for value
                          }
        
                        UCBValid = 1;
                        ReadbackIndex = 0;
                        ReadbackLCB = true;
                      }
                  }
                break;     

                case 10303: // Save User Parameters: Copy LCB to UCB in EEPROM
                  {
                  LCB[0] = EEPROMFormatValid;
                  for (i = 0;i < 32; i++)
                      {
                        EEPROM.update(i<<1, LCB[i] & 0xff);            // Low byte of LCB
                        EEPROM.update((i<<1) + 1, (LCB[i] >> 8) & 0xff); // High byte of LCB
                      }
                  }
                break;
                
                case 10505: // Readback LCB to GUI
                  {
                    ReadbackIndex = 0;
                    ReadbackLCB = true;
                  }
                break;   
              }
          } 
        NewCommand = false;  
    }
  }

