//********************************************************************************************************//
//         Micro Measurement Display 2 - uMD2 - Heterodyne interferometer firmware for Teensy 4.0         //
//                 Copyright® Jan Beck and Sam Goldwasser, 1994-2025, all rights reserved                 //
//********************************************************************************************************//

// V2.61.01 New branch.  First re-attempt at heuristic deglitching 18-Jan-2025.  Changed to numeric firmware
//          sub-versions instead of letters to display in "COM %" field of GUI without modifications. 
// V2.61.02 Now we start hacking the interpolation for real. ;-)
// V2.61.03 Built state transition table for interpolation.  Still buggy.
// V2.61.04 Reducing glitches.
// V2.61.05 Very close to being clean.  Added diagnostic serial monitor options.
// V2.61.06 Cleaned a couple more bogus state errors  At least one more to go.
// V2.61.07 Cleaned up state transition 25.
// V2.61.08 Interpolation appears to be 99.99999% glitch-free but still some noise at kludge points, probably
//           nothing can be done to remedy that.  This version works with and without interpolation.
// V2.61.09 Aborted - crashes.
// V2.61.10 First test of three axis interpolation, at least to compile and not crash or mess up axis 1.
// V2.61.11 First to have QuadTimer_Setup.cpp .
// V2.61.12 Skip.
// V2.61.13 Skip superstition. ;-)
// V2.61.14 First to have Next_State.cpp.  Seems OK, usually. ;-( ;-)
// V2.61.15 Stopped working. ;-(
// V2.61.16 Skipped accidentally.
// V2.61.17 Cleaned up a bit.
// V2.61.18 Cleaned up a bit more.
// V2.61.19 Reverted to single sketch file.  (No .cpp files.)  Still may be an unitialized variable.  Added LED Axis blink code. ;-)
// V2.61.20 Test of single axis interpolation using floating point.
// V2.61.21 Update Axes 2 and 3 interpolation using floating point.  NOT TESTED.
// V2.61.22 Moved Phase(1,2,3) calculation to Phase_Execute().
// V2.61.23 Minor cleanup.
// V261.24  Added 16 bit Phase default and new firmware version format XXX.YY for use with 29-Jan-2025 uMD GUI or later.
// V261.25  Cleanup
// V261.26  Aborted
// V261.27  Upgraded to aborted V261.26.  Cleaned up 8 and 16 bit Phase calculations.
// V261.28  Converted uMD GUI to default to 16 bit Phase to minimize possibility of large offset at startup.
// V261.30  Increased number of samples to 5000.  This should result in better than 0.02 nm resolution, 1/6th the diameter of a hydrogen atom. ;-)
// V261.31  Cleaned up.  PRC% and COM% implemented in Diagnostic Readouts along with REF and MEAS counts/cycle (x1 or x2 depending on Multiplier).
// V261.32  Started adding tests for displacement incompatible with phase interpolation. Code 1.
// V261.33  Continued adding tests for displacement incompatible with phase interpolation including status in unused Hom3A and Hom3B LEDs. Code 2.
// V261.34  Broke ;-(
// V261.35  Broke ;-(
// V261.36  Working on re-entering End-Poimt Regime.  Seems mostly where edges coincide, surprise, surprise.
// V261.37  Still issues re-entering EPR.  May not be worth a lot of effort.  It's a purity thing. ;-)
// V261.38  Ditto, released version.
// V261.40  Fixed a couple of simple typos that Codex found (!!), complete Codex-edited version is V261.41.

// Version with three axis interpolation capability.  Only Axis 1 has been fully tested.  Interpolation
// provides sub-nm-scale resolution and precision (accuracy subject to enbironmental compensation) for slew rates of
// around 0.01 mm/second or less and normal 79 or 158 nm resolution (depending on interferometer optics) above that.

#define Heterodyne 1            // Set to the number of axes for heterodyne, sample rate always 1 kHz.

#define Interpolation 1         // O for none, 1 for enabled.
#define Old_uMD_GUI 0           // Set to 0 for 16 bit Interpolation with 2025+ GUI, 1 for 8 bit with all GUIs.
#define OLED 1                  // OLED display of banner and version if defined
#define OLED_Displacement 1     // OLED display of SN, Disp1, Disp2, Disp3 if defined
#define OLED_REF 2              // REF frequency display if defined; Value is text size.  1 (default) and 2 (large) valid.

#define PCB_Version 100         // Select PCB version as 100 for V1.00 (really old) or 122 or 123 for V1.22 or V1.23.  Only affects Interpolation status LEDs.

// No user or hacker sericeable parts beyond this point. ;-)

#define FirmwareVersion 261     // Firmware version used by OLED and GUI About.
#define FirmwareSubVersion 40   // Now numeric in XXX.YY format.
#define FirmwareVersion_Flag 4  // Set to 4 to tell 2025+ GUI to use new firmware Version/Subversion format for About.

