// Bipolar PWM Driver Demo Firmware

// Signals:
//   Encoder: A=D2, B=D3, UP LED=D8, DOWN LED=D11.
//   H-Bridge (PWM): HBridgePos=D9, HBridgeNeg=D10.

//   B (digital pin 8 to 13)
//   C (analog input pins)
//   D (digital pins 0 to 7) 
    
#define FirmwareVersion 1.03
#define LEDWidth 5

#define Pin1A 4        // D2 Encoder input A
#define Pin1B 8        // D3 Encoder input B

#define Up 1           // D8 Up LED
#define Down 8         // D11 Down LED
#define Error 64       // D13 On-Board LED

#define HBridgePOS  9  // D9 H-Bridge positive drive
#define HBridgeNEG 10  // D10 H-Bridge negative drive

#define Resolution  4  // Results in 4 total turns

int MaxCount = (256 << Resolution) - 1; // Total +/- counts
int Value  = 0;
int PWMValue = 0;
int LEDCount1 = 0;

int oldState1 = 0;
int newState1 = 0;
int Changed = 1;

void setup()
 {
   pinMode(2, INPUT);    // Channel 1 A
   pinMode(3, INPUT);    // Channel 1 B
   pinMode(8, OUTPUT);   // Channel 1 Up
   pinMode(11, OUTPUT);  // Channel 1 Down
   pinMode(9, OUTPUT);   // H-Bridge Positive (PWM)
   pinMode(10, OUTPUT);  // H-Bridge Negative (PWM)

   PORTB = 0;     // Reset LEDs.
   newState1 = 0; // Initial State
 }

void loop ()
 {
   // Quadrature decoder.
   
   oldState1 = newState1;
   newState1 = PIND & (Pin1A | Pin1B);

   if (oldState1 == 0)
     {
       if (newState1 == 0) {}        // No change.
       else if (newState1 == Pin1A)
         {
           Value++;       // Increment position.
           if (Value > MaxCount) Value = MaxCount;
           LEDCount1 = LEDWidth;
           PORTB = Up;
           Changed = 1;
         }
       else if (newState1 == Pin1B)
         {
           Value--;       // Decrement position.
           if (Value < -MaxCount) Value = -MaxCount;
           LEDCount1 = LEDWidth;
           PORTB = Down;
           Changed = 1;
         }
       else  PORTB = Error;   // Set Error LED.
     }
   else if (oldState1 == Pin1A)
     {
       if (newState1 == Pin1A) {}        // No change.
       else if (newState1 == 0)
         {
           Value--;       // Decrement position.
           if (Value < -MaxCount) Value = -MaxCount;
           LEDCount1 = LEDWidth;
           PORTB = Down;
           Changed = 1;
         }
       else if (newState1 == (Pin1A | Pin1B))
         {
           Value++;       // Increment position.
           if (Value > MaxCount) Value = MaxCount;
           LEDCount1 = LEDWidth;
           PORTB = Up;
           Changed = 1;
         }
       else  PORTB = Error;   // Set Error LED.
     }
   else if (oldState1 == Pin1B)
     {
       if (newState1 == Pin1B) {}        // No change.
       else if (newState1 == 0)
         {
           Value++;       // Increment position.
           if (Value > MaxCount) Value = MaxCount;
           LEDCount1 = LEDWidth;
           PORTB = Up;
           Changed = 1;  
         }
       else if (newState1 == (Pin1A | Pin1B))
         {
           Value--;       // Decrement position.
           if (Value < -MaxCount) Value = -MaxCount;
           LEDCount1 = LEDWidth;
           PORTB = Down;
           Changed = 1;
         }
       else  PORTB = Error;   // Set Error LED.
     }
   else if (oldState1 == (Pin1A | Pin1B))
     {
       if (newState1 == (Pin1A | Pin1B)) {}        // No change.
       else if (newState1 == Pin1B)
         {
           Value++;       // Increment position.
           if (Value > MaxCount) Value = MaxCount;
           LEDCount1 = LEDWidth;
           PORTB = Up;
           Changed = 1;
         }
       else if (newState1 == Pin1A)
         {
           Value--;       // Decrement position.
           if (Value < -MaxCount) Value = -MaxCount;
           LEDCount1 = LEDWidth;
           PORTB = Down;
           Changed = 1;
         }
       else PORTB = Error;   // Set Error LED.
     }
    
    // Up/down LED for demo version.

    if (LEDCount1 == 0)
      {
      	PORTB = 0;
      }
    else LEDCount1--;
    
    // Update PWM Outputs.
 
    if (Changed != 0)
      {
        if (Value >= 0)
          {
            PWMValue = ((Value >> Resolution)) & 255;
            analogWrite(HBridgePOS,PWMValue); // H-Bridge positive drive
            analogWrite(HBridgeNEG,0);        // H-Bridge negative zeroed
          }
        else
          {
            PWMValue = ((-Value >> Resolution)) & 255;
            analogWrite(HBridgeNEG,PWMValue); // H-Bridge negative drive
            analogWrite(HBridgePOS,0);        // H-Bridge positive drive zeroes
          }
      }
 }



