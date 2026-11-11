/* Mini Laser Mode Analyzer Firmware using Atmega 328 Nano 3.0 and minimal external hardware. */

// V1.00 First version - SFPI only
// V1.01 Added longitudinal mode display
// V1.04 Added initial info screen
// V1.06 Added laser power meter
// V1.09 Added hold function
// V1.11 Added dual polarization
// V1.19 Non-dual polarization uses sum of P and S mode inputs for SFPI
// V1.20 Fix Mode display
// V1.21 Use dual Trace buffers for MODE
// V1.22 Add P and S annotation for POWER
// V1.24 Use dual Trace buffers for SFPI
// V1.27 Cleaned up
// V1.28 Test version
// V1.29 Removed display brightness test due to incompatibility with newer OLEDs.  Cause unknown

#define FirmwareVersion 129

#include <SPI.h>
#include <Wire.h>

//#include <d:\documents\uecide\libraries\spi\src\spi.h>
//#include <d:\documents\uecide\libraries\wire\src\wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Fonts/FreeSans9pt7b.h>

#define SCREEN_WIDTH 128 // OLED display width in pixels
#define SCREEN_HEIGHT 64 // OLED display height in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Inputs set up with pullup so will be high if unconnected
#define Free_Run 3           // D3 Ignore trigger input if high
#define Interpolate 4        // D4 Intpolate points if high
#define Sample 5             // D5 High when data is being acquired
#define Ramp 6               // D6 Ramp output
#define Average 7            // D7 Average if high
#define Dim_Display 8        // D8 Bright if high
#define Function_Select0 9   // D9 Function_Select10: HOLD = 00, POWER = 01, MODE = 10, SFPI = 11
#define Function_Select1 10  // D10
#define Dual_Polarization 12 // D12 Display both P and S modes for SFPI or MODE if high
#define Nano_LED 13          // D13 will be the same as the Sample LED except for errors

#define SFPI_PMode 0         // A0 SFPI P mode
#define SFPI_SMode 1         // A1 SFPI S mode
#define SFPI_Trigger 2       // A2 SFPI external trigger
#define Laser_Power 3        // A3 Laser power
#define MSweep_PMode 6       // A6 Mode sweep P mode
#define MSweep_SMode 7       // A7 Mode sweep S mode

#define Function0 1          // Function1,0: HOLD = 00, POWER = 01, MODE = 10, SFPI = 11.
#define Function1 2

#define HOLD 0
#define POWER 1 // Function0
#define MODE 2  // Function1
#define SFPI 3  // Function1 | Function0

#define Interpolate_Flag 4
#define Average_Flag 8
#define Dual_Polarization_Flag 16
 
#define PreSweep 8   // Number of steps before scan.  For 8: 15, 120, 3; for 16: 455, 7280, 8.
#define N 8          // Averaging factor - must be power of 2, maximum 64.
#define NShift 3     // Shift factor for averaging - log2(N)

uint8_t j = 0;
uint16_t Temp1 = 0;
uint16_t Temp2 = 0;
uint16_t Temp3 = 0;

char Function = 0;
char oldFunction = 0;
char realFunction = 0;
bool SPSelect = LOW;

char PTrace[128]; // Trace buffer for P mode (SFPI or MODE)
char STrace[128]; // Trace buffer for S mode (SFPI or MODE)