#define Multiplier 2            // Integer for REF/MEAS1/MEAS2/MEAS3 counts/cycle.  1 for rising edge; 2 for double clocking.  Used for TMR_CTRL_CM value.  Do NOT mess with that!

#define Phase_8bit_Flag 2       // Tells 2025+ GUI to use 8 bit Phase values if Old_uMD_GUI set to 1, 16 bit Phase is the default.  There is no good reason to use 8 bit with the 2025+ GUI.
#define REF_Sync 0              // Doesn't appear to be required for interpolation or anything else. ;-)  Leave at 0.

#define Diagnostics 1           // 0 Normal uMD2.
                                // 1 Diagnostics inside uMD GUI but display may be strange with some data in axes 2 and 3 fields. ;-)
                                // 2 Extended printout using Serial Monitor or terminal program but incompatible with uMD GUI.

#define REF_Pin 0      // REF input
#define Hom1A_Pin 1    // Hom 1A wired to REF
#define Hom1B_Pin 2    // Hom 1B wired to MEAS1
#define Hom2A_Pin 3    // Hom 2A wired to MEAS2
#define Hom2B_Pin 4    // Hom 2B wired to MEAS3
#define Hom3A_Pin 5    // Hom 3A / Interpolation_Error_LED V1.2X PCB; Interpolation_OK_LED V1.00 PCB
#define REFI_Pin 6     // REF interpolation input
#define Hom3B_Pin 7    // Hom 3B / Interpolation_OK_LED V1.2X PCB; Interpolation_Error_LED V1.00 PCB
#define MEAS1I_Pin 8   // MEAS1 interpolation input
#define MEAS1_Pin 9    // MEAS1 input
#define MEAS2_Pin 10   // MEAS2 input
#define MEAS2I_Pin 11  // MEAS2 interpolation input
#define MEAS3I_Pin 12  // MEAS3 interpolation input
#define MEAS3_Pin 14   // MEAS3 Input

// Interpolation Axis 1 computation status LEDs
#if PCB_Version == 100
#define Interpolation_OK_LED 5    // Phase1 State NOT Error green
#define Interpolation_Error_LED 7 // Displacement1 change > 1 red
#else
#define Interpolation_OK_LED 7    // Phase1 State NOT Error green
#define Interpolation_Error_LED 5 // Displacement1 change > 1 red
#endif

#define On 0           // LED On for input pins ONLY
#define Off 1          // LED Off for input pins ONLY

#define IOK_LED_Timeout 25 // Amount of time in loop cycles to stay ON
#define IER_LED_Timeout 25 // Amount of time in loop cycles to stay ON

int IOK_LED_Timer = 0;     // These keep track of how long their respective LEDs have been on
int IER_LED_Timer = 0;

#ifdef OLED
#include <U8x8lib.h>       // Graphics library available though the Library Manager
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE);

#endif

float FirmwareFloat = FirmwareVersion;
uint OLEDSubVersion = FirmwareSubVersion;

char buffer[100];
int32_t Scale_Shift   = 0;

// DON'T mess with these!
IMXRT_TMR_t * TMR1 = (IMXRT_TMR_t *)&IMXRT_TMR1;
IMXRT_TMR_t * TMR2 = (IMXRT_TMR_t *)&IMXRT_TMR2;
IMXRT_TMR_t * TMR3 = (IMXRT_TMR_t *)&IMXRT_TMR3;
IMXRT_TMR_t * TMR4 = (IMXRT_TMR_t *)&IMXRT_TMR4;
#define IMXRT_GPIO7_DIRECT  (*(volatile uint32_t *)0x42004008)

IntervalTimer usbTimer;            // send USB data at predefinded rate to make frequency analysis work in the GUI

#ifdef OLED
char oled_buffer[16];
char old_oled_buffer[16];
char REF_oled_buffer[16];
char old_REF_oled_buffer[16];
char sn_oled_buffer[16];
char old_sn_oled_buffer[16];
char DISP1_oled_buffer[16];
char old_DISP1_oled_buffer[16];
char DISP2_oled_buffer[16];
char old_DISP2_oled_buffer[16];
char DISP3_oled_buffer[16];
char old_DISP3_oled_buffer[16];
      
uint8_t tiles[8] = {0x0, 0x80, 0x7c, 0x40, 0x40, 0x40, 0x7c, 0x0}; // Tail on "micro"
#endif

int64_t counter_REF   =  0;
int64_t counter_MEAS1 =  0;
int64_t counter_MEAS2 =  0;
int64_t counter_MEAS3 =  0;

int64_t counter_REF_save   =  0;
int64_t counter_MEAS1_save =  0;
int64_t counter_MEAS2_save =  0;
int64_t counter_MEAS3_save =  0;

int32_t OLED_MEAS1 =  0;
int32_t OLED_MEAS2 =  0;
int32_t OLED_MEAS3 =  0;

int32_t REF_Frequency_Total = 0;
int32_t REF_Frequency_Total_Count = 0;
  
float   REF_Frequency = 0;
float   REF_Frequency_Old = 0;
float   REF_Frequency_New = 0;
float   REF_Frequency_Average = 0;

