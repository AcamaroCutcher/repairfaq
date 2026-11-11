// Optimized single channel quadrature to pulse converter front-end intended to be used with uMD1

// v01 - Basic implementation
// v06 - Direct PORT calls
// v07 - Differential output for Up/Down

// Signals: A=D2, B=D3, Up_P = D4, Up_N = D6, Down_P = D5, Down_N = D7.

#define FirmwareVersion 0106

#define Pin1A 4
#define Pin1B 8
#define Up_P 16
#define Down_P 32
#define Up_N 64
#define Down_N 128
#define Error 32

int oldState1 = 0;
int newState1 = 0;
int count = 0;

void setup()
{
  pinMode(2, INPUT);   // Channel 1 A
  pinMode(3, INPUT);   // Channel 1 B
  pinMode(4, OUTPUT);  // Channel 1 Up_P Pulse
  pinMode(5, OUTPUT);  // Channel 1 Down_P Pulse
  pinMode(6, OUTPUT);  // Channel 1 Up_N Pulse
  pinMode(7, OUTPUT);  // Channel 1 Down_N Pulse

  pinMode(13, OUTPUT);  // Error LED

  PORTB = 0;   // Reset Error LED.

  newState1 = PIND & (Pin1A | Pin1B);
}

void loop ()
{
  for(count=10;count>0;count--) digitalWrite(7, HIGH); // Delay to stretch LED on-time for demo.

  oldState1 = newState1;
  newState1 = PIND & (Pin1A | Pin1B);

  PORTD = Up_N | Down_N;   // Reset Up/Down outputs.
  
  if (oldState1 == 0)
    {
      if (newState1 == 0) {}        // No change.
      else if (newState1 == Pin1A)
        {
            PORTD = Up_P | Down_N;
        }
      else if (newState1 == Pin1B)
        {
            PORTD = Down_P | Up_N;
        }
     else  PORTB = Error;   // Set Error LED.
    }
  else if (oldState1 == Pin1A)
    {
      if (newState1 == Pin1A) {}        // No change.
      else if (newState1 == 0)
        {
            PORTD = Down_P | Up_N;
        }
      else if (newState1 == (Pin1A | Pin1B))
        {
            PORTD = Up_P | Down_N;
        }
      else  PORTB = Error;   // Set Error LED.
    }
  else if (oldState1 == Pin1B)
    {
      if (newState1 == Pin1B) {}        // No change.
      else if (newState1 == 0)
        {
            PORTD = Up_P | Down_N;
        }
      else if (newState1 == (Pin1A | Pin1B))
        {
            PORTD = Down_P | Up_N;
        }
      else  PORTB = Error;   // Set Error LED.
    }
  else if (oldState1 == (Pin1A | Pin1B))
    {
      if (newState1 == (Pin1A | Pin1B)) {}        // No change.
      else if (newState1 == Pin1B)
        {
           PORTD = Up_P | Down_N;
        }
      else if (newState1 == Pin1A)
        {
           PORTD = Down_P | Up_N;
        }
      else PORTB = Error;   // Set Error LED.
    }
}