void setup()
{
  pinMode(Free_Run, INPUT_PULLUP);
  pinMode(Interpolate, INPUT_PULLUP);
  pinMode(Average, INPUT_PULLUP);
  pinMode(Dim_Display, INPUT_PULLUP);
  pinMode(Function_Select0, INPUT_PULLUP);
  pinMode(Function_Select1, INPUT_PULLUP);
  pinMode(Dual_Polarization, INPUT_PULLUP);

  pinMode(Sample, OUTPUT);
  pinMode(Ramp, OUTPUT);
  pinMode(Nano_LED, OUTPUT);
    
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally // Address 0x3D for 128x64
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;) // Don't proceed, loop forever
        {
          digitalWrite(Nano_LED,HIGH); // Flash Atmega LED to indicate failure
          delay(50);
          digitalWrite(Nano_LED,LOW);
          delay(1000);
        }
    }

    display.clearDisplay();
    display.display();
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(3, 0);
    display.print(F("  Documentation at "));
    display.setCursor(0, 8);
    display.print(F("RepairFAQ.org/sam/LMA"));
    display.setCursor(0, 56);
    display.print(F("    Version: "));   
    display.print(FirmwareVersion*.01);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(20, 32);
    display.print(F("Mini Laser"));
    display.setCursor(0, 48);
    display.print(F("Mode Analyzer"));
    display.display();
    delay(3000);
    display.fillRect(0, 0, 128, 64, BLACK);
    display.setFont(NULL);

    display.setCursor(0, 0);     // Start at top-left corner
    display.print(F("Mini LMA V"));
    display.print(FirmwareVersion*.01);
    display.display();
    digitalWrite(Nano_LED, LOW);
    digitalWrite(Sample, LOW);
    analogWrite(Ramp, 0);
 
    // Clear trace buffer
    for (j = 0; j < 128; j++) PTrace[j] = 0;

    analogRead(MSweep_PMode); // Extra read to eliminate glitch
    analogRead(SFPI_PMode); // Extra read to eliminate glitch

    // Set PWM frequency to ~63 kHz for pins 5 and 6 (also devides delay() times by 64)
    TCCR0B = (TCCR0B & 0b11111000) | 1;
}