float   OLED_REF_Frequency = 0;
float   OLED_REF_Frequency_save = 0;

int32_t REF =  0;
int32_t MEAS1 =  0;
int32_t MEAS2 =  0;
int32_t MEAS3 =  0;

int32_t OLED_REF_save = 0;
int32_t OLED_MEAS1_save = 0;
int32_t OLED_MEAS2_save = 0;
int32_t OLED_MEAS3_save = 0;

int32_t OLED_DISP1 =  0;
int32_t OLED_DISP2 =  0;
int32_t OLED_DISP3 =  0;

int32_t OLED_DISP1_save = 0;
int32_t OLED_DISP2_save = 0;
int32_t OLED_DISP3_save = 0;

int32_t displacement1 = 0;
int32_t displacement2 = 0;
int32_t displacement3 = 0;

int32_t previous_displacement1 = 0;
int32_t previous_displacement2 = 0;
int32_t previous_displacement3 = 0;

int32_t Displacement_Sent = 0;
int32_t Displacement1_Sent = 0;
int32_t Displacement2_Sent = 0;
int32_t Displacement3_Sent = 0;

int32_t velocity1     = 0;
int32_t velocity2     = 0;
int32_t velocity3     = 0;

uint64_t sequenceNumber = 0;
uint32_t sn             = 0;
uint32_t old_sn = 0;

int32_t LowSpeedCode = 0;
int32_t LowSpeedData = 0;
int32_t LowSpeedCodeSelect = 0;

uint32_t j = 0;

uint32_t data = 0;

uint32_t REF_Save = 0;
uint32_t MEAS1_Save = 0;

uint32_t REF_Waveform[5500];
uint32_t MEAS1_Waveform[5500];
uint32_t MEAS2_Waveform[5500];
uint32_t MEAS3_Waveform[5500];

uint32_t gpioData[5500];

uint32_t MEAS_Valid = 0;
uint32_t MEAS1_Valid = 0;
uint32_t MEAS2_Valid = 0;
uint32_t MEAS3_Valid = 0;

int32_t REF_Offset = 0;
int32_t MEAS1_Offset = 0;
int32_t MEAS2_Offset = 0;
int32_t MEAS3_Offset = 0;

// Variables for Phase1 calculation

double REF_Period_Total_float = 0;
double REF_Edge_Count_Adj_float = 0;
double REF_Period_Average_float = 0;

double MEAS_Period_Total_float = 0;
double MEAS_Edge_Count_Adj_float = 0;
double MEAS_Period_Average_float = 0;
double Phase_Average_float = 0;
double Phase_Total_float = 0;

int32_t Phase_Average = 0;
int32_t Phase_Total = 0;

int32_t Phase_Offset = 0;

int32_t Phase = 0;
int32_t Phase1 = 0;
int32_t Phase2 = 0;
int32_t Phase3 = 0;

int32_t Phase_8bit = 0;
int32_t Phase1_8bit = 0;
int32_t Phase2_8bit = 0;
int32_t Phase3_8bit = 0;

int32_t Phase1_Sent = 0;
int32_t Phase2_Sent = 0;
int32_t Phase3_Sent = 0;

int32_t Previous_Phase = 0;
int32_t Previous_Phase1 = 0;
int32_t Previous_Phase2 = 0;
int32_t Previous_Phase3 = 0;

uint32_t REF_Edge_Count = 0;
uint32_t MEAS1_Edge_Count = 0;
uint32_t MEAS2_Edge_Count = 0;
uint32_t MEAS3_Edge_Count = 0;

uint32_t REF_Edge_Count_Adj = 0;
uint32_t MEAS_Edge_Count_Adj = 0;

uint32_t REF_Edge_Positions[2500];
uint32_t MEAS1_Edge_Positions[2500];
uint32_t MEAS2_Edge_Positions[2500];
uint32_t MEAS3_Edge_Positions[2500];

uint32_t REF_Period_Average = 0;
uint32_t MEAS_Period_Average = 0;
uint32_t MEAS1_Period_Average = 0;

uint32_t REF_Period_Total = 0;
uint32_t MEAS_Period_Total = 0;

uint32_t REF_NoChange = 0;
uint32_t Waveform_Length = 5000;

volatile uint32_t No_REF = 0x12345; // Will be changed by any REF activity in the assembly block.  This is probably silly but it doesn't seem to hurt. ;-)

int Processing_Start = 0;
int Processing_Time = 0;
int Communications_Start = 0;
int Communications_Time = 0;

int Exit = 0;
int Test_Flags = 0;

int Phase1_Skip_Timer = 0;
int Phase2_Skip_Timer = 0;
int Phase3_Skip_Timer = 0;

bool Phase1_Enable = false; // True if conditions are such that the Phase computation will be valie
bool Phase2_Enable = false;
bool Phase3_Enable = false;

/* DISP=Displacement Count, Phase=Interpolated Value
   NC=No Change, DEC=DECrement, INC=INCrement
   SC=Small Change of less than +/-63, Drop=Decline by more than 127, Jump=Increase by more than 127 */

