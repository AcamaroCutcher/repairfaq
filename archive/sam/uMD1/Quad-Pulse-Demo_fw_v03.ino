// Quadrature to pulse converter front-end
// v01 - Basic implementation
// Signals:
//   Channel 1: A=D2, B=D3, Up=D4, Down=D5.
//   Channel 2: A=D6, B=D7, Up=D8, Down=D9.
//   Channel 3: A=D10, B=D11, UP=D12, Down=D13.

#define FirmwareVersion 0103

#define LEDWidth 25

#define Channel1 // Enable Channel 1 if defined
// #define Channel2 // Enable Channel 2 if defined
// #define Channel3 // Enable Channel 3 if defined

#ifdef Channel1

// Channel 1 definitions and interrupt routines 

bool oldPin1A = LOW;
bool oldPin1B = LOW;
bool newPin1A = LOW;
bool newPin1B = LOW;
int Error1 = 0;
long Channel1Position  = 0;
int LEDCount1 = 0;

void encoderupdate1A() {
  oldPin1A = newPin1A;
  oldPin1B = newPin1B;

  newPin1A = digitalRead(2);
  newPin1B = digitalRead(3);

  if (oldPin1A == oldPin1B)
    {
      Channel1Position++;       // Increment position.
      digitalWrite(4, LOW);
      digitalWrite(4, HIGH);
      LEDCount1 = LEDWidth;
    }
  else
    {
      Channel1Position--;       // Decrement position.
      digitalWrite(5, LOW);
      digitalWrite(5, HIGH);
      LEDCount1 = LEDWidth;
    }
 }

void encoderupdate1B() {
  oldPin1A = newPin1A;
  oldPin1B = newPin1B;

  newPin1A = digitalRead(2);
  newPin1B = digitalRead(3);

  if (oldPin1A == oldPin1B)
    {
      Channel1Position--;       // Decrement position.
      digitalWrite(5, LOW);
      digitalWrite(5, HIGH);
      LEDCount1 = LEDWidth;
    }
  else
    {
      Channel1Position++;       // Increment position.
      digitalWrite(4, LOW);
      digitalWrite(4, HIGH);
      LEDCount1 = LEDWidth;
    }
}
#endif

#ifdef Channel2

// Channel 2 definitions and interrupt routines 

bool oldPin2A = LOW;
bool oldPin2B = LOW;
bool newPin2A = LOW;
bool newPin2B = LOW;
int Error2 = 0;
long Channel2Position  = 0;
int LEDCount2 = 0;
#endif

#ifdef Channel3

// Channel 3 definitions and interrupt routines 

bool oldPin3A = LOW;
bool oldPin3B = LOW;
bool newPin3A = LOW;
bool newPin3B = LOW;
int Error3 = 0;
long Channel3Position  = 0;
int LEDCount3 = 0;
#endif

void setup()
{
#ifdef Channel1
  pinMode(2, INPUT);   // Channel 1 A
  pinMode(3, INPUT);   // Channel 1 B
  pinMode(4, OUTPUT);  // Channel 1 Up
  pinMode(5, OUTPUT);  // Channel 1 Down

  attachInterrupt(0, encoderupdate1A, CHANGE);
  attachInterrupt(1, encoderupdate1B, CHANGE);
#endif

#ifdef Channel2
  pinMode(6, INPUT);   // Channel 2 A
  pinMode(7, INPUT);   // Channel 2 B
  pinMode(8, OUTPUT);  // Channel 2 Up
  pinMode(9, OUTPUT);  // Channel 2 Down

  attachInterrupt(digitalPinToInterrupt(6), encoderupdate2A, CHANGE); // If available
  attachInterrupt(digitalPinToInterrupt(7), encoderupdate2B, CHANGE); // If available
#endif
  
#ifdef Channel3
  pinMode(10, INPUT);  // Channel 3 A
  pinMode(11, INPUT);  // Channel 3 B
  pinMode(12, OUTPUT); // Channel 3 Up
  pinMode(13, OUTPUT); // Channel 3 Down/LED
    
  attachInterrupt(digitalPinToInterrupt(10), encoderupdate3A, CHANGE); // If available
  attachInterrupt(digitalPinToInterrupt(11), encoderupdate3B, CHANGE); // If available
#endif
}

void loop() {
  if (LEDCount1 > 0)
    {
      LEDCount1--;
      if (LEDCount1 == 0)
        {
          digitalWrite(4,LOW);
          digitalWrite(5,LOW);
        }
    }
}