void loop()
{    
  getFunction(); // Determine operating function
  
// Select display brightness - Removed due to newer OLEDs shutting down

 /* if (digitalRead(Dim_Display) == HIGH)
    {
      display.dim(HIGH); // Low
    }
  else
    {
      display.dim(LOW);  // High
    }
*/
  switch (Function & 3)
    {
      case HOLD: // Lock display, flash "*"
        display.setFont(NULL);
        display.setTextSize(1);
        display.fillRect(122, 0, 128, 15, BLACK);
        display.display();
        delay(1000);
        display.setCursor(122, 0);
        display.print(F("*"));
        display.display();
        delay(1000);            
      break;

      case POWER:  // Power meter display
        if((Function & 3) != (oldFunction & 3))
          {
            display.setFont(NULL); // Function changed to POWER
            display.setTextSize(1);
            display.fillRect(90, 0, 128, 15, BLACK);
            display.setCursor(90, 0);
            display.print(F("POWER"));   
            display.display();
            display.setFont(&FreeSans9pt7b);
            display.setTextSize(2);
          }
 
          display.fillRect(0, 16, 128, 64, BLACK);
          if ((Function & Average_Flag) != 0)
            {
              Temp1 = 0; // Average N samples
              Temp2 = 0;
              Temp3 = 0;
              for (j = 0; j < N; j++)
                {
                  Temp1 = Temp1 += analogRead(Laser_Power);
                  Temp2 = Temp2 += analogRead(MSweep_PMode);
                  Temp3 = Temp3 += analogRead(MSweep_SMode);
                  digitalWrite(Sample, HIGH);
                  digitalWrite(Nano_LED, HIGH);
                  delay(300);
                  digitalWrite(Sample, LOW);
                  digitalWrite(Nano_LED, LOW);
                  delay(500);
                }
              Temp1 = (((Temp1 >> NShift) * 42) / 43);
              Temp2 = (((Temp2 >> NShift) * 21) / 43);
              Temp3 = (((Temp3 >> NShift) * 21) / 43);
            }
          else
            {
              Temp1 = analogRead(Laser_Power); // Single sample
              Temp2 = analogRead(MSweep_PMode);
              Temp3 = analogRead(MSweep_SMode);
              digitalWrite(Sample, HIGH);
              digitalWrite(Nano_LED, HIGH);
              delay(500);
              digitalWrite(Sample, LOW);
              digitalWrite(Nano_LED, LOW);              
              delay(6000);
              Temp1 = (Temp1 * 42) / 43;
              Temp2 = (Temp2 * 21) / 43;
              Temp3 = (Temp3 * 21) / 43;
            }

          display.setCursor(35, 42);
 
          if ((Function & Dual_Polarization_Flag) != 0)
            {
              display.print((Temp2+Temp3)*.01);
              display.setTextSize(1);
              display.setCursor(15, 37);
              display.print(F("T:")); 
              display.fillRect(0, 52, 128, 63, BLACK);
              display.setFont(NULL);
              display.setTextSize(1); 
              display.setCursor(25, 54);
              display.print(F(":")); 
              display.setCursor(21, 54);
              display.print(F("P")); 
              display.setCursor(31, 54);
              display.print(Temp2*.01); 
              display.setCursor(75, 54);
              display.print(F(":")); 
              display.setCursor(71, 54);
              display.print(F("S")); 
              display.setCursor(81, 54);
              display.print(Temp3*.01); 
            }

          else
            {
              display.print(Temp1*.01);
            }
          display.fillRect(120, 52, 128, 63, BLACK);
          display.display();
                        
          display.setFont(&FreeSans9pt7b);
          display.setTextSize(2); 
  
          break;
 
       case MODE: // Longitudinal mode display
         // Clear the display area
         display.fillRect(0, 16, 128, 62, BLACK);      
         if(((Function - oldFunction) & 3) != 0)
           {
             display.setFont(NULL);
             display.setTextSize(1);
             display.fillRect(90, 0, 128, 15, BLACK);
             display.setCursor(90, 0);
             display.print(F("MODE "));    
             if ((realFunction & 3) != MODE)
               {
                 if ((Function & Dual_Polarization_Flag) != 0)
                   {
                     display.drawLine(0, 58, 128, 58, SSD1306_WHITE);
                     for (j = 0; j < 128; j++) STrace[j] = 0; // Clear Trace buffer
                   }
                 display.drawLine(0, 63, 128, 63, SSD1306_WHITE);
                 for (j = 0; j < 128; j++) PTrace[j] = 0; // Clear Trace buffer
               }
             else
               {
                 Temp1 = analogRead(MSweep_PMode); 
                 for (j = 1; j < 128; j++) // Scroll left one pixel and add one blank column
                   {
                     PTrace[j-1] = PTrace[j];
                     STrace[j-1] = STrace[j];
                   }
                 PTrace[127] = 63;
                 STrace[127] = 63;
               }
           }

         digitalWrite(Sample, HIGH);
         digitalWrite(Nano_LED, HIGH);

         for (j = 1; j < 128; j++) // Scroll left one pixel
           {
              PTrace[j-1] = PTrace[j];
              STrace[j-1] = STrace[j];
           }

         if ((Function & Average_Flag) != 0)
           {
             Temp1 = 0;
             Temp2 = 0;
             for (j = 0; j < N; j++)
               {
                 Temp1 += analogRead(MSweep_PMode); // Average if N samples
                 Temp2 += analogRead(MSweep_SMode); // Average if N samples
                 delay(3000 / N); // Spread samples somewhat evenly with similar update rate as without averaging
               }
             Temp1 = Temp1 >> NShift;
             Temp2 = Temp2 >> NShift;
           }
         else
           {
             Temp1 = analogRead(MSweep_PMode);
             Temp2 = analogRead(MSweep_SMode);
             delay(26000); // Just for LEDs :-)
           }

           digitalWrite(Sample, LOW);
           digitalWrite(Nano_LED, LOW);

        if ((Function & Dual_Polarization_Flag) != 0)
          {
            PTrace[127] = Temp1/24;
            STrace[127] = Temp2/24;
          }
        else
          {
            PTrace[127] = Temp1/22;
            STrace[127] = Temp2/22;
          }
        digitalWrite(Sample, LOW);
        digitalWrite(Nano_LED, LOW);
 
        Copy_Trace_to_OLED();
        Interpolate_Trace();
           
        // Update screen  
        display.display();
        break;

      case SFPI: // SFPI display

        display.fillRect(0, 16, 128, 64, BLACK); // Clear the display area

        if(((Function - oldFunction) & 3) != 0)
          {
            display.setFont(NULL); // Function changed to POWER
            display.setTextSize(1);
            display.fillRect(90, 0, 128, 15, BLACK);
            display.setCursor(90, 0);
            display.print(F("SFPI "));    

            for (j = 0; j < 128; j++) PTrace[j] = 0; // Clear PMode trace buffer
 
            display.drawLine(0, 63, 128, 63, SSD1306_WHITE);
            if ((Function & Dual_Polarization_Flag) != 0)
              {
                display.drawLine(0, 58, 128, 58, SSD1306_WHITE);
              }

            display.fillRect(0, 16, 128, 64, BLACK); // Clear the display area                        
            display.display();
          }

          // Rising edge trigger
          if (Free_Run == HIGH) // Ignore trigger input if high or floating
            {
              while (analogRead(SFPI_Trigger) > 512) {}
              while (analogRead(SFPI_Trigger) <= 512) {}
            }

          // Pre-sweep - get ramp rolling with 8 steps
          digitalWrite(Nano_LED, HIGH);

          // Start ramp to get past RC filter transient and keep timing the same, will be ignored
          for (j = 0; j < PreSweep; j++)
            { 
              analogWrite(Ramp, (((j * 15) >> 3) + 1));
              Temp1 = analogRead(SFPI_PMode);        // Extra analogReads to minimize analog mux switching glitch
              Temp1 = (analogRead(SFPI_PMode)/22);
              Temp2 = analogRead(SFPI_SMode);
              Temp2 = (analogRead(SFPI_SMode)/22);
            }
          
        digitalWrite(Sample, HIGH);

        if ((Function & Dual_Polarization_Flag) == 0)    
          {
            for (j = 0; j < 128; j++)
              {
                delay(1);                            // Diagnostic delay
                analogWrite(Ramp, ((((j * 15) + 120) >> 3) + 1 ));
                Temp1 = analogRead(SFPI_PMode);      // Extra analogReads to minimize analog mux switching glitch
                PTrace[j] = analogRead(SFPI_PMode)/22;
                Temp2 = analogRead(SFPI_SMode);
                STrace[j] = analogRead(SFPI_SMode)/22;
              }
          }
        else
          {
            for (j = 0; j < 128; j++)
              {
                delay(1);
                analogWrite(Ramp, ((((j * 15) + 120) >> 3) + 1 ));
                Temp1 = analogRead(SFPI_PMode);
                PTrace[j] = analogRead(SFPI_PMode)/24;
                Temp2 = analogRead(SFPI_SMode);
                STrace[j] = analogRead(SFPI_SMode)/24;
              }
          }

          analogWrite(Ramp, 1); // Avoid 0 glitch?
          digitalWrite(Sample, LOW);
          digitalWrite(Nano_LED, LOW);

          Copy_Trace_to_OLED();
          Interpolate_Trace();

          // Update screen
          display.display();  

          break;        
      }
}

