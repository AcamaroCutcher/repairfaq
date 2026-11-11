// Optimized single channel quadrature to pulse converter front-end intended to be used with uMD1
// v01 - Basic implementation
// v06 - Direct PORT calls

// Signals: A=D2, B=D3, Up=D4, Down=D5.

#define FirmwareVersion 0106

#define Pin1A 4
#define Pin1B 8
#define Up 16
#define Down 32
#define Error 32

int oldState1 = 0;
int newState1 = 0;
int count = 0;

void setup()
{
  pinMode(2, INPUT);   // Channel 1 A
  pinMode(3, INPUT);   // Channel 1 B
  pinMode(4, OUTPUT);  // Channel 1 Up Pulse
  pinMode(5, OUTPUT);  // Channel 1 Down Pulse
  pinMode(7, OUTPUT);  // Dummy output
  pinMode(13, OUTPUT);  // Error LED

  PORTB = 0;   // Reset Error LED.

  newState1 = PIND & (Pin1A | Pin1B);
}

void loop ()
{
  for(count=1;count>0;count--) digitalWrite(7, HIGH); // Delay to guarantee minimum output pulse width.
  oldState1 = newState1;
  newState1 = PIND & (Pin1A | Pin1B);

  PORTD = 0;   // Reset Up/Down outputs.
  
  if (oldState1 == 0)
    {
      if (newState1 == 0) {}        // No change.
      else if (newState1 == Pin1A)
        {
            PORTD = Up;
        }
      else if (newState1 == Pin1B)
        {
            PORTD = Down;
        }
     else  PORTB = Error;   // Set Error LED.
    }
  else if (oldState1 == Pin1A)
    {
      if (newState1 == Pin1A) {}        // No change.
      else if (newState1 == 0)
        {
            PORTD = Down;
        }
      else if (newState1 == (Pin1A | Pin1B))
        {
            PORTD = Up;
        }
      else  PORTB = Error;   // Set Error LED.
    }
  else if (oldState1 == Pin1B)
    {
      if (newState1 == Pin1B) {}        // No change.
      else if (newState1 == 0)
        {
            PORTD = Up;
        }
      else if (newState1 == (Pin1A | Pin1B))
        {
            PORTD = Down;
        }
      else  PORTB = Error;   // Set Error LED.
    }
  else if (oldState1 == (Pin1A | Pin1B))
    {
      if (newState1 == (Pin1A | Pin1B)) {}        // No change.
      else if (newState1 == Pin1B)
        {
           PORTD = Up;
        }
      else if (newState1 == Pin1A)
        {
           PORTD = Down;
        }
      else PORTB = Error;   // Set Error LED.
    }
}