#define Normal_State_0  0  // DISP NC, Phase SC, DISP DEC, Phase Drop, DISP INC, Phase Jump.

#define Bogus_State_1  1   // DISP NC.  Phase Jump.
#define Bogus_State_2  2   // DISP NC.  Phase Drop. 
#define Bogus_State_3  3   // DISP DEC. Phase SC. 
#define Bogus_State_4  4   // DISP INC. Phase SC. 
#define Bogus_State_5  5   // DISP DEC. Phase Jump.  Error.
#define Bogus_State_6  6   // DISP INC. Phase Drop.  Error.
#define Bogus_State_7  7   // Reserved

#define Error_State_15 15  // Transitions that are generally not possible even with jittery REF and MEAS edges.

uint32_t Phase_State  = Normal_State_0;
uint32_t Phase1_State = Normal_State_0;
uint32_t Phase2_State = Normal_State_0;
uint32_t Phase3_State = Normal_State_0;

void Waveform_Capture_Assembly_Block() __attribute__((optimize("-O0")));

void setup() {

  pinMode(REF_Pin, INPUT_PULLUP);    // 0 REF input
  pinMode(Hom1A_Pin, INPUT_PULLUP);  // 1 Hom 1A wired to REF
  pinMode(Hom1B_Pin, INPUT_PULLUP);  // 2 Hom 1B wired to MEAS1
  pinMode(MEAS1_Pin, INPUT_PULLUP);  // 3 MEAS1 input
  pinMode(MEAS2_Pin, INPUT_PULLUP);  // MEAS2 input
  pinMode(MEAS3_Pin, INPUT_PULLUP);  // MEAS3 input
  pinMode(REFI_Pin, INPUT);          // REF   interpolation input
  pinMode(MEAS1I_Pin, INPUT);        // MEAS1 interpolation input
  pinMode(MEAS2I_Pin, INPUT);        // MEAS2 interpolation input
  pinMode(MEAS3I_Pin, INPUT);        // MEAS3 interpolation input

  // Turn off unused LEDs for Homodyne channel 3 IFF no line receivers present.

  pinMode(Hom3A_Pin, OUTPUT);        // Hom 3A
  digitalWrite (Hom3A_Pin, Off);    // Turn off LED for Hom 3A
  pinMode(Hom3B_Pin, OUTPUT);        // Hom 3B
  digitalWrite (Hom3B_Pin, Off);    // Turn off LED for Hom 3B

  // Turn off unused LEDs for MEAS2 and MEAS3 (and Homodyne channel 2) if no line receivers present.
  if (Heterodyne == 1) {
    pinMode(Hom2A_Pin, OUTPUT);      // Hom 2A wired to MEAS2
    digitalWrite (Hom2A_Pin, Off);  // Turn off LED for Hom 2A
    pinMode(Hom2B_Pin, OUTPUT);      // Hom 2B wired to MEAS3
    digitalWrite (Hom2B_Pin, Off);  // Turn off LED for Hom 2B
  }

  pinMode(LED_BUILTIN, OUTPUT);  // Status LED

  Scale_Shift = Multiplier >> 1;

  Serial.begin(2000000);

#ifdef OLED
  // Initialize and clear display
  u8x8.begin();
  u8x8.setPowerSave(0);

  // Banner and sequence number display
  u8x8.setFont(u8x8_font_chroma48medium8_r);  // Default font (thin)
  u8x8.drawString(0, 0, "   MD2 V      ");
  u8x8.drawTile(2, 0, 1, tiles);              // Needed for tail of micro symbol
  sprintf(buffer, "%3d", FirmwareVersion);
  u8x8.drawString(8, 0, buffer);
  u8x8.drawString(11, 0, ".");
  sprintf(buffer, "%02d", OLEDSubVersion );
  u8x8.drawString(12, 0, buffer);

  for (j = 0; j < 16; j++) {
    oled_buffer[j] = 0;
    old_oled_buffer[j] = 0;
    REF_oled_buffer[j] = 0;
    old_REF_oled_buffer[j] = 0;      
    DISP1_oled_buffer[j] = 0;
    old_DISP1_oled_buffer[j] = 0;
    DISP2_oled_buffer[j] = 0;
    old_DISP2_oled_buffer[j] = 0;
    DISP3_oled_buffer[j] = 0;
    old_DISP3_oled_buffer[j] = 0;
    sn_oled_buffer[j] = 0;
    old_sn_oled_buffer[j] = 0;
  }

 #ifdef OLED_REF
  // Initialize REF frequency line

  if (OLED_REF == 1) {
    u8x8.drawString(0, 3, "REF: 0.0000 MHz");     // Default font
  }

  if (OLED_REF == 2) {
    u8x8.setFont(u8x8_font_8x13_1x2_f);         // Medium font
    u8x8.drawString(0, 6, "   REF (MHz)   ");
    u8x8.setFont(u8x8_font_chroma48medium8_r);  // Default font
  }
 #endif
#endif

#ifdef OLED_Displacement

  u8x8.drawString(0, 2, "Seq#:        ");

 #if OLED_REF != 2

  u8x8.drawString(0, 4, "Disp1:");

  if (Heterodyne > 1) {
    u8x8.drawString(0, 5, "Disp2:");
    u8x8.drawString(0, 6, "Disp3:");
  }

 #endif
#endif

  QuadTimer_Setup ();                    // Initialize counters for REF, MEAS1, MEAS2, MEAS3

  usbTimer.begin(USBSender, 1000);       // Send USB data every 1000 microseconds
  usbTimer.priority(200);                // Lower numbers are higher priority, with 0 the highest and 255 the lowest. Most other interrupts default to 128

  // Clear junk from waveform buffers.
  for (j = 0; j < Waveform_Length; j++) {
    REF_Waveform[j] = 0;
    MEAS1_Waveform[j] = 0;
    MEAS2_Waveform[j] = 0;
    MEAS3_Waveform[j] = 0;
  }

  // Reset counters again.  Only should need to do LSBs.
  TMR2->CH[0].CNTR = 0;   // set REF count to 0
  TMR4->CH[0].CNTR = 0;   // set MEAS1 count to 0
  TMR1->CH[0].CNTR = 0;   // set MEAS2 count to 0
  TMR3->CH[0].CNTR = 0;   // set MEAS3 count to 0
}