// Get current function
void getFunction()
{
  oldFunction = Function;
  if (oldFunction != (HOLD & 3)) realFunction = Function;
  Function = 0;
  if (digitalRead(Function_Select0) == HIGH) Function = Function0;
  if (digitalRead(Function_Select1) == HIGH) Function |= Function1;
  if (digitalRead(Interpolate) == HIGH) Function |= Interpolate_Flag;
  if (digitalRead(Average) == HIGH) Function |= Average_Flag;
  if (digitalRead(Dual_Polarization) == HIGH) Function |= Dual_Polarization_Flag;
//  SPSelect = !SPSelect;
}

// Copy trace memory to OLED
void Copy_Trace_to_OLED()
{
  if ((Function & Dual_Polarization_Flag) != 0)
    {
      for (j = 0; j < 128; j++)
        { 
          display.drawPixel(j, 63-PTrace[j], SSD1306_WHITE);
          display.drawPixel(j, 58-STrace[j], SSD1306_WHITE);
        }
    }
  else
    {
      for (j = 0; j < 128; j++)
        {
          Temp1 = PTrace[j] + STrace[j];
          if (Temp1 < 48)
            {
              display.drawPixel(j, 63-Temp1, SSD1306_WHITE); // Draw pixel
            }
          else
            {
              display.drawPixel(j, 16, SSD1306_WHITE);  // Clip
            }
        }
    }
}
    
// Interpolate between vertical points if Interpolate pin is high and difference is > 1
void Interpolate_Trace()
{
  if ((Function & Interpolate_Flag) != 0)
    {
      for (j = 1; j < 128; j++)
        {
          if ((Function & Dual_Polarization_Flag) != 0) // Separate traces
            {
              if ((abs(PTrace[j-1] - PTrace[j]) > 1)) display.drawLine(j, (63-PTrace[j-1]), j, (63-PTrace[j]), SSD1306_WHITE);
              if ((abs(STrace[j-1] - STrace[j]) > 1)) display.drawLine(j, (58-STrace[j-1]), j, (58-STrace[j]), SSD1306_WHITE);
            }
          else // Single trace
            {
              Temp1 = PTrace[j-1] + STrace[j-1];
              if (Temp1 > 47) Temp1 = 47;
              Temp2 = PTrace[j] + STrace[j];
              if (Temp2 > 47) Temp2 = 47;
              if (abs(Temp1 - Temp2) > 1) display.drawLine(j, (63-Temp1), j, (63-Temp2), SSD1306_WHITE);
            }
        }
    } 
}