void USBSender()
{
  Processing_Start = micros(); // Time stamp for start of processing

#if Interpolation > 0
 #if REF_Sync
  // Sync on REF counter input being low in preparation for assembly sync on rising edge.  This may be redundant.
  for (j = 0; j < 100; j++) {  // Loop until REF clock is high
    if ((IMXRT_GPIO7_DIRECT & 0x400) != 0) {
      j = 100;
    }
  } 

  for (j = 0; j < 100; j++) {  // Loop until REF clock goes low and capture REF counter value
    if ((IMXRT_GPIO7_DIRECT & 0x400) == 0) {
      j = 100;
    }
  }

  REF_Save = TMR2->CH[0].CNTR; // This will be REF before incrementing
 #endif

  Waveform_Capture_Assembly_Block ();  // Capture GPIO waveforms if interpolation enabled
#endif

  // Load counter values

  counter_REF   =  TMR2->CH[3].HOLD;
  counter_REF   =  counter_REF * 65536  + TMR2->CH[2].HOLD;
  counter_REF   =  counter_REF * 65536  + TMR2->CH[1].HOLD;
  counter_REF   =  counter_REF * 65536  + TMR2->CH[0].HOLD;

  counter_MEAS1 =  TMR4->CH[3].HOLD;
  counter_MEAS1 =  counter_MEAS1 * 65536  + TMR4->CH[2].HOLD;
  counter_MEAS1 =  counter_MEAS1 * 65536  + TMR4->CH[1].HOLD;
  counter_MEAS1 =  counter_MEAS1 * 65536  + TMR4->CH[0].HOLD;

  counter_MEAS2 =  TMR1->CH[3].HOLD;
  counter_MEAS2 =  counter_MEAS2 * 65536  + TMR1->CH[2].HOLD;
  counter_MEAS2 =  counter_MEAS2 * 65536  + TMR1->CH[1].HOLD;
  counter_MEAS2 =  counter_MEAS2 * 65536  + TMR1->CH[0].HOLD;

  counter_MEAS3 =  TMR3->CH[3].HOLD;
  counter_MEAS3 =  counter_MEAS3 * 65536  + TMR3->CH[2].HOLD;
  counter_MEAS3 =  counter_MEAS3 * 65536  + TMR3->CH[1].HOLD;
  counter_MEAS3 =  counter_MEAS3 * 65536  + TMR3->CH[0].HOLD;

  REF   = (counter_REF   - counter_REF_save)   >> Scale_Shift; // Reduction of GUI frequency counter resolution if Multiplier set to 2 (Scale_Shift = 1).
  MEAS1 = (counter_MEAS1 - counter_MEAS1_save) >> Scale_Shift; // This does not affect displacement.
  MEAS2 = (counter_MEAS2 - counter_MEAS2_save) >> Scale_Shift;
  MEAS3 = (counter_MEAS3 - counter_MEAS3_save) >> Scale_Shift;

  // Update values

  counter_REF_save = counter_REF;
  counter_MEAS1_save = counter_MEAS1;
  counter_MEAS2_save = counter_MEAS2;
  counter_MEAS3_save = counter_MEAS3;

  previous_displacement1 = displacement1;
  previous_displacement2 = displacement2;
  previous_displacement3 = displacement3;

  displacement1 = (counter_MEAS1 - counter_REF);
  displacement2 = (counter_MEAS2 - counter_REF);
  displacement3 = (counter_MEAS3 - counter_REF);

  velocity1 = displacement1 - previous_displacement1;
  velocity2 = displacement2 - previous_displacement2;
  velocity3 = displacement3 - previous_displacement3;

#if Interpolation == 0
  Displacement1_Sent = displacement1;
  Displacement2_Sent = displacement2;
  Displacement3_Sent = displacement3;
#endif
 
#if Interpolation == 1
Interpolation_Execute();
#endif

  sequenceNumber++;
  sn = sequenceNumber;

// Numder of axes LED blink code
       if                      ((sn & 0x1F0) == 0x040)  digitalWrite (LED_BUILTIN, 1); // 1 Blink =  1 Axis
  else if ((Heterodyne > 1) && ((sn & 0x1F0) == 0x0C0)) digitalWrite (LED_BUILTIN, 1); // 2 Blinks = 2 Axes
  else if ((Heterodyne > 2) && ((sn & 0x1F0) == 0x140)) digitalWrite (LED_BUILTIN, 1); // 3 Blinks = 3 Axes
  else                                                  digitalWrite (LED_BUILTIN, 0); // Black space

// Status LED keep on timers

  if (IER_LED_Timer > 1) IER_LED_Timer --;  // Keed red Interpolation Error LED on for a while
  else if (IER_LED_Timer >= 0) { digitalWrite(Interpolation_Error_LED, Off); IER_LED_Timer = 0; }
  if (IOK_LED_Timer > 1) IOK_LED_Timer --;  // Keep green Interpolation OK LED on for a while
  else if (IOK_LED_Timer >= 0) { digitalWrite(Interpolation_OK_LED, Off); IOK_LED_Timer = 0; }

// Hazard-free update of OLED REF frequency
#ifdef OLED_REF
  noInterrupts();
  REF_Frequency_Total += REF;
  REF_Frequency_Total_Count++;
  interrupts();
#endif

  // Set up appropriate low speed data
  LowSpeedCode = 0;                        // Default to no low speed data
  LowSpeedData = 0;
  LowSpeedCodeSelect = sequenceNumber & 0x1f;

  if (LowSpeedCodeSelect == 1) {           // Send firmware version
    LowSpeedCode = 10;
    LowSpeedData = FirmwareVersion;
#if Old_uMD_GUI == 0
    LowSpeedData += (FirmwareSubVersion << 16);
#endif
  }

  else if (LowSpeedCodeSelect == 2) {      // Sammple frequency x 100
    LowSpeedCode = 8;
    LowSpeedData = 100000;
  }

  else if (LowSpeedCodeSelect == 3) {      // Tell GUI this is not a homodyne system
    LowSpeedCode = 20;
    LowSpeedData = (Multiplier * 256);     // Tell GUI this is not a homodyne system; Multiplier must be present or GUI NaN.
  }

  else if (LowSpeedCodeSelect == 4) {      // Percent time spent in capture and analysis.
    LowSpeedCode = 121;
    LowSpeedData = Processing_Time * 65.53938;
  }

  else if (LowSpeedCodeSelect == 5) {      // Percent time spent in communications)
    LowSpeedCode = 122;
    LowSpeedData = Communications_Time * 65.53938;
  }

  else if (LowSpeedCodeSelect == 6) {      // # CPU clocks spent in communications
    LowSpeedCode = 15;
    LowSpeedData = gpioData[0];            // GPIO
  }

  // Low speed codes 101, 102, 111, 112 available for diagnostics
  else if (LowSpeedCodeSelect == 7) {      // REF period average and REF to MEAS1 average values
    LowSpeedCode = 101;
    LowSpeedData = (REF_Period_Average & 0xff);
  }

  else if (LowSpeedCodeSelect == 8) {      // REF period average and REF to MEAS1 average values
    LowSpeedCode = 111;
    LowSpeedData = (MEAS_Period_Average & 0xff);
  }

  else if (LowSpeedCodeSelect == 10) {     // Firmware_Flags for 2025 and beyond uMD GUI, ignored by previous versions
    LowSpeedCode = 112;
#if (Old_uMD_GUI == 1)
    LowSpeedData = Phase_8bit_Flag;        // Tells the 2025+ GUI to use 8 bit Phase.
#endif
    LowSpeedData |= FirmwareVersion_Flag;  // Tells the GUI to use decimal Firmware Subversion
  }

#if Old_uMD_GUI == 1                       // Use 8 bit Phase if it is selected or using old uMD GUI
      Phase1_Sent = Phase1_8bit;
      Phase2_Sent = Phase2_8bit;
      Phase3_Sent = Phase3_8bit;
#else
      Phase1_Sent = Phase1;
      Phase2_Sent = Phase2;
      Phase3_Sent = Phase3;
#endif

Processing_Time = micros() - Processing_Start; // Time spent in acquiring data

Communications_Start = micros(); // Tiome stamp for start of communications

#if Diagnostics == 0  // Normal uMD GUI
  if (Serial.availableForWrite() > 256) {
    Serial.printf("%li ", REF);                      // 1: REF
    Serial.printf("%li ", MEAS1);                    // 2: MEAS1
    Serial.printf("%li ", Displacement1_Sent);       // 3: Displacement 1
    Serial.printf("%li ", velocity1);                // 4: Velocity Count 1
    Serial.printf("%li ", Phase1_Sent);              // 5: Phase 1
    Serial.printf("%llu ", sequenceNumber);          // 6: Sequence Number
    Serial.printf("%li ", LowSpeedCode);             // 7: LowSpeedCode
    Serial.printf("%li", LowSpeedData);              // 8: LowSpeedData

    if (Heterodyne > 1) {
      Serial.printf(" %li ", MEAS2);                 // 9: MEAS2
      Serial.printf("%li ", Displacement2_Sent);     // 10: Displacement 2
      Serial.printf("%li ", velocity2);              // 11: Velocity Count 2
      Serial.printf("%li ", Phase2_Sent);            // 12: Phase 2
      Serial.printf("%li ", MEAS3);                  // 13: MEAS3
      Serial.printf("%li ", Displacement3_Sent);     // 14: Displacement 3
      Serial.printf("%li ", velocity3);              // 15: Velocity Count 3
      Serial.printf("%li", Phase3_Sent);             // 16: Phase 3
    }
    Serial.println();
  }
#endif

#if Diagnostics == 1  // Interpolation diagnostic values within uMD GUI
  if (Serial.availableForWrite() > 256) {
    Serial.printf("%li ", REF);                      // 1: REF
    Serial.printf("%li ", MEAS1);                    // 2: MEAS1
    Serial.printf("%li ", Displacement1_Sent);       // 3: Displacement Sent 1
    Serial.printf("%li ", displacement1);            // 4: Raw displacement    // Velocity Count 1
    Serial.printf("%li ", Phase1_Sent);              // 5: Phase 1
    Serial.printf("%llu ", sequenceNumber);          // 6: Sequence Number
    Serial.printf("%li ", LowSpeedCode);             // 7: LowSpeedCode
    Serial.printf("%li", LowSpeedData);              // 8: LowSpeedData

int zero = 0;
    if (Heterodyne > 0) {
      Serial.printf(" %li " , Exit);                 // State exit identifier  //  9 MEAS2
      Serial.printf("%li ", Phase1_State );          // State flags            // 10 Velocity2
      Serial.printf("%li ", Phase2_Sent);            // Phase2);               // 11: Phase 2
      Serial.printf("%li ", zero);                   // velocity2);            // 12: Velocity Count 2
      Serial.printf("%li ", zero);                   // MEAS3);                // 13: MEAS3
      Serial.printf("%li ", zero);                   // displacement3);        // 14: Displacement 3
      Serial.printf("%li ", zero);                   // velocity3);            // 15: Velocity Count 3
      Serial.printf("%li", zero);                    // Phase3);               // 16: Phase 3
    }
    Serial.println();
  }
#endif

#if Diagnostics == 2 // Interpolation diagnostic values NOT compatible with uMD GUI, only serial monitor
  if (Serial.availableForWrite() > 256) {
    Serial.printf("SN: ");
    Serial.printf("%7llu ", sequenceNumber);         // Sequence Number
    Serial.printf(" DISP Raw: " );
    Serial.printf("%2li ", displacement1);           // displacement 1
    Serial.printf(" Sent: ");
    Serial.printf("%2li ", Displacement1_Sent);      // Displacement 1 Sent
    Serial.printf(" Phase: ");
    Serial.printf("%4li ", Phase1_Sent);             // Phase 1
    Serial.printf(" Exit: ");
    Serial.printf("%3li " , Exit);                   // State exit identifier   Serial.printf(" State: ");  
    Serial.printf("Goto: %2li ", Phase1_State );     // State flags
    Serial.printf(" Flags: D");
    if ((Test_Flags & 1) != 0)  Serial.printf(" ");  // DISP and Phase change flags
    if ((Test_Flags & 2) != 0)  Serial.printf("-");
    if ((Test_Flags & 4) != 0)  Serial.printf("+");
    Serial.printf(" P");
    if ((Test_Flags & 8) != 0)  Serial.printf(" ");
    if ((Test_Flags & 16) != 0) Serial.printf("-");
    if ((Test_Flags & 32) != 0) Serial.printf("+");
    Serial.printf(" DS");
    Serial.printf(" %4li ", Displacement1_Sent);
    Serial.printf(" P");
    Serial.printf(" %4li", Phase1);

    if (Heterodyne > 1) {
      Serial.printf(" %li ", MEAS2);                 // 9: MEAS2
      Serial.printf("%li ", displacement2);          // 10: Displacement 2
      Serial.printf("%li ", velocity2);              // 11: Velocity Count 2
      Serial.printf("%li ", Phase2_Sent);            // 5: Phase 2
      Serial.printf("%li ", MEAS3);                  // 13: MEAS3
      Serial.printf("%li ", displacement3);          // 14: Displacement 3
      Serial.printf("%li ", velocity3);              // 15: Velocity Count 3
      Serial.printf("%li", Phase3_Sent);             // 16: Phase 3
    }
    Serial.println();
  }
#endif

Communications_Time = micros() - Communications_Start; // Time spent in communications
}

void loop() { // Only for OLED and Status LEDs
  OLED_DISPLAY();
  delay(100);
}

#if Interpolation == 1
void Interpolation_Execute() {
   
  // Test for REF activity
  if ((No_REF != 0x12345) && (REF >= 100)) { // REF OK and at least 100 counts/ms)

    // Extract bits corresponding to REF, MEAS1, MEAS2, MEAS3 to separate waveform arrays for convenience and possibly speed.
    REF_Offset = 4; // 1-6 appear to work; 4 is default. 
    MEAS1_Offset = 0; // Only 0 appears to work.
    MEAS2_Offset = 0;
    MEAS3_Offset = 0;

    for (j = 0; j < Waveform_Length; j++) {
      data = gpioData[j]; // copy 32 bit value
      REF_Waveform[j + REF_Offset] = (data >> 10) & 0x1;     // REF is IO register bit D6
      MEAS1_Waveform[j + MEAS1_Offset] = (data >> 16) & 0x1; // MEAS1 is IO register bit D8;
      MEAS2_Waveform[j + MEAS2_Offset] = (data >> 2) & 0x1;  // MEAS2 is IO register bit D11;
      MEAS3_Waveform[j + MEAS3_Offset] = (data >> 1) & 0x1;  // MEAS3 is IO register bit D12;
    }

    // Locate and store REF and MEAS edge positions for this sample
    REF_Edge_Count = 0;
    MEAS1_Edge_Count = 0;
    MEAS2_Edge_Count = 0;
    MEAS3_Edge_Count = 0;

 #if Multiplier == 1 // Look for rising edges and save positions
    for (j = 1; j < Waveform_Length; j++) {
      if ((REF_Waveform[j] - REF_Waveform[j - 1]) == 1) {      // Look for rising edges on REF waveform
        REF_Edge_Positions[REF_Edge_Count] = j;                // Store position of found REF edge
        REF_Edge_Count++;                                      // Increment REF edge count
      }

      if ((MEAS1_Waveform[j] - MEAS1_Waveform[j - 1]) == 1) {  // Look for rising edges on MEAS1 waveform
        MEAS1_Edge_Positions[MEAS1_Edge_Count] = j;            // Store position of found MEAS1 edge
        MEAS1_Edge_Count++;                                    // Increment MEAS1 edge count
      }

  #if Heterodyne > 1
      if ((MEAS2_Waveform[j] - MEAS2_Waveform[j - 1]) == 1) {  // Look for rising edges on MEAS2 waveform
        MEAS2_Edge_Positions[MEAS2_Edge_Count] = j;            // Store position of found MEAS2 edge
        MEAS2_Edge_Count++;                                    // Increment MEAS2 edge count
      }
  #endif

  #if Heterodyne > 2
      if ((MEAS3_Waveform[j] - MEAS3_Waveform[j - 1]) == 1) {  // Look for rising edges on MEAS3 waveform
        MEAS3_Edge_Positions[MEAS3_Edge_Count] = j;            // Store position of found MEAS3 edge
        MEAS3_Edge_Count++;                                    // Increment MEAS3 edge count
      }
  #endif
  }
 #endif // Multiplier == 1

 #if Multiplier == 2 // Look for rising and falling edges and store positions
    for (j = 1; j < Waveform_Length; j++) {
      if (((REF_Waveform[j] - REF_Waveform[j - 1]) == 1) ||  ((REF_Waveform[j-1] - REF_Waveform[j]) == 1)) {          // Look for rising or falling edges on REF waveform
        REF_Edge_Positions[REF_Edge_Count] = j;                // Store position of found REF edge
        REF_Edge_Count++;                                      // Increment REF edge count
      }

      if (((MEAS1_Waveform[j] - MEAS1_Waveform[j - 1]) == 1) ||  ((MEAS1_Waveform[j-1] - MEAS1_Waveform[j]) == 1)) {  // Look for rising or falling edges on MEAS1 waveform
        MEAS1_Edge_Positions[MEAS1_Edge_Count] = j;            // Store position of found MEAS1 edge
        MEAS1_Edge_Count++;                                    // Increment MEAS1 edge count
      }

  #if Heterodyne > 1
      if (((MEAS2_Waveform[j] - MEAS2_Waveform[j - 1]) == 1) ||  ((MEAS2_Waveform[j-1] - MEAS2_Waveform[j]) == 1)) {  // Look for rising or falling edges on MEAS1 waveform
        MEAS2_Edge_Positions[MEAS2_Edge_Count] = j;            // Store position of found MEAS1 edge
        MEAS2_Edge_Count++;                                    // Increment MEAS1 edge count
      }
  #endif

  #if Heterodyne > 2  
      if (((MEAS3_Waveform[j] - MEAS3_Waveform[j - 1]) == 1) ||  ((MEAS3_Waveform[j-1] - MEAS3_Waveform[j]) == 1)) { 