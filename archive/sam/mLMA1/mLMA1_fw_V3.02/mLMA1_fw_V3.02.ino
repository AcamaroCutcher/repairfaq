/*************************************************************************************************************/
/*         Mini Laser Mode Analyzer Firmware using Atmega 328 Nano 3.0 and minimal external hardware         */
/*     Copyright (c) 1994-2024 by Samuel M. Goldwasser / Sam's Laser FAQ at http://www.repairfaq.org/LMA     */
/* This code may be used for non-commerical purposes as long as the above notice is included in its entirety */
/*************************************************************************************************************/

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
// V2.00 First version using 240x240 color TFT SD7789
// V2.02 HOLD and POWER mostly done
// V2.03 Only update POWER total digits if changed :(
// V2.06 Dual polarization MODE works more or less without average or interpolate
// V2.07 Cleaned up
// V2.08 Dual polarization SFPI works more or less
// V2.09 SFPI with interpolation, maybe :)
// V2.10 Speeded up slightly
// V2.11 Added overlap color for interpoated SFPI
// V2.12 Cleaned up
// V2.14 Moved interpolation stuff to subroutines
// V2.15 Added color schemes
// V2.16 Added non dual polarization SFPI
// V2.17 Semi working interpolated DP MODE, slow FastVL
// V2.18 Semi working interpolated DP MODE with total power, proper erase
// V2.20 Added bottom annotation
// V2.24 Added SCOPE - oscilloscope mode display, combined some functions into subroutines
// V2.26 All functions have bottom annotation, interpolation added to MODE-S
// V2.28 Cleaned up, added beginning of COMBO function
// V2.46 Back to mostly working all modes after weird erratic crashing -- Believed to be due to
//        bugs in the Arduino C compiler, switched to UECIDE
// V2.48 Added kernal generator for SFPI antialiasing filter
// V2.49 Switched to optimized graphics libraries.  Now about 8 scans/sec for SFPI, 6.4 for COMBO.
// V2.50 Cleanup
// V2.51 Started adding hooks for antializaing filter for SFPI
// V2.52 Cleanup, put trigger for SFPI back in
// V2.53 Bug fixes, made trigger option for all functions
// V2.54 Bug fixes, simplified trigger to just Free_Run pin
// V2.55 Bug fixes in CHART
// V2.57 Added download function for trace buffers (CHART and SFPI)
// V2.70 First version with antialiasing filter.
// V2.72 Rudimentary three button interface (TBI)
// V2.77 Added persistance option for SFPI and SFPI/COMBO
// V2.78 Added EEPROM load, untested
// V2.79 TBI state transistions mostly work, only main functions implemented so far
// V2.80 Cleaned up TBI somewhat, still no parameter adjustments
// V2.81 TBI now includes all functions, control of on/off parameters (but not adjustments), and EEPROM save/recall/defaults
// V2.82 Added Rate adjustments
// V2.82a Added POWER CAL adjustment and better chnage button response
// V2.82b All adjustments except SPAN, OFFSET, and KERNAL are functional
// V2.82c All adjustments are functional except KERNAL does nothing yet
// V2.82d Rearranged subroutines etc. to be more logical and match documentation
// V2.82f Cleaned up
// V2.82g Added momentary display of current value when entering paremeter adjust
// V2.82h Merge of V3.xx with frequency counter display.  Basic operationg tested with V3 but not frequency counter yet
// V2.82i Renamed temporary variables - hopefully didn't break much
// V2.82j Frequency counter operational using interrupts
// V2.83k Simpler frequency counter using only delays
// V2.82m Replaced delay() with millis() in attempt to eliminate TIMER1 interrupts no go, micros() doesn't work at all
// V2.82n Cleaned up, adjusted delays, fixed erroneous function reset
// V3.00  First official version with V3 and V2
// V3.01  Bug fixes
// V3.02  Changed defult #define FC

#define FirmwareVersion 3.02
#define EEPROMFormatValid 37 // Arbitraty magic number to indicate data in EEPROM is valid - change to force new defaults
// #define DARK // Enable DARK color theme
#define Eight_Color // Enable 8 color theme for brain dead LCDs. :)

#define FC // FC->version with frequency counter; comment out for no frequency counter or PCB V2.00.

// Color themes - Change the appropriate ifdefs below for the one you want.

// Custom colors     RRRRRGGGGGGBBBBB
#define RUBY       0b1110000010001100
#define EMERALD    0b0100011100010000
#define TOPAZ      0b1000011011111111
#define ORANGE     0b1111101111101111
#define DARKRUBY   0b0111000001000110
#define BROWN      0b1010001100000000
#define LIGHTBROWN 0b1101100110111000
#define DARKWHITE  0b1100011000011000
#define GRAY       0b1010010100010100
#define MIDGRAY    0b0110001100001100
#define DARKGRAY   0b0010000100000100
#define DARKRED    0b1101000000000000
#define DARKGREEN  0b0000011000001000
#define LIGHTGREEN 0b0110011110010011
#define DARKBLUE   0b0000000000011000
#define DARKYELLOW 0b1100011000000000
#define DARKCYAN   0b0000011110011011
#define PINK       0b1111110010111001

#ifdef DARK // Dark background like real DSO. :)
#define BG_Color BLACK
#define FG_Color WHITE
#define LMA_Documentation_Color YELLOW
#define LMA_Banner_Color BLUE
#define LMA_Version_Color MAGENTA
#define BUTTON1_Color GRAY
#define BUTTON2_Color GREEN
#define BUTTON3_Color GRAY
#define OFF_Color DARKRED
#define ON_Color DARKGREEN
#define TEXT_Color DARKYELLOW
#define STATUS_Color GREEN
#define LINE_Color GRAY
#define MARKER_Color MIDGRAY
#define POWER_Color BLUE
#define POWER_Text_Color BLUE
#define POWER_Test_Overflow_Color ORANGE
#define MODE_Color MAGENTA
#define CHART_Color PINK
#define SCOPE_Color TOPAZ
#define SFPI_Color YELLOW
#define COMBO_Color LIGHTGREEN
#define PMode_Color RED
#define SMode_Color GREEN
#define SPMode_Color YELLOW
#define SPMode_Overlap_Color YELLOW
#define MODE_Overlap_Color YELLOW
#define MODE_Total_Color BLUE
#define SFPI_Overlap_Color YELLOW
#define MODE_Total_Overflow_Color ORANGE
#define RATE_Color RED
#define CAL_Color RUBY
#define EXIT_Color RED
#define SETTINGS_Color DARKGREEN
#define UTILITIES_Color LIGHTBROWN
#define DUMP_Color BROWN
#define EXECUTE_Color ORANGE
#define UBanner_Color WHITE
#define LBanner_Color BLUE
#define LMA_Color BLUE
#define PLUS_Color GREEN
#define MINUS_Color RED
#define FREQUENCY_Color TOPAZ
#define FREQUENCY_WARNING_Color ORANGE
#define FREQUENCY_OVERFLOW_Color RED
#endif

#ifdef Eight_Color // For brain-dead LCDs stuck in 8-color mode. :)
#define BG_Color BLACK
#define FG_Color WHITE
#define LMA_Documentation_Color YELLOW
#define LMA_Banner_Color BLUE
#define LMA_Version_Color MAGENTA
#define BUTTON1_Color WHITE
#define BUTTON2_Color GREEN
#define BUTTON3_Color WHITE
#define OFF_Color RED
#define ON_Color GREEN
#define TEXT_Color YELLOW
#define STATUS_Color GREEN
#define LINE_Color WHITE
#define MARKER_Color WHITE
#define POWER_Color BLUE
#define POWER_Text_Color BLUE
#define POWER_Test_Overflow_Color MAGENTA
#define MODE_Color MAGENTA
#define CHART_Color MAGENTA
#define SCOPE_Color CYAN
#define SFPI_Color YELLOW
#define COMBO_Color GREEN
#define PMode_Color RED
#define SMode_Color GREEN
#define SPMode_Color YELLOW
#define SPMode_Overlap_Color YELLOW
#define MODE_Overlap_Color YELLOW
#define MODE_Total_Color BLUE
#define SFPI_Overlap_Color YELLOW
#define MODE_Total_Overflow_Color MAGENTA
#define RATE_Color RED
#define CAL_Color RED
#define EXIT_Color RED
#define SETTINGS_Color GREEN
#define UTILITIES_Color MAGENTA
#define DUMP_Color MAGENTA
#define EXECUTE_Color CYAN
#define UBanner_Color WHITE
#define LBanner_Color BLUE
#define LMA_Color BLUE
#define PLUS_Color GREEN
#define MINUS_Color RED
#define FREQUENCY_Color CYAN
#define FREQUENCY_WARNING_Color MAGENTA
#define FREQUENCY_OVERFLOW_Color RED
#endif

//#define COMPATIBILITY_MODE // May improve performance if not on Atmega 328
//#define CS_ALWAYS_LOW

#include <Arduino.h>
#include <PDQ_GFX.h>    // Core graphics library by Adafruit
#include <Arduino_ST7789_Fast.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <EEPROM.h>

// SPI pins for LCD
#define TFT_DC    9   // D9: DC for LCD
#define TFT_MOSI  11  // D11: SDA for LCD
#define TFT_SCLK  13  // D13 (also Nano LED): SCL for LCD
#define TFT_RST   19  // D19: RES for LCD

// Input pins set up with pullup so will be high if unconnected (D8, D9, D11, D13 used for SPI)

#ifndef FC
 // Version 2.xx pin definitions
 #define Function_Select0 2   // D2 (SW1,BT1): Function_Select0 - HOLD = 000, POWER = 001, MODE Chart = 010,
 #define Function_Select1 3   // D3 (SW2,BT2): Function_Select1    MODE Scope = 011 SFPI = 100, MIX = 101
 #define Function_Select2 4   // D4 (SW3,BT3): Function_Select2
 #define Ramp_N 5             // D5 Ramp inverted
 #define Ramp_P 6             // D6 Ramp positive
 #define Dual_Polarization 7  // D7 (SW4): Display both P and S modes if high
 #define Free_Run 8           // D8 (SW5): Stop samplnig if low, trigger on rising edge
 #define Interpolate 10       // D10 (SW6): Interpolate points if high
 #define Filter 12            // D12 (SW7): N-point average for POWER, CHART, SCOPE, and COMBO/SCOPE or antialiasing filter for SFPI and COMBO/SFPI if high
 #define DSpare 16            // D16 or A2 Spare
 #define Persistance 17       // D17 (SW8): SFPI persistance
 #define Sample_P 18          // D18: Sample pulse out
#endif

#ifdef FC
 // Version 3.xx pin definitions
 #define Function_Select0 2   // D2 (SW1,BT1): Function_Select0 - HOLD = 000, POWER = 001, MODE Chart = 010,
 #define Function_Select1 4   // D4 (SW2,BT2): Function_Select1    MODE Scope = 011 SFPI = 100, MIX = 101
 #define Function_Select2 7   // D7 (SW3,BT3): Function_Select2
 #define Ramp_N 3             // D3: Ramp inverted
 #define Ramp_P 6             // D6: Ramp positive
 #define Dual_Polarization 8  // D8 (SW4): Display both P and S modes if high
 #define Free_Run 16          // D16 (SW5): Stop sampling if low, trigger on rising edge
 #define Interpolate 10       // D10 (SW6): Interpolate points if high
 #define Filter 12            // D12 (SW7): N-point average for POWER, CHART, SCOPE, and COMBO/SCOPE or antialiasing filter for SFPI and COMBO/SFPI if high
 #define Persistance 17       // D17: SFPI persistance
 #define Sample_P 18          // D18: Sample pulse out

 #define Frequency_Counter_Input 5 // D5: External TTL REF frequency input
#endif

// Analog inputs
#define SFPI_PMode 0   // A0 SFPI P mode
#define SFPI_SMode 1   // A1 SFPI S mode
#define MSweep_PMode 6 // A6 Mode sweep P mode
#define MSweep_SMode 7 // A7 Mode sweep S mode

#define Function0 1          // Function210: HOLD = 000, POWER = 001, CHART = 010, SCOPE = 011 SFPI = 100, COMBO = 110
#define Function1 2
#define Function2 4

#define DLHLD 0 // Download trace buffers to host and lock display
#define POWER 1 // Power meter
#define CHART 2 // Chart recorder formatted mode sweep
#define SCOPE 3 // Scope formatted mode sweep
#define SFPI  4 // SFPI
#define COMBO 5 // Combination of SFPI and SCOPE
#define HOLD1 6 // Lock display
#define HOLD2 7 // Lock display
#define SETTINGS 6  //
#define UTILITIES 7 //

#define Interpolate_Flag 8
#define Filter_Flag 16
#define Dual_Polarization_Flag 32
#define Persistance_Flag 64
#define Free_Run_Flag 128

#define PreSweep 16  // Number of steps before scan.
#define N 8          // Averaging factor - must be power of 2, maximum 64.
#define NShift 3     // Shift factor for averaging - log2(N)

#define Old 242  // SCOPE data locations must not conflixt with SFPI or COMBO/SFPI PreSweep
#define New 243

#define topSeparator 20      // Top separator
#define middleSeparator 119  // Middle separator
#define bottomSeparator 218  // Bottom separator

#define fullUpper 24
#define fullBase 216
#define fullLimit 191
#define fullSize 192

#define halfUpper 120
#define halfBase 117
#define halfLimit 95
#define halfSize 97

#define Kernal_Span 0
#define FilterKernalSize 32

#define Start SBTemp1 // For Overlap
#define End SBTemp2

// ACB, UCB locations
#define UCB_VALID 0
#define STARTUP_FUNCTION 1
#define STARTUP_STATE 2
#define POWER_CAL 3
#define POWER_RATE 4
#define CHART_RATE 5
#define SCOPE_RATE 6
#define SFPI_SPAN 7
#define SFPI_OFFSET 8
#define SFPI_KERNAL 9
#define SFPI_RATE 10
#define COMBO_RATE 11
#define MODE_GAIN 12
#define SFPI_GAIN 13
#define ACB_FLAGS 14
#define DUAL_POL_ENABLE 1
#define INTERP_ENABLE 2
#define FILTER_ENABLE 4
#define FREE_RUN_ENABLE 8
#define PERSISTANCE_ENABLE 16
#define ACB_CHECKSUM 15

// Three-button interface state and action definitions.  Please refer to www.repairfaq.org/sam/mLMA1/ for state diagrams.
//         State                        Value
//-----------------------------------------------------------------------------
#define   RESERVED                      0                    //

#define   POWER_SEL                     1
#define   POWER_RUN                     2
#define   POWER_CAL_SEL                 3
#define   POWER_RATE_SEL                4
#define   POWER_EXIT_SEL                5
#define   POWER_CAL_RUN                 6
#define   POWER_RATE_RUN                7

#define   CHART_SEL                     8
#define   CHART_RUN                     9
#define   CHART_RATE_SEL                10
#define   CHART_EXIT_SEL                11
#define   CHART_RATE_RUN                12

#define   SCOPE_SEL                     13
#define   SCOPE_RUN                     14
#define   SCOPE_RATE_SEL                15
#define   SCOPE_EXIT_SEL                16
#define   SCOPE_RATE_RUN                17

#define   SFPI_SEL                      18
#define   SFPI_RUN                      19
#define   SFPI_PERSIST_SEL              20
#define   SFPI_SPAN_SEL                 21
#define   SFPI_OFFSET_SEL               22
#define   SFPI_KERNAL_SEL               23
#define   SFPI_RATE_SEL                 24
#define   SFPI_EXIT_SEL                 25
#define   SFPI_SPAN_RUN                 26
#define   SFPI_OFFSET_RUN               27
#define   SFPI_KERNAL_RUN               28
#define   SFPI_RATE_RUN                 29

#define   COMBO_SEL                     30
#define   COMBO_RUN                     31
#define   COMBO_RATE_SEL                32
#define   COMBO_EXIT_SEL                33
#define   COMBO_RATE_RUN                34

#define   SETTINGS_SEL                  35
#define   SETTINGS_RUN                  36
#define   SETTINGS_MODE_GAIN_SEL        37
#define   SETTINGS_SFPI_GAIN_SEL        38
#define   SETTINGS_DUAL_POL_SEL         39
#define   SETTINGS_INTERP_SEL           40
#define   SETTINGS_FILTER_SEL           41
#define   SETTINGS_FREE_RUN_SEL         42
#define   SETTINGS_EXIT_SEL             43
#define   SETTINGS_MODE_GAIN_RUN        44
#define   SETTINGS_SFPI_GAIN_RUN        45

#define   UTILITIES_SEL                 46
#define   UTILITIES_RUN                 47
#define   UTILITIES_LOCK_DISPLAY_SEL    48
#define   UTILITIES_DUMP_DATA_SEL       49
#define   UTILITIES_LOAD_PARAM_SEL      50
#define   UTILITIES_SAVE_PARAM_SEL      51
#define   UTILITIES_DEFAULTS_SEL        52
#define   UTILITIES_EXIT_SEL            53
#define   UTILITIES_LOAD_PARAM_CONF     54
#define   UTILITIES_SAVE_PARAM_CONF     55
#define   UTILITIES_DEFAULTS_CONF       56

#define   POWER_CAL_DEC                 100
#define   POWER_CAL_INC                 101

#define   POWER_RATE_DEC                102
#define   POWER_RATE_INC                103

#define   CHART_RATE_DEC                104
#define   CHART_RATE_INC                105

#define   SCOPE_RATE_DEC                106
#define   SCOPE_RATE_INC                107

#define   SFPI_PERSIST_TOG              108

#define   SFPI_SPAN_DEC                 109
#define   SFPI_SPAN_INC                 110

#define   SFPI_OFFSET_DEC               111
#define   SFPI_OFFSET_INC               112

#define   SFPI_KERNAL_DEC               113
#define   SFPI_KERNAL_INC               114

#define   SFPI_RATE_DEC                 115
#define   SFPI_RATE_INC                 116

#define   COMBO_RATE_DEC                117
#define   COMBO_RATE_INC                118

#define   SETTINGS_DUAL_POL_TOG         119
#define   SETTINGS_INTERP_TOG           120
#define   SETTINGS_FILTER_TOG           121
#define   SETTINGS_FREE_RUN_TOG         122

#define   SETTINGS_MODE_GAIN_DEC        123
#define   SETTINGS_MODE_GAIN_INC        124

#define   SETTINGS_SFPI_GAIN_DEC        125
#define   SETTINGS_SFPI_GAIN_INC        126

#define   UTILITIES_LOCK_DISPLAY_TOG    127
#define   UTILITIES_DUMP_DATA_EXE       128

#define   UTILITIES_LOAD_PARAM_EXE      129
#define   UTILITIES_SAVE_PARAM_EXE      130
#define   UTILITIES_DEFAULTS_EXE        131

//const byte hello[10]
const uint8_t State_Memory[180] PROGMEM = {

//const PROGMEM uint8_t State_Memory[] = {
// Three button interface state transition and action table
// State or action ->                Button1                       Button2                      Button3
//--------------------------------------------------------------------------------------------------------------------
  /*  0 RESERVED */                    0,                            0,                           0,

  /*  1 POWER_SEL,*/                   POWER_SEL,                    POWER_RUN,                   CHART_SEL,
  /*  2 POWER_RUN */                   POWER_RUN,                    POWER_SEL,                   POWER_CAL_SEL,
  /*  3 POWER_CAL_SEL */               POWER_RUN,                    POWER_CAL_RUN,               POWER_RATE_SEL,
  /*  4 POWER_RATE_SEL */              POWER_CAL_SEL,                POWER_RATE_RUN,              POWER_EXIT_SEL,
  /*  5 POWER_EXIT_SEL */              POWER_RATE_SEL,               POWER_SEL,                   POWER_EXIT_SEL,
  /*  6 POWER_CAL_RUN */               POWER_CAL_DEC,                POWER_CAL_SEL,               POWER_CAL_INC,
  /*  7 POWER_RATE_RUN */              POWER_RATE_DEC,               POWER_RATE_SEL,              POWER_RATE_INC,

  /*  8 CHART_SEL */                   POWER_SEL,                    CHART_RUN,                   SCOPE_SEL,
  /*  9 CHART_RUN */                   CHART_RUN,                    CHART_SEL,                   CHART_RATE_SEL,
  /* 10 CHART_RATE_SEL */              CHART_RUN,                    CHART_RATE_RUN,              CHART_EXIT_SEL,
  /* 11 CHART_EXIT_SEL */              CHART_RATE_SEL,               CHART_SEL,                   CHART_EXIT_SEL,
  /* 12 CHART_RATE_RUN */              CHART_RATE_DEC,               CHART_RATE_SEL,              CHART_RATE_INC,

  /* 13 SCOPE_SEL */                   CHART_SEL,                    SCOPE_RUN,                   SFPI_SEL,
  /* 14 SCOPE_RUN */                   SCOPE_RUN,                    SCOPE_SEL,                   SCOPE_RATE_SEL,
  /* 15 SCOPE_RATE_SEL */              SCOPE_RUN,                    SCOPE_RATE_RUN,              SCOPE_EXIT_SEL,
  /* 16 SCOPE_EXIT_SEL */              SCOPE_RATE_SEL,               SCOPE_SEL,                   SCOPE_EXIT_SEL,
  /* 17 SCOPE_RATE_RUN */              SCOPE_RATE_DEC,               SCOPE_RATE_SEL,              SCOPE_RATE_INC,

  /* 18 SFPI_SEL */                    SCOPE_SEL,                    SFPI_RUN,                    COMBO_SEL,
  /* 19 SFPI_RUN */                    SFPI_RUN,                     SFPI_SEL,                    SFPI_PERSIST_SEL,
  /* 20 SFPI_PERSIST_SEL */            SFPI_RUN,                     SFPI_PERSIST_TOG,            SFPI_SPAN_SEL,
  /* 21 SFPI_SPAN_SEL */               SFPI_PERSIST_SEL,             SFPI_SPAN_RUN,               SFPI_OFFSET_SEL,
  /* 22 SFPI_OFFSET_SEL */             SFPI_SPAN_SEL,                SFPI_OFFSET_RUN,             SFPI_KERNAL_SEL,
  /* 23 SFPI_KERNAL_SEL */             SFPI_OFFSET_SEL,              SFPI_KERNAL_RUN,             SFPI_RATE_SEL,
  /* 24 SFPI_RATE_SEL */               SFPI_KERNAL_SEL,              SFPI_RATE_RUN,               SFPI_EXIT_SEL,
  /* 25 SFPI_EXIT_SEL */               SFPI_RATE_SEL,                SFPI_SEL,                    SFPI_EXIT_SEL,
  /* 26 SFPI_SPAN_RUN */               SFPI_SPAN_DEC,                SFPI_SPAN_SEL,               SFPI_SPAN_INC,
  /* 27 SFPI_OFFSET_RUN */             SFPI_OFFSET_DEC,              SFPI_OFFSET_SEL,             SFPI_OFFSET_INC,
  /* 28 SFPI_KERNAL_RUN */             SFPI_KERNAL_DEC,              SFPI_KERNAL_SEL,             SFPI_KERNAL_INC,
  /* 29 SFPI_RATE_RUN */               SFPI_RATE_DEC,                SFPI_RATE_SEL,               SFPI_RATE_INC,

  /* 30 COMBO_SEL */                   SFPI_SEL,                     COMBO_RUN,                   SETTINGS_SEL,
  /* 31 COMBO_RUN */                   COMBO_RUN,                    COMBO_SEL,                   COMBO_RATE_SEL,
  /* 32 COMBO_RATE_SEL */              COMBO_RUN,                    COMBO_RATE_RUN,              COMBO_EXIT_SEL,
  /* 33 COMBO_EXIT_SEL */              COMBO_RATE_SEL,               COMBO_SEL,                   COMBO_EXIT_SEL,
  /* 34 COMBO_RATE_RUN */              COMBO_RATE_DEC,               COMBO_RATE_SEL,              COMBO_RATE_INC,

  /* 35 SETTINGS_SEL */                COMBO_SEL,                    SETTINGS_RUN,                UTILITIES_SEL,
  /* 36 SETTINGS_RUN */                SETTINGS_RUN,                 SETTINGS_SEL,                SETTINGS_MODE_GAIN_SEL,
  /* 37 SETTINGS_MODE_GAIN_SEL */      SETTINGS_RUN,                 SETTINGS_MODE_GAIN_RUN,      SETTINGS_SFPI_GAIN_SEL,
  /* 38 SETTINGS_SFPI_GAIN_SEL */      SETTINGS_MODE_GAIN_SEL,       SETTINGS_SFPI_GAIN_RUN,      SETTINGS_DUAL_POL_SEL,
  /* 39 SETTINGS_DUAL_POL_SEL */       SETTINGS_SFPI_GAIN_SEL,       SETTINGS_DUAL_POL_TOG,       SETTINGS_INTERP_SEL,
  /* 40 SETTINGS_INTERP_SEL */         SETTINGS_DUAL_POL_SEL,        SETTINGS_INTERP_TOG,         SETTINGS_FILTER_SEL,
  /* 41 SETTINGS_FILTER_SEL */         SETTINGS_INTERP_SEL,          SETTINGS_FILTER_TOG,         SETTINGS_FREE_RUN_SEL,
  /* 42 SETTINGS_FREE_RUN_SEL */       SETTINGS_FILTER_SEL,          SETTINGS_FREE_RUN_TOG,       SETTINGS_EXIT_SEL,
  /* 43 SETTINGS_EXIT_SEL */           SETTINGS_FREE_RUN_SEL,        SETTINGS_SEL,                SETTINGS_EXIT_SEL,
  /* 44 SETTINGS_MODE_GAIN_RUN */      SETTINGS_MODE_GAIN_DEC,       SETTINGS_MODE_GAIN_SEL,      SETTINGS_MODE_GAIN_INC,
  /* 45 SETTINGS_SFPI_GAIN_RUN */      SETTINGS_SFPI_GAIN_DEC,       SETTINGS_SFPI_GAIN_SEL,      SETTINGS_SFPI_GAIN_INC,

  /* 46 UTILITIES_SEL */               SETTINGS_SEL,                 UTILITIES_RUN,               UTILITIES_SEL,
  /* 47 UTILITIES_RUN */               UTILITIES_RUN,                UTILITIES_SEL,               UTILITIES_LOCK_DISPLAY_SEL,
  /* 48 UTILITIES_LOCK_DISPLAY_SEL */  UTILITIES_RUN,                UTILITIES_LOCK_DISPLAY_TOG,  UTILITIES_DUMP_DATA_SEL,
  /* 49 UTILITIES_DUMP_DATA_SEL */     UTILITIES_LOCK_DISPLAY_SEL,   UTILITIES_DUMP_DATA_EXE,     UTILITIES_LOAD_PARAM_SEL,
  /* 50 UTILITIES_LOAD_PARAM_SEL */    UTILITIES_DUMP_DATA_SEL,      UTILITIES_LOAD_PARAM_CONF,   UTILITIES_SAVE_PARAM_SEL,
  /* 51 UTILITIES_SAVE_PARAM_SEL */    UTILITIES_LOAD_PARAM_SEL,     UTILITIES_SAVE_PARAM_CONF,   UTILITIES_DEFAULTS_SEL,
  /* 52 UTILITIES_DEFAULTS_SEL */      UTILITIES_SAVE_PARAM_SEL,     UTILITIES_DEFAULTS_CONF,     UTILITIES_EXIT_SEL,
  /* 53 UTILITIES_EXIT_SEL */          UTILITIES_DEFAULTS_SEL,       UTILITIES_SEL,               UTILITIES_EXIT_SEL,

  /* 54 UTILITIES_LOAD_PARAM_CONF */   UTILITIES_LOAD_PARAM_SEL,     UTILITIES_LOAD_PARAM_CONF,   UTILITIES_LOAD_PARAM_EXE,
  /* 55 UTILITIES_SAVE_PARAM_CONF */   UTILITIES_SAVE_PARAM_SEL,     UTILITIES_SAVE_PARAM_CONF,   UTILITIES_SAVE_PARAM_EXE,
  /* 56 UTILITIES_DEFAULTS_CONF */     UTILITIES_DEFAULTS_SEL,       UTILITIES_DEFAULTS_CONF,     UTILITIES_DEFAULTS_EXE
};

// Declaration for 240x240 TFT LCD
Arduino_ST7789 tft = Arduino_ST7789(TFT_DC, TFT_RST);

uint8_t i = 0;
uint16_t j = 0;

uint8_t X_Index = 0;

uint8_t UBTemp1 = 0; // Unsigned 8 bit temps
uint8_t UBTemp2 = 0;
uint8_t UBTemp3 = 0;

int8_t SBTemp1 = 0; // Signed 8 bit temps
int8_t SBTemp2 = 0;
int8_t SBTemp3 = 0;

uint16_t UWTemp1 = 0; // Unsigned 16 bit temps
uint16_t UWTemp2 = 0;
uint16_t UWTemp3 = 0;

int16_t SWTemp1 = 0; // Signed 16 bit temps
int16_t SWTemp2 = 0;
int16_t SWTemp3 = 0;

uint32_t ULTemp1 = 0;
uint32_t ULTemp2 = 0;
uint32_t ULTemp3 = 0;

int32_t Count = 0;
int32_t old_Count = 0;

uint16_t Time_Delay = 0;

uint8_t SPANINT = 128;
int8_t OFSTINT = 0;
int8_t SPANOFSTINT = 0;

uint8_t Function = 255;
uint8_t realFunction = 0;

bool Function_Changed = 1;
bool Flag = LOW;

uint8_t Buttons_Function = COMBO;
uint8_t oldButtons_Function = COMBO;

uint8_t currentState = COMBO_RUN;
uint8_t previousState = COMBO_RUN;
uint8_t oldState = COMBO_RUN;
uint8_t nextState = 0;

bool Frequency_Detected = 0;
float Frequency_Value = 0;
float FTemp1 = 0;

float a = 0.0;

// These may be modified in the button interrupt handler
volatile uint16_t Time_Stamp = 0;
volatile uint16_t last_Time_Stamp = 30000;
volatile bool Interrupt_Flag = 0;
volatile uint8_t pressedButton = 0;
volatile bool Buttons_Used_Flag = 1;
volatile bool Skip_Flag = 0;

bool Dumped_Flag = LOW;
bool Buttons_Flag = LOW;
bool Header_Displayed_Flag = LOW;

int8_t PDigit001 = 111;
int8_t PDigit010 = 111;
int8_t PDigit100 = 111;

uint8_t oldPDigit001 = 111;
uint8_t oldPDigit010 = 111;
uint8_t oldPDigit100 = 111;

// The following four arrays must be maintained in this order for SFPI
uint8_t Presweep_PTrace[16];
uint8_t PTrace[260]; // Trace buffer for P mode (SFPI or MODE)
uint8_t Presweep_STrace[16];
uint8_t STrace[260]; // Trace buffer for S mode (SFPI or MODE)

uint8_t PFilter[260]; // Filter output buffer for P mode (SFPI)
uint8_t SFilter[260]; // Filter output buffer for S mode (SFPI)

uint16_t Kernal[32];

int16_t Filter_PBuffer[40];
int16_t Filter_SBuffer[40];

bool UCBValid = LOW;

// Settings Control Blocks - ACB, DCB, UCB

volatile int8_t ACB[16]; // Active Settings Control Block
volatile int8_t DCB[16]; // Default Settings Control Block

uint8_t Startup_State[8]; // Initial state for LPAR

void setup() {
  pinMode(Interpolate, INPUT_PULLUP);
  pinMode(Filter, INPUT_PULLUP);
  pinMode(Function_Select0, INPUT_PULLUP);
  pinMode(Function_Select1, INPUT_PULLUP);
  pinMode(Function_Select2, INPUT_PULLUP);
  pinMode(Dual_Polarization, INPUT_PULLUP);
  pinMode(Free_Run, INPUT_PULLUP);
  pinMode(Ramp_N, OUTPUT);
  pinMode(Ramp_P, OUTPUT);
  pinMode(Sample_P, OUTPUT);
  pinMode(Persistance, INPUT_PULLUP);

#ifdef FC
  pinMode(Frequency_Counter_Input, INPUT_PULLUP);
#endif

  // Dummy filter code to test memory limits
  for(i=0; i<240; i++) {
    for(j=0; j<FilterKernalSize; j++) {  // Move points to be filtered down 1
      Filter_PBuffer[j] = Filter_PBuffer[j+1];
      Filter_SBuffer[j] = Filter_SBuffer[j+1];
    }

    PFilter[i] = Filter_PBuffer[FilterKernalSize>>1];
    SFilter[i] = Filter_SBuffer[FilterKernalSize>>1];
  }

  Serial.begin(115200);
  Serial.print(F("Mini Laser Mode Analyzer"));
  Serial.print(F(" Version: "));
  Serial.println(FirmwareVersion);

#ifdef FC
  Serial.println(F("Frequency Counter Enabled"));
#endif

  tft.init(240, 240);   // initialize a ST7789 chip, 240x240 pixels

  Serial.println(F("Display Initialized"));

  tft.fillScreen(BG_Color);
  tft.setTextColor(LMA_Banner_Color);
  tft.setTextSize(3);
  tft.setCursor(35, 70);
  tft.println(F("Mini Laser"));
  tft.setCursor(5, 100);
  tft.println(F("Mode Analyzer"));
  tft.setTextSize(2);
  tft.setTextColor(LMA_Documentation_Color);
  tft.setCursor(25, 10);
  tft.println(F("Documentation at"));
  tft.setCursor(18, 35);
  tft.println(F("Repairfaq.Org/LMA/"));
  tft.setCursor(34, 185);
  tft.print(F("Sam's Laser FAQ"));
  tft.setCursor(10, 210);
  tft.print(F("Copyright 1994-2020"));
  tft.setTextColor(LMA_Version_Color);

#ifndef FC
  tft.setCursor(40, 150);
  tft.print(F("Version: "));
  tft.print(FirmwareVersion);
#endif

#ifdef FC
  tft.setCursor(40, 138);
  tft.print(F("Version: "));
  tft.print(FirmwareVersion);
  tft.setTextSize(1);
  tft.setTextColor(FREQUENCY_Color);
  tft.setCursor(38, 160);
  tft.print(F("(Frequency Counter Enabled)"));
#endif

  Get_Function();

  if((Function & 7) == 7) {
    Buttons_Flag = HIGH;
    PCICR &= !7;
    PCICR |= (1 << PCIE2);     // set PCIE2 to enable PCMSK2 scan
    PCMSK0 = 0;
    PCMSK1 = 0;

#ifndef FC
    PCMSK2 = (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20); // set PCINT2 to trigger an interrupt on D2,D3,D4 state change
#endif

#ifdef FC
    PCMSK2 = (1 << PCINT18) | (1 << PCINT20) | (1 << PCINT23); // set PCINT2 to trigger an interrupt on D2,D4,D7 state change
#endif

    interrupts();  // Enable button interrupts
  }

 // Generate filter kernal if required and send values to host
  if(Kernal_Span != 0) {
    Kernal_Gen(Kernal_Span);

    Serial.println(F("SFPI Antialiasing Filter Kernal, Span: "));
    Serial.println(Kernal_Span);

    for(j = 0; j < 32; j++) {
      a = Kernal[j];
      a = (a - 127.0) / 128.0;
      Serial.println(a);
    }
  }

  else {
    Serial.println(F("No SFPI Antialiasing Filter"));
  }
  
  delay_Millis (4000);
    
  Clearout(BG_Color);

  tft.drawFastHLine(0, topSeparator, 240, LINE_Color);  // Top separator
  tft.drawFastHLine(0, bottomSeparator, 240, LINE_Color); // Bottom separator

  // Set PWM frequency to ~63 kHz for pin 6 (also devides delay()/millis() times by 64)
  TCCR0B = (TCCR0B & 0b11111000) | 1; // No prescaling.

  TIMSK0 = 0; // Disable all TCNT0 interrupts
  
#ifdef FC
  // Set PWM frequency to ~63 kHz for pin 3 also
  TCCR2B = (TCCR2B & 0b11111000) | 1; // No prescaling.

  // Initialize Timer1 for REF frequency counter or nothing depending on version
  // (refer to ATMega328.pdf chapter 16-bit counter1)

  TCCR1A = 0;  // reset timer/counter1 control register A

               // set timer/counter1 hardware as counter, counts events on
               // pin T1 (arduino pin D5), normal mode, wgm10 .. wgm13 = 0

  TCCR1B = (_BV(CS10) | _BV(CS11) | _BV(CS12)); // Clk select external rising
#endif

#ifndef FC
  TCCR1B = 0; // Disable TIMER1 input
#endif

  TCNT1 = 0;   // Set counter value to 0
  
  // Initialize DCB with default values
  DCB[UCB_VALID] = EEPROMFormatValid; // EEPROM format valid
  DCB[STARTUP_FUNCTION] = COMBO;  // COMBO running
  DCB[STARTUP_STATE] = COMBO_RUN; // COMBO state
  DCB[POWER_CAL] = 10;            // Range: 1-100 Multiplier = CAL/10 mW
  DCB[POWER_RATE] = 10;           // Range: 1-10 Delay in ms = 5000/RATE-500
  DCB[CHART_RATE] = 10;           //   " "
  DCB[SCOPE_RATE] = 10;           //   " "
  DCB[SFPI_SPAN] = 100;           // Range: (128-1024)/4
  DCB[SFPI_OFFSET] = 0;           // Range: +(1024-SPAN)/4 to -(1024+SPAN)/4
  DCB[SFPI_KERNAL] = 1;           // 1 out of 10 max width
  DCB[SFPI_RATE] = 10;            // Delay in us = 2000/RATE-200
  DCB[COMBO_RATE] = 10;           // Delay in ms = 5000/RATE-500
  DCB[MODE_GAIN] = 8;            // Range:  (8-64) (5 V to 0.624 V FS) 8->gain of 1
  DCB[SFPI_GAIN] = 8;            //   " "
  DCB[ACB_FLAGS] = DUAL_POL_ENABLE | INTERP_ENABLE | FREE_RUN_ENABLE;
  DCB[ACB_CHECKSUM] = 0;          // Currently not computed

  UCBValid = 0;

  Startup_State[POWER] = POWER_RUN;
  Startup_State[CHART] = CHART_RUN;
  Startup_State[SCOPE] = SCOPE_RUN;
  Startup_State[SFPI] = SFPI_RUN;
  Startup_State[COMBO] = COMBO_RUN;

  LOAD_PARAMS();

  if(UCBValid == 0) {
    LOAD_DEFAULTS();
    SAVE_PARAMS();
    currentState = Startup_State[Function & 7];
  }
 
  Serial.println(F("Running"));
}

void Clearout(uint16_t color) {
  for(int16_t x = tft.width(); x>0; x--) {
    tft.drawRect((tft.width()-x)>>1, (tft.height()-x)>>1, x, x, color);
    delay_Millis (2);
  }
}

void delay_Millis (uint16_t d) {
  uint16_t k = 0;
  for (k = 0;k < d; k++) {
    delayMicroseconds (996);
  }
}

void loop() {

  Display_State();

  Get_Function(); // Determine operating function

  if(((Function & Free_Run_Flag) == 0) && (digitalRead(Free_Run) == LOW)) {   // Ignore trigger input if high or floating {
    tft.setCursor(228, 0);
    tft.setTextColor(GREEN);
    tft.print(F("+"));
  } else {
    tft.fillRect(228, 0, 240, 15, BG_Color);

    switch(Function & 7) {

    case DLHLD: // Download P and S trace buffers once ("v"), then hold (flashing "*").

      tft.setCursor(236, 0);
      tft.setTextColor(STATUS_Color);

      if(Dumped_Flag == LOW) {
        Dump_Data();
        Dumped_Flag = HIGH;
      }

      delay_Millis (250);
      tft.fillRect(236, 6, 2, 4, STATUS_Color);
      delay_Millis (250);
      tft.fillRect(236, 6, 2, 4, BG_Color);
      Time_Stamp += 5000;
      
      break;

    case POWER:  // Power meter display

      Function_Init();
      Acquire_Mode_Data(4);
      MODE_Annotate();
      UWTemp3 = (UWTemp3 * ACB[POWER_CAL]) / 10;

      if(UWTemp3 > 999) { UWTemp3 = 999; }

      Power_Check_Update(UWTemp3);

      Time_Delay = 250 * (11 - ACB[POWER_RATE]);
      delay_Millis (Time_Delay);
      Time_Stamp += ((Time_Delay << 3) + 500);

      break;

    case CHART: // Longitudinal mode display - Chart recorder scrolling

      Function_Init(); // Check if need to initialize

      // Erase old pixel from TFT, add new pixel
      tft.drawFastVLine(0, 24, fullSize, BG_Color);
      tft.drawFastVLine(1, 24, fullSize, BG_Color);

      for(j = 0; j < 239; j++) { // Scroll TFT and trace buffers left one pixel
        UWTemp1 = PTrace[j + 1] + STrace[j + 1]; // Total power

        if(UWTemp1 > fullLimit) { UWTemp1 = fullLimit; }

        UWTemp2 = PTrace[j] + STrace[j];

        if(UWTemp2 > fullLimit) { UWTemp2 = fullLimit; }

        tft.drawPixel(j + 2, fullBase - UWTemp2, BG_Color);   // Erase old Total pixel
        tft.drawPixel(j + 2, fullBase - PTrace[j], BG_Color); // Erase old PTrace pixel
        tft.drawPixel(j + 2, fullBase - STrace[j], BG_Color); // Erase old STrace pixel

        if(((Function & Interpolate_Flag) != 0)) {
          Interpolate_Trace(PTrace, j + 2, j, j + 2, fullBase, BG_Color);
          Interpolate_Trace(STrace, j + 2, j, j + 2, fullBase, BG_Color);
          Interpolate_TTrace(j + 2, j, j + 2, fullBase, fullLimit, BG_Color);
        }

        if((Function & Dual_Polarization_Flag) != 0) {
          if(STrace[j + 1] == PTrace[j + 1]) {
            tft.drawPixel(j+1, fullBase - PTrace[j + 1], SPMode_Overlap_Color);
          }

          else {
            tft.drawPixel(j + 1, fullBase - PTrace[j + 1], PMode_Color); // Move new PTrace value left one pixel
            tft.drawPixel(j + 1, fullBase - STrace[j + 1], SMode_Color); // Move new STrace value left one pixel
          }

          if(((Function & Interpolate_Flag) != 0) && (j > 1)) {
            Interpolate_Trace(PTrace, j+1, j, j+1, fullBase, PMode_Color);
            Interpolate_Trace(STrace, j+1, j, j+1, fullBase, SMode_Color);
            Overlap(j+1, j+1, fullBase, SPMode_Overlap_Color);  // Check for overlapping segments and replace with overlap color
          }
        }

        if((PTrace[j + 1] + STrace[j + 1]) > fullLimit) {
          tft.drawPixel(j + 1, fullBase - UWTemp1, MODE_Total_Overflow_Color);
        }

        else {
          tft.drawPixel(j + 1, fullBase - UWTemp1, MODE_Total_Color);
        }        // Move new Total value left one pixel

        if(((Function & Interpolate_Flag) != 0) && (j > 1)) {
          Interpolate_TTrace(j + 1, j, j + 1, fullBase, fullLimit, MODE_Total_Color);
        }

        PTrace[j] = PTrace[j + 1];
        STrace[j] = STrace[j + 1];
      }

      Acquire_Mode_Data(4);
      PTrace[239] = (UWTemp1 * 3) >> 4;
      STrace[239] = (UWTemp2 * 3) >> 4;
      MODE_Annotate();

      Time_Delay = 50 * (10 - ACB[CHART_RATE]);
      delay_Millis (Time_Delay);
      Time_Stamp += ((Time_Delay << 3) + 100);

      break;

    case SCOPE: // Longitudinal mode display - Scope formatting

      Function_Init(); // Check if need to initialize
      SCOPE_Code(4, fullBase, fullUpper, fullLimit, fullSize);
      PTrace[X_Index] = PTrace[New]; // Only for download
      STrace[X_Index] = STrace[New];

      if(X_Index != 239) { X_Index++; }
      else { X_Index = 0; }

      tft.drawFastVLine(X_Index, fullUpper, fullSize, MARKER_Color); // Add vertical line to the right of new point

      PTrace[X_Index] = 255;
      STrace[X_Index] = 255;

      break;

    case SFPI: // SFPI display

      Function_Init();
      SFPI_Code(0, fullBase, fullUpper, fullLimit, fullSize);
      UWTemp1 = analogRead(MSweep_PMode);
      UWTemp2 = analogRead(MSweep_SMode);
      MODE_Annotate(); // Display P and S mode values

      break;

    case COMBO: // SFPI and Longitudinal mode display - Scope formatting

      Function_Init();
      SFPI_Code(1, halfBase, fullUpper, halfLimit, halfSize);
      SCOPE_Code(5, fullBase, halfUpper, halfLimit, halfSize);

      if(X_Index != 239) { X_Index++; }
      else { X_Index = 0; }

      tft.drawFastVLine(X_Index, halfUpper, halfSize, MARKER_Color); // Add vertical line to the right of new point

      Time_Delay = 50 * (10 - ACB[COMBO_RATE]);
      delay_Millis (Time_Delay);
      Time_Stamp += ((Time_Delay << 3) + 100);
      
      break;

    case HOLD1: // Lock display, flash "*"
    case HOLD2:

      tft.fillRect(236, 6, 2, 4, STATUS_Color);
      delay_Millis (250);
      tft.fillRect(236, 6, 2, 4, BG_Color);
      delay_Millis (250);
      Time_Stamp += 5000; 
      break;
    }
  }
}

void SCOPE_Code(uint8_t shift, uint8_t base, uint8_t upper, uint8_t limit, uint8_t size) {
  tft.drawFastVLine(X_Index, upper, size, BG_Color); // Erase old column from LCD
  Acquire_Mode_Data(4);
  PTrace[Old] = PTrace[New];
  PTrace[New] = (UWTemp1 * 3) >> shift;
  STrace[Old] = STrace[New];
  STrace[New] = (UWTemp2 * 3) >> shift;

  // Draw new S mode, P mode, or Total pixel on LCD
  if((Function & Dual_Polarization_Flag) != 0) {
    if(PTrace[New] == STrace[New]) {
      tft.drawPixel(X_Index, base - PTrace[New], SPMode_Overlap_Color);
    }

    else {
      tft.drawPixel(X_Index, base - PTrace[New], PMode_Color);
      tft.drawPixel(X_Index, base - STrace[New], SMode_Color);
    }
  }

  UWTemp3 = PTrace[New] + STrace[New];

  if((UWTemp3) <= limit) {
    tft.drawPixel(X_Index, base - UWTemp3, MODE_Total_Color);
  }

  else {
    tft.drawPixel(X_Index, base - limit, MODE_Total_Overflow_Color);
  }

  // Interpolation
  if((Function & Interpolate_Flag) != 0) {
    if((Function & Dual_Polarization_Flag) == 0) {
      Interpolate_TTrace(Old, New, X_Index, base, limit, MODE_Total_Color);
    }

    else {
      Interpolate_Trace(PTrace, Old, New, X_Index, base, PMode_Color);
      Interpolate_Trace(STrace, Old, New, X_Index, base, SMode_Color);
      Overlap(New, X_Index, base, SPMode_Overlap_Color);  // Check for overlapping segments and replace with overlap color
      Interpolate_TTrace(Old, New, X_Index, base, limit, MODE_Total_Color);
    }
  }

  MODE_Annotate();
  
  Time_Delay = 50 * (10 - ACB[SCOPE_RATE]);
  delay_Millis (Time_Delay);
  Time_Stamp += ((Time_Delay << 3) + 100);
}

void SFPI_Code(uint8_t shift, uint8_t base, uint8_t upper, uint8_t limit, uint8_t size) {

  Time_Delay = 400 * (10 - ACB[SFPI_RATE]);
  Time_Stamp += ((Time_Delay) + 500);
    
  SPANINT = (ACB[SFPI_SPAN] * 64) / 50;
  OFSTINT = (ACB[SFPI_OFFSET] * 64) / 25;
  SPANOFSTINT = 128 - SPANINT + OFSTINT;

  // Pre-sweep - get ramp rolling with 16 steps out of 256
  // Start ramp to get past RC filter transient and keep timing the same, will be ignored
  for(j = 0; j < PreSweep; j++) {
    UBTemp1 = ((j * SPANINT) >> 7) + SPANOFSTINT;
    analogWrite(Ramp_P, UBTemp1);
    analogWrite(Ramp_N, 255-UBTemp1);

    Acquire_SFPI_Trace_Data(j-PreSweep); // Puts result near end of PTrace and STrace, will be overwritten
    PTrace[j-PreSweep] = PTrace[j-PreSweep] >> shift;
    STrace[j-PreSweep] = STrace[j-PreSweep] >> shift;
  }

  // Main capture loop
  for(j = 0; j < 240; j++) {
    UBTemp1 = (((j + PreSweep) * SPANINT) >> 7) + SPANOFSTINT;
    analogWrite(Ramp_P, UBTemp1);
    analogWrite(Ramp_N, 255-UBTemp1);   // Clear previous trace

    if((Function & Persistance_Flag) == 0) {
      tft.drawPixel(j, base - PTrace[j], BG_Color); // Erase old pixels

      if((Function & Dual_Polarization_Flag) != 0) {
        tft.drawPixel(j, base - STrace[j], BG_Color);
      }

      // Clear previous if interpolation
      if((Function & Interpolate_Flag) != 0) {
        Interpolate_Trace(PTrace, j + 1, j, j + 1, base, BG_Color);

        if((Function & Dual_Polarization_Flag) != 0) {
          Interpolate_Trace(STrace, j + 1, j, j + 1, base, BG_Color);
        }
      }
    }

    // Acquire new points
    Acquire_SFPI_Trace_Data(j); // Puts result in PTrace[j] and STrace[j]
    PTrace[j] = PTrace[j] >> shift;
    STrace[j] = STrace[j] >> shift;

    if((Function & Dual_Polarization_Flag) == 0) {
      UWTemp1 = PTrace[j] + STrace[j];

      if(UWTemp1 > limit) {
        PTrace[j] = limit;
      }

      else PTrace[j] = UWTemp1;
    }

    // Check if dual polarization
    if((Function & Dual_Polarization_Flag) != 0) {
      if(STrace[j] == PTrace[j]) {
        tft.drawPixel(j, base - PTrace[j], SPMode_Overlap_Color); // If identical
      } else {
        tft.drawPixel(j, base - PTrace[j], PMode_Color); // Write new PMode pixel
        tft.drawPixel(j, base - STrace[j], SMode_Color); // Write new SMode pixel
      }
    }

    else {
      tft.drawPixel(j, base - PTrace[j], MODE_Total_Color);
    } // Not dual polarization

    // Interpolation
    if((Function & Interpolate_Flag) != 0) {
      if((Function & Dual_Polarization_Flag) == 0) {
        Interpolate_Trace(PTrace, j, j-1, j, base, MODE_Total_Color);
      }

      else {
        Interpolate_Trace(PTrace, j, j-1, j, base, PMode_Color);
        Interpolate_Trace(STrace, j, j-1, j, base, SMode_Color);
        Overlap(j, j, base, SPMode_Overlap_Color);  // Check for overlapping segments and replace with overlap color
      }
    }
  }

  tft.drawFastVLine(0, upper, size, BG_Color);
  analogWrite(Ramp_P, 1);   // Avoid 0 glitch?
  analogWrite(Ramp_N, 254);
}

// Acquirre data for POWER, CHART, SCOPE, COMBO/SCOPE
void Acquire_Mode_Data(uint16_t t) {
  digitalWrite(Sample_P, HIGH);

  if((Function & Filter_Flag) != 0) { // Single point
    UWTemp1 = 0;
    UWTemp2 = 0;

    for(i = 0; i < N; i++) { // N point average
      UWTemp1 = UWTemp1 + analogRead(MSweep_PMode);
      UWTemp2 = UWTemp2 + analogRead(MSweep_SMode);
      delay_Millis (t>>NShift);
    }

    UWTemp1 = UWTemp1 >> NShift;
    UWTemp2 = UWTemp2 >> NShift;
  }

  else {
    UWTemp1 = analogRead(MSweep_PMode);
    UWTemp2 = analogRead(MSweep_SMode);
    delay_Millis (t);
  }

  UWTemp1 = (UWTemp1 * ACB[MODE_GAIN]) >> 3; // Mode gain from 1 to 8, MODE_GAIN of 4 -> actual gain of 1
  if(UWTemp1 > 1023) { UWTemp1 = 1023; }

  UWTemp2 = (UWTemp2 * ACB[MODE_GAIN]) >> 3;
  if(UWTemp2 > 1023) { UWTemp2 = 1023; }

  digitalWrite(Sample_P, LOW);
}

// Acquire data either directly or with 4X oversampling convolution
void Acquire_SFPI_Trace_Data(int16_t j) {

  if((Function & Filter_Flag) == 0) {
    UWTemp1 = analogRead(SFPI_PMode);
    UWTemp2 = analogRead(SFPI_SMode);
  }

  else { // Convolution someday, average for now
    UWTemp1 = 0;
    UWTemp2 = 0;

    for(i = 0; i < 4; i++) {
      UWTemp1 = UWTemp1 + analogRead(SFPI_PMode);
      UWTemp2 = UWTemp2 + analogRead(SFPI_SMode);
    }

    UWTemp1 = UWTemp1 >> 2;
    UWTemp2 = UWTemp2 >> 2;
  }

  UWTemp1 = (UWTemp1 * ACB[SFPI_GAIN]) >> 3; // SFPI gain from 1 to 8, MODE_GAIN of 4 -> actual gain of 1

  if(UWTemp1 > 1023) { UWTemp1 = 1023; }

  UWTemp2 = (UWTemp2 * ACB[SFPI_GAIN]) >> 3;

  if(UWTemp2 > 1023) { UWTemp2 = 1023; }

  PTrace[j] = (UWTemp1  * 3) >> 4;
  STrace[j] = (UWTemp2  * 3) >> 4;

  delayMicroseconds(Time_Delay);
}

// Get current function
void Get_Function() {

  realFunction = Function;
  Function = 0;

  if(Buttons_Flag == LOW) {
    if(digitalRead(Function_Select0) == HIGH) { Function |= Function0; }
    if(digitalRead(Function_Select1) == HIGH) { Function |= Function1; }
    if(digitalRead(Function_Select2) == HIGH) { Function |= Function2; }
    if(digitalRead(Persistance) == HIGH) { Function |= Persistance_Flag; }
    if(digitalRead(Dual_Polarization) == HIGH) { Function |= Dual_Polarization_Flag; }
    if(digitalRead(Interpolate) == HIGH) { Function |= Interpolate_Flag; }
    if(digitalRead(Filter) == HIGH) { Function |= Filter_Flag; }
    if(digitalRead(Free_Run) == HIGH) { Function |= Free_Run_Flag; }
  }

  else {
    Function = Buttons_Function;

    if((ACB[ACB_FLAGS] & PERSISTANCE_ENABLE) != 0) { Function |= Persistance_Flag; }
    if((ACB[ACB_FLAGS] & DUAL_POL_ENABLE) != 0) { Function |= Dual_Polarization_Flag; }
    if((ACB[ACB_FLAGS] & INTERP_ENABLE) != 0) { Function |= Interpolate_Flag; }
    if((ACB[ACB_FLAGS] & FILTER_ENABLE) != 0) { Function |= Filter_Flag; }
    if((ACB[ACB_FLAGS] & FREE_RUN_ENABLE) != 0) { Function |= Free_Run_Flag; }

    Buttons_Used_Flag = 1;
  }
}

void Function_Init() {
  Dumped_Flag = LOW;

  if((Function != realFunction) || (Function_Changed  != 0)) {
    Function_Changed = 0;

    tft.fillRect(170, 0, 73, 15, BG_Color);   // Clear function area
    tft.fillRect(0, 24, 240, 192, BG_Color);  // Clear display area
    tft.setCursor(170, 0);

    digitalWrite(Sample_P, LOW);

    switch(Function & 7) {

    case POWER:

      tft.setTextColor(POWER_Color);
      tft.print(F("POWER"));
      tft.fillRect(0, 21, 240, 196, BG_Color);
      tft.fillRect(0, 220, 240, 20, BG_Color);
      tft.setTextSize(4);
      tft.setTextColor(POWER_Text_Color);
      tft.setCursor(178, 110);
      tft.print(F("mW"));
      tft.setCursor(55, 119);
      tft.print(F("."));

      oldPDigit001 = 123;
      oldPDigit010 = 123;
      oldPDigit100 = 123;

      tft.setTextSize(2);

      break;

    case CHART:

      tft.setTextColor(CHART_Color);
      tft.print(F("CHART"));
      tft.drawFastHLine(0, fullBase, 240, SPMode_Overlap_Color); // Draw baseline

      break;

    case SCOPE:

      tft.setTextColor(SCOPE_Color);
      tft.print(F("SCOPE"));
      tft.drawFastHLine(0, fullBase, 240, SPMode_Overlap_Color); // Draw baseline

      break;

    case SFPI:

      tft.setTextColor(SFPI_Color);
      tft.print(F("SFPI"));
      tft.drawFastHLine(0, fullBase, 240, SPMode_Overlap_Color); // Draw baseline

      break;

    case COMBO:

      tft.setTextColor(COMBO_Color);
      tft.print(F("COMBO"));

      tft.drawFastHLine(0, middleSeparator, 240, LINE_Color); // Baseline
      tft.drawFastHLine(0, halfBase, 240, SPMode_Overlap_Color); // Baseline
      tft.drawFastHLine(0, fullBase, 240, SPMode_Overlap_Color); // Baseline

      break;
    }

    for(j = 0; j < 240; j++) {
      PTrace[j] = 0;   // Clear trace buffers
      STrace[j] = 0;
    }
    
    Bottom_Annotation_Init();
    X_Index = 0;
  }
}

void Bottom_Annotation_Init() {
  tft.setTextColor(MODE_Total_Color);
  tft.setCursor(8, 226);
  tft.print(F(":"));
  tft.setCursor(0, 226);
  tft.print(F("T"));
  tft.fillRect(20, 226, 50, 14, BG_Color);

  if((Function & Dual_Polarization_Flag) != 0) {
    tft.setTextColor(PMode_Color);
    tft.setCursor(95, 226);
    tft.print(F(":"));
    tft.setCursor(87, 226);
    tft.print(F("P"));
    tft.fillRect(106, 226, 50, 14, BG_Color);

    tft.setTextColor(SMode_Color);
    tft.setCursor(181, 226);
    tft.print(F(":"));
    tft.setCursor(173, 226);
    tft.print(F("S"));
    tft.fillRect(193, 226, 50, 14, BG_Color);
  }

  else {
    tft.fillRect(87, 226, 153, 14, BG_Color);
  }
}

void MODE_Annotate() {
  tft.drawPixel(236, 8, STATUS_Color); // Activity indicator
  UWTemp1 = (UWTemp1 * 21) / 43;
  UWTemp2 = (UWTemp2 * 21) / 43;
  UWTemp3 = (UWTemp1 + UWTemp2);

  tft.setTextColor(MODE_Total_Color);
  tft.setCursor(19, 226);
  tft.fillRect(19, 226, 50, 20, BG_Color);

  if(UWTemp3 > 499) { tft.setTextColor(MODE_Total_Overflow_Color); }

  tft.print(UWTemp3 * .01);

  if((Function & Dual_Polarization_Flag) != 0) {
    tft.setTextColor(PMode_Color);
    tft.setCursor(106, 226);
    tft.fillRect(106, 226, 50, 14, BG_Color);
    tft.print(UWTemp1 * .01);

    tft.setTextColor(SMode_Color);
    tft.setCursor(192, 226);
    tft.fillRect(192, 226, 50, 14, BG_Color);
    tft.print(UWTemp2 * .01);
  }

  tft.drawPixel(236, 8, BG_Color);
}

void Power_Check_Update(int16_t value) {
  PDigit001 = value % 10;
  PDigit010 = (value / 10) % 10;
  PDigit100 = value / 100;

  tft.setTextColor(POWER_Text_Color); // Main power readout
  tft.setTextSize(8);

  if(PDigit100 != oldPDigit100) {
    tft.fillRect(15, 70, 40, 80, BG_Color);
    tft.setCursor(15, 90);
    tft.print(PDigit100);
  }

  if(PDigit010 != oldPDigit010) {
    tft.fillRect(75, 70, 40, 80, BG_Color);
    tft.setCursor(75, 90);
    tft.print(PDigit010);
  }

  if(PDigit001 != oldPDigit001) {
    tft.fillRect(125, 70, 40, 80, BG_Color);
    tft.setCursor(125, 90);
    tft.print(PDigit001);
  }

  tft.setTextSize(2);

  oldPDigit001 = PDigit001;
  oldPDigit010 = PDigit010;
  oldPDigit100 = PDigit100;
}

// Interpolate between two points at PTrace or STrace locations k and l with result put in column m
void Interpolate_Trace(uint8_t Trace[], uint8_t k, uint8_t l, uint8_t m, uint8_t b, uint16_t color) {
  if((abs(Trace[k] - Trace[l])) > 1) {
    SWTemp1 = Trace[k] - Trace[l];

    if(SWTemp1 > 0) {
      tft.drawFastVLine(m, b - Trace[k], SWTemp1, color);
    } else {
      tft.drawFastVLine(m, b - Trace[l], -SWTemp1, color);
    }
  }
}

// Interpolate between two points at PTrace + STrace locations k and l with result put in column m
void Interpolate_TTrace(uint8_t k, uint8_t l, uint8_t m, uint16_t b, uint16_t s, uint16_t color) {

  SWTemp2 = PTrace[k] + STrace[k];

  if(SWTemp2 > s) { SWTemp2 = s; }

  SWTemp3 = PTrace[l] + STrace[l];

  if(SWTemp3 > s) { SWTemp3 = s; }

  if(abs(SWTemp2 - SWTemp3) > 1) {
    SWTemp1 = SWTemp2 - SWTemp3;

    if(SWTemp1 > 0) {
      tft.drawFastVLine(m, b - SWTemp2, SWTemp1, color);
    } else {
      tft.drawFastVLine(m, b - SWTemp3, -SWTemp1, color);
    }
  }
}

void Overlap(uint8_t k, uint8_t l, uint8_t b, uint16_t color) {

  if((abs(PTrace[k] - PTrace[k-1]) > 1) && (abs(STrace[k] - STrace[k-1]) > 1)) {
    if((PTrace[k] < PTrace[k - 1]) && (STrace[k] < STrace[k - 1])) {
      if(((PTrace[k - 1] - STrace[k]) >= 0) && ((STrace[k - 1] - PTrace[k]) >= 0)) {
        // overlap
        Start = max(PTrace[k], STrace[k]);
        End = min(PTrace[k - 1], STrace[k - 1]); // could be a point [x, x]
        tft.drawFastVLine(l, b - End, End - Start, color);
      }                                          // else no overlap
    }

    else if((PTrace[k] > PTrace[k - 1]) && (STrace[k] < STrace[k - 1])) {
      if(((PTrace[k] - STrace[k]) >= 0) && ((STrace[k - 1] - PTrace[k - 1]) >= 0)) {
        // overlap
        Start = max(PTrace[k - 1], STrace[k]);
        End = min(PTrace[k], STrace[k - 1]); // could be a point [x, x]
        tft.drawFastVLine(k, b - End, End - Start, color);
      }                                      // else no overlap
    }

    else if((PTrace[k] < PTrace[k - 1]) && (STrace[k] > STrace[k - 1])) {
      if(((PTrace[k - 1] - STrace[k - 1]) >= 0) && ((STrace[k] - PTrace[k]) >= 0)) {
        // overlap
        Start = max(PTrace[k], STrace[k - 1]);
        End = min(PTrace[k - 1], STrace[k]); // could be a point [x, x]
        tft.drawFastVLine(k, b - End, End - Start, color);
      }                                      // else no overlap
    }

    else if((PTrace[k] > PTrace[k - 1]) && (STrace[k] > STrace[k - 1])) {
      if(((PTrace[k] - STrace[k - 1]) >= 0) && ((STrace[k] - PTrace[k - 1]) >= 0)) {
        // overlap
        Start = max(PTrace[k - 1], STrace[k - 1]);
        End = min(PTrace[k], STrace[k]);     // could be a point [x, x]
        tft.drawFastVLine(k, b - End, End - Start, color);
      }                                      // else no overlap
    }
  }
}

// Generate 32 value symmetric filter kernal for SFPI antialiaing, Kernal_span determines width
// Output is fixed point 0 to 255 equaling -1.0 to 0.99.
void Kernal_Gen(float span) {
  float x = 0;
  float y = 0;

  if(Kernal_Span == 0) { return; }

  Kernal[16] = 255;

  for(j=1; j<16; j++) {
    x = j * span;
    y = sin(x) / x;
    Kernal[16+j] = (y * 128) + 128;
    Kernal[16-j] = Kernal[16+j];
  }
}

void Get_Next_State() {
  if(pressedButton != 0) {
    previousState = currentState;
    currentState =  pgm_read_byte_near(State_Memory + (3 * currentState) + pressedButton - 1);
    pressedButton = 0;
  }
}

void State_Action() {

  switch(currentState) {

  case POWER_CAL_DEC:

    PARAM_DEC(60, POWER_CAL, 1, POWER_Color);
    Display_CAL(60, POWER_Color);

    currentState = POWER_CAL_RUN;

    break;

  case POWER_CAL_INC:

    PARAM_INC(60, POWER_CAL, 100, POWER_Color);
    Display_CAL(60, POWER_Color);

    currentState = POWER_CAL_RUN;

    break;

  case POWER_RATE_DEC:

    PARAM_DEC(66, POWER_RATE, 1, POWER_Color);
    Display_RATE(54, POWER_Color);

    currentState = POWER_RATE_RUN;

    break;

  case POWER_RATE_INC:

    PARAM_INC(66, POWER_RATE, 10, POWER_Color);
    Display_RATE(54, POWER_Color);

    currentState = POWER_RATE_RUN;

    break;

  case CHART_RATE_DEC:

    PARAM_DEC(66, CHART_RATE, 1, CHART_Color);
    Display_RATE(54, CHART_Color);

    currentState = CHART_RATE_RUN;

    break;

  case CHART_RATE_INC:

    PARAM_INC(66, CHART_RATE, 10, CHART_Color);
    Display_RATE(54, CHART_Color);

    currentState = CHART_RATE_RUN;

    break;

  case SCOPE_RATE_DEC:

    PARAM_DEC(66, SCOPE_RATE, 1, SCOPE_Color);
    Display_RATE(54, SCOPE_Color);

    currentState = SCOPE_RATE_RUN;

    break;

  case SCOPE_RATE_INC:

    PARAM_INC(66, SCOPE_RATE, 10, SCOPE_Color);
    Display_RATE(54, SCOPE_Color);

    currentState = SCOPE_RATE_RUN;

    break;

  case SFPI_PERSIST_TOG:

    Display_SFPI(0, BUTTON1_Color);

    if((ACB[ACB_FLAGS] & PERSISTANCE_ENABLE) == 0) {
      ACB[ACB_FLAGS] |= PERSISTANCE_ENABLE;
      Display_PERS(54, ON_Color);
    }

    else {
      ACB[ACB_FLAGS] &= ~PERSISTANCE_ENABLE;
      Display_PERS(54, OFF_Color);
    }

    Display_SPAN(108,BUTTON3_Color);

    currentState = SFPI_PERSIST_SEL;

    break;

  case SFPI_SPAN_DEC:

    PARAM_DEC(60, SFPI_SPAN, 10, SFPI_Color);

    Display_SPAN(54, SFPI_Color);

    currentState = SFPI_SPAN_RUN;

    break;

  case SFPI_SPAN_INC:

    PARAM_INC(60, SFPI_SPAN, 100, SFPI_Color);

    SWTemp1 = (100 - ACB[SFPI_SPAN]) >> 1;

    if(ACB[SFPI_OFFSET] > SWTemp1) { ACB[SFPI_OFFSET] = SWTemp1; }
    if(ACB[SFPI_OFFSET] < -SWTemp1) { ACB[SFPI_OFFSET] = -SWTemp1; }

    Display_SPAN(54, SFPI_Color);

    currentState = SFPI_SPAN_RUN;

    break;

  case SFPI_OFFSET_DEC:

    if (ACB[SFPI_SPAN] != 100) {
      PARAM_DEC(60, SFPI_OFFSET, ((-99 + ACB[SFPI_SPAN]) >> 1), SFPI_Color);
    }

    else {
      Display_Param_Value (60, SFPI_OFFSET, SFPI_Color);
    }

    Display_OFST(54, SFPI_Color);

    currentState = SFPI_OFFSET_RUN;

    break;

  case SFPI_OFFSET_INC:

    if (ACB[SFPI_SPAN] != 100) {
      PARAM_INC(60, SFPI_OFFSET, ((100 - ACB[SFPI_SPAN]) >> 1), SFPI_Color);
    }

    else {
      Display_Param_Value (60, SFPI_OFFSET, SFPI_Color);
    }
    
    Display_OFST(54, SFPI_Color);

    currentState = SFPI_OFFSET_RUN;

    break;

  case SFPI_KERNAL_DEC:

    PARAM_DEC(66, SFPI_KERNAL, 1, SFPI_Color);

    Display_KERN(54, SFPI_Color);

    currentState = SFPI_KERNAL_RUN;

    break;

  case SFPI_KERNAL_INC:

    PARAM_INC(66, SFPI_KERNAL, 10, SFPI_Color);

    Display_KERN(54, SFPI_Color);

    currentState = SFPI_KERNAL_RUN;

    break;

  case SFPI_RATE_DEC:

    PARAM_DEC(66, SFPI_RATE, 1, SFPI_Color);
    Display_RATE(54, SFPI_Color);

    currentState = SFPI_RATE_RUN;

    break;

  case SFPI_RATE_INC:

    PARAM_INC(66, SFPI_RATE, 10, SFPI_Color);
    Display_RATE(54, SFPI_Color);

    currentState = SFPI_RATE_RUN;

    break;

  case COMBO_RATE_DEC:

    PARAM_DEC(66, COMBO_RATE, 1, COMBO_Color);
    Display_RATE(54, COMBO_Color);

    currentState = COMBO_RATE_RUN;

    break;

  case COMBO_RATE_INC:

    PARAM_INC(66, COMBO_RATE, 10, COMBO_Color);
    Display_RATE(54, COMBO_Color);

    currentState = COMBO_RATE_RUN;

    break;

  case SETTINGS_MODE_GAIN_DEC:

    PARAM_DEC(66, MODE_GAIN, 8, MODE_Color);
    Display_MDGN(54, MODE_Color);

    currentState = SETTINGS_MODE_GAIN_RUN;

    break;

  case SETTINGS_MODE_GAIN_INC:

    PARAM_INC(66, MODE_GAIN, 64, MODE_Color);
    Display_MDGN(54, MODE_Color);

    currentState = SETTINGS_MODE_GAIN_RUN;

    break;

  case SETTINGS_SFPI_GAIN_DEC:

    PARAM_DEC(66, SFPI_GAIN, 8, SFPI_Color);
    Display_SFGN(54, SFPI_Color);

    currentState = SETTINGS_SFPI_GAIN_RUN;

    break;

  case SETTINGS_SFPI_GAIN_INC:

    PARAM_INC(66, SFPI_GAIN, 64, SFPI_Color);
    Display_SFGN(54, SFPI_Color);

    currentState = SETTINGS_SFPI_GAIN_RUN;

    break;

  case SETTINGS_DUAL_POL_TOG:

    Display_SETG(0, BUTTON1_Color);

    if((ACB[ACB_FLAGS] & DUAL_POL_ENABLE) == 0) {
      ACB[ACB_FLAGS] |= DUAL_POL_ENABLE;
      Display_DPOL(54, ON_Color);
    }

    else {
      ACB[ACB_FLAGS] &= ~DUAL_POL_ENABLE;
      Display_DPOL(54, OFF_Color);
    }

    Display_INTP(108, BUTTON3_Color);

    currentState = SETTINGS_DUAL_POL_SEL;

    break;

  case SETTINGS_INTERP_TOG:

    Display_DPOL(0, BUTTON1_Color);

    if((ACB[ACB_FLAGS] & INTERP_ENABLE) == 0) {
      ACB[ACB_FLAGS] |= INTERP_ENABLE;
      Display_INTP(54, ON_Color);
    }

    else {
      ACB[ACB_FLAGS] &= ~INTERP_ENABLE;
      Display_INTP(54, OFF_Color);
    }

    Display_FILT(108, BUTTON3_Color);

    currentState = SETTINGS_INTERP_SEL;

    break;

  case SETTINGS_FILTER_TOG:

    Display_INTP(0, BUTTON1_Color);

    if((ACB[ACB_FLAGS] & FILTER_ENABLE) == 0) {
      ACB[ACB_FLAGS] |= FILTER_ENABLE;
      Display_FILT(54, ON_Color);
    }

    else {
      ACB[ACB_FLAGS] &= ~FILTER_ENABLE;
      Display_FILT(54, OFF_Color);
    }

    Display_FRRN(108, BUTTON3_Color);

    currentState = SETTINGS_FILTER_SEL;

    break;

  case SETTINGS_FREE_RUN_TOG:

    Display_FILT(0, BUTTON1_Color);

    if((ACB[ACB_FLAGS] & FREE_RUN_ENABLE) == 0) {
      ACB[ACB_FLAGS] |= FREE_RUN_ENABLE;
      Display_FRRN(54, ON_Color);
    }

    else {
      ACB[ACB_FLAGS] &= ~FREE_RUN_ENABLE;
      Display_FRRN(54, OFF_Color);
    }

    Display_EXIT(108, BUTTON3_Color);

    currentState = SETTINGS_FREE_RUN_SEL;

    break;

  case UTILITIES_LOCK_DISPLAY_TOG:

    Display_UTIL(0, BUTTON1_Color);

    if((Function & 7) != HOLD1) {
      UWTemp1 = Function;
      Function = (Function & ~7) | HOLD1;
      Buttons_Function = HOLD1;
      Display_LOCK(54, ON_Color);
    }

    else {
      Function = UWTemp1;
      Buttons_Function = UWTemp1;
      Display_LOCK(54, OFF_Color);
    }

    Display_DUMP(108, BUTTON3_Color);

    currentState = UTILITIES_LOCK_DISPLAY_SEL;

    break;

  case UTILITIES_DUMP_DATA_EXE:

    Display_LOCK(0, BUTTON1_Color);
    Display_DUMP(54, EXECUTE_Color);
    Display_LPAR(108, BUTTON3_Color);

    Dump_Data();

    Display_DUMP(54, DUMP_Color);

    currentState = UTILITIES_DUMP_DATA_SEL;

    break;

  case UTILITIES_LOAD_PARAM_EXE:

    if (UCBValid == 1) {
      Display_YES(108, EXECUTE_Color);
      LOAD_PARAMS();
      delay_Millis (250);
    }
   
    Interrupt_Flag = 1;
    currentState = UTILITIES_LOAD_PARAM_SEL;

    break;

  case UTILITIES_SAVE_PARAM_EXE:

    Display_YES(108, EXECUTE_Color);

    SAVE_PARAMS();
    delay_Millis (250);
    
    Interrupt_Flag = 1;
    currentState = UTILITIES_SAVE_PARAM_SEL;

    break;

  case UTILITIES_DEFAULTS_EXE:

    Display_YES(108, EXECUTE_Color);

    LOAD_DEFAULTS();
    delay_Millis (250);
    
    Display_RATE(54, COMBO_Color);

    Interrupt_Flag = 1;
    currentState = UTILITIES_DEFAULTS_SEL;

    break;
  }
}

void Display_State() {

  State_Action();
  
    if (((Time_Stamp - last_Time_Stamp) > 30000) && (Header_Displayed_Flag == 0)) {
      tft.fillRect(0, 0, 164, 17, BG_Color);
      tft.setTextColor(TEXT_Color);
      tft.setTextSize(1); 
      last_Time_Stamp = Time_Stamp;
         
      if (Frequency_Detected == 0) { 
        tft.setCursor(2, 0);     // Start at top-left corner
        tft.print(F("Mini Laser Mode Analyzer"));
        tft.setCursor(149, 0);
        tft.print(F("1"));
        tft.setCursor(12, 9);
        tft.print(F("Firmware Version: "));       
        tft.print(FirmwareVersion);
        tft.setTextSize(2); 
      }

      else {
        tft.setCursor(5, 0);
        tft.print(F("mLMA1"));
        tft.setCursor(4, 9);
        tft.print(F("V"));
        tft.print(FirmwareVersion);
                    
        tft.setTextColor(FREQUENCY_Color);    
        tft.setTextSize(2);
        tft.setCursor(50, 0);    
        tft.print(F(":"));
        tft.setCursor(43, 0);
        tft.print(F("F"));
        tft.setCursor(127, 0);
        tft.print(F("MHz"));
        old_Count = -1; // Force refresh
      }
   
      Header_Displayed_Flag = 1;
  }   

  if (Frequency_Detected == 1) {
    if (Header_Displayed_Flag == 1) {
      ULTemp1 = TCNT1;        // Calculate frequency, gate time = 1 ms
      delayMicroseconds(997); // Actual delay runs with interrupts disabled, 1000-overhead
      UWTemp1 = 11111;        // Fine tuning delay kludge - adds 8 clock cycles :)
      ULTemp2 = TCNT1;
      
      Count = (ULTemp2 - ULTemp1);
        
      if (ULTemp2 < ULTemp1) { // Rolled over
        Count += 0x10000;
      }
 
      if (old_Count != Count) {
        old_Count = Count;
        Frequency_Value = Count;
        Frequency_Value /= 1000;
        tft.setCursor(61, 0); // Update frequency display
        tft.fillRect(61, 0, 60, 17, BG_Color);
 
        if (Count > 6400) {
          tft.setTextColor(FREQUENCY_OVERFLOW_Color);
          tft.print (F("-.---"));
        }
        
        else {
          if (Count > 4000) {
            tft.setTextColor(FREQUENCY_WARNING_Color);
          }
         
          else {
            tft.setTextColor(FREQUENCY_Color);
          }

        tft.print (Frequency_Value, 3);
        }
      }
    }
  }
  
  else if (TCNT1L != 0) {
    Frequency_Detected = 1;
    Header_Displayed_Flag = 0;  
    Time_Stamp = last_Time_Stamp + 20000; // millis() - 8000;
  } 
      
  if(Interrupt_Flag != 0) {
    Interrupt_Flag = 0;
    Header_Displayed_Flag = 0;
    Get_Next_State();

    if((currentState != oldState) || (Skip_Flag == 1)) {
      Skip_Flag = 0;

      if(currentState < 100) { tft.fillRect(0, 0, 164, 17, BG_Color); }

      oldState = currentState;

      switch(currentState) {

      case RESERVED:
        break;

      case POWER_SEL:

        Display_PWR(59, POWER_Color);
        Display_CHRT(108, BUTTON3_Color);

        break;

      case POWER_RUN:

        Display_PWR(52, POWER_Color);
        Display_CAL(108, BUTTON3_Color);

        if(previousState == POWER_SEL) { Function_Changed = 1; }
        previousState = POWER_RUN;
        Buttons_Function = POWER;

        break;

      case POWER_CAL_SEL:

        Display_PWR(7, BUTTON1_Color);
        Display_CAL(60, POWER_Color);
        Display_RATE(108, BUTTON3_Color);

        break;

      case POWER_RATE_SEL:

        Display_CAL(0, BUTTON1_Color);
        Display_RATE(54, POWER_Color);
        Display_EXIT(108, BUTTON3_Color);

        break;

      case POWER_EXIT_SEL:

        Display_RATE(0, BUTTON1_Color);
        Display_EXIT(54, EXIT_Color);

        break;

      case POWER_CAL_RUN:

        Display_LIMIT_PLUSMINUS (POWER_CAL, 10, 100);
        Display_Param_Value (60, POWER_CAL, POWER_Color);
        Display_CAL(60, POWER_Color);

      break;

      case POWER_RATE_RUN:
 
        Display_LIMIT_PLUSMINUS (POWER_RATE, 1, 10);
        Display_Param_Value (66, POWER_RATE, POWER_Color);
        Display_RATE(54, POWER_Color);
  
        break;

      case CHART_SEL:

        Display_PWR(0, BUTTON1_Color);
        Display_CHRT(54, CHART_Color);
        Display_SCPE(108, BUTTON3_Color);

        break;

      case CHART_RUN:

        Display_CHRT(54, CHART_Color);
        Display_RATE(108, BUTTON3_Color);

        if(previousState == CHART_SEL) { Function_Changed = 1; }
        previousState = CHART_RUN;
        Buttons_Function = CHART;

        if(previousState == CHART_SEL) { Function_Changed = 1; }

        break;

      case CHART_RATE_SEL:

        Display_CHRT(0, BUTTON1_Color);
        Display_RATE(54, CHART_Color);
        Display_EXIT(108, BUTTON3_Color);

        break;

      case CHART_EXIT_SEL:

        Display_RATE(0, BUTTON1_Color);
        Display_EXIT(54, EXIT_Color);

        break;

      case CHART_RATE_RUN:
  
        Display_LIMIT_PLUSMINUS (CHART_RATE, 1, 10);
        Display_Param_Value (66, CHART_RATE, CHART_Color);
        Display_RATE(54, CHART_Color);

        break;

      case SCOPE_SEL:

        Display_CHRT(0, BUTTON1_Color);
        Display_SCPE(54, SCOPE_Color);
        Display_SFPI(108, BUTTON3_Color);

        break;

      case SCOPE_RUN:

        Display_SCPE(54,SCOPE_Color);
        Display_RATE(108,BUTTON3_Color);

        if(previousState == SCOPE_SEL) { Function_Changed = 1; }
        previousState = SCOPE_RUN;
        Buttons_Function = SCOPE;

        if(previousState == SCOPE_SEL) { Function_Changed = 1; }

        break;

      case SCOPE_RATE_SEL:

        Display_SCPE(0, BUTTON1_Color);
        Display_RATE(54, SCOPE_Color);
        Display_EXIT(108, BUTTON3_Color);

        break;

      case SCOPE_EXIT_SEL:

        Display_RATE(0, BUTTON1_Color);
        Display_EXIT(54, EXIT_Color);

        break;

      case SCOPE_RATE_RUN:
  
        Display_LIMIT_PLUSMINUS (SCOPE_RATE, 1, 10);
        Display_Param_Value (66, SCOPE_RATE, SCOPE_Color);
        Display_RATE(54, SCOPE_Color);
      
      break;

      case SFPI_SEL:

        Display_SCPE(0, BUTTON1_Color);
        Display_SFPI(54, SFPI_Color);
        Display_COMB(108, BUTTON3_Color);

        break;

      case SFPI_RUN:

        Display_SFPI(54, SFPI_Color);
        Display_PERS(108, BUTTON3_Color);

        if(previousState == SFPI_SEL) { Function_Changed = 1; }
        previousState = SFPI_RUN;
        Buttons_Function = SFPI;

        break;

      case SFPI_PERSIST_SEL:

        Display_SFPI(0, BUTTON1_Color);

        if((ACB[ACB_FLAGS] & PERSISTANCE_ENABLE) == 0) { Display_PERS(54, OFF_Color); }
        else { Display_PERS(54, ON_Color); }

        Display_SPAN(108,BUTTON3_Color);

        break;

      case SFPI_SPAN_SEL:

        Display_PERS(0, BUTTON1_Color);
        Display_SPAN(54, SFPI_Color);
        Display_OFST(108, BUTTON3_Color);

        break;

      case SFPI_OFFSET_SEL:

        Display_SPAN(0, BUTTON1_Color);
        Display_OFST(54, SFPI_Color);
        Display_KERN(108, BUTTON3_Color);

        break;

      case SFPI_KERNAL_SEL:

        Display_OFST(0, BUTTON1_Color);
        Display_KERN(54, SFPI_Color);
        Display_RATE(108, BUTTON3_Color);

        break;

      case SFPI_RATE_SEL:

        Display_KERN(0, BUTTON1_Color);
        Display_RATE(54, SFPI_Color);
        Display_EXIT(108, BUTTON3_Color);

        break;

      case SFPI_EXIT_SEL:

        Display_RATE(0, BUTTON1_Color);
        Display_EXIT(54, EXIT_Color);

        break;

      case SFPI_SPAN_RUN:

        Display_LIMIT_PLUSMINUS (SFPI_SPAN, 10, 100);
        Display_Param_Value (60, SFPI_SPAN, SFPI_Color);
        Display_SPAN(54, SFPI_Color);
        break;

      case SFPI_OFFSET_RUN:

        Display_LIMIT_PLUSMINUS (SFPI_OFFSET, -((99 - ACB[SFPI_SPAN]) >> 1),  ((100 - ACB[SFPI_SPAN]) >> 1));
        Display_Param_Value (60, SFPI_OFFSET, SFPI_Color);
        Display_OFST(54, SFPI_Color);

      break;

      case SFPI_KERNAL_RUN:

        Display_LIMIT_PLUSMINUS (SFPI_KERNAL, 1, 10);
        Display_Param_Value (66, SFPI_KERNAL, SFPI_Color);
        Display_KERN(54, SFPI_Color);

      break;

      case SFPI_RATE_RUN:
   
        Display_LIMIT_PLUSMINUS (SFPI_RATE, 1, 10);
        Display_Param_Value (66, SFPI_RATE, SFPI_Color);
        Display_RATE(54, SFPI_Color);
        
        break;

      case COMBO_SEL:

        Display_SFPI(0, BUTTON1_Color);
        Display_COMB(54,COMBO_Color);
        Display_SETG(108, BUTTON3_Color);

        break;

      case COMBO_RUN:

        Display_COMB(54,COMBO_Color);
        Display_RATE(108,BUTTON3_Color);

        if(previousState == COMBO_SEL) { Function_Changed = 1; }
        previousState = COMBO_RUN;
        Buttons_Function = COMBO;

        break;

      case COMBO_RATE_SEL:

        Display_COMB(0, BUTTON1_Color);
        Display_RATE(54, COMBO_Color);
        Display_EXIT(108, BUTTON3_Color);

        break;

      case COMBO_RATE_RUN:

        Display_LIMIT_PLUSMINUS (COMBO_RATE, 1, 10);
        Display_Param_Value (66, COMBO_RATE, COMBO_Color);
        Display_RATE(54, COMBO_Color);

        break;

      case COMBO_EXIT_SEL:

        Display_RATE(0, BUTTON1_Color);
        Display_EXIT(54, EXIT_Color);

        break;

      case SETTINGS_SEL:

        Display_COMB(0, BUTTON1_Color);
        Display_SETG(54, SETTINGS_Color);
        Display_UTIL(108, BUTTON3_Color);

        break;

      case SETTINGS_RUN:

        Display_SETG(54, SETTINGS_Color);
        Display_MDGN(108, BUTTON3_Color);

        break;

      case SETTINGS_MODE_GAIN_SEL:

        Display_SETG(0, BUTTON1_Color);
        Display_MDGN(54, MODE_Color);
        Display_SFGN(108, BUTTON3_Color);

        break;

      case SETTINGS_SFPI_GAIN_SEL:

        Display_MDGN(0, BUTTON1_Color);
        Display_SFGN(54, SFPI_Color);
        Display_DPOL(108, BUTTON3_Color);

        break;

      case SETTINGS_DUAL_POL_SEL:

        Display_SFGN(0, BUTTON1_Color);

        if((ACB[ACB_FLAGS] & DUAL_POL_ENABLE) == 0) { Display_DPOL(54, OFF_Color); }
        else { Display_DPOL(54, ON_Color); }

        Display_INTP(108, BUTTON3_Color);

        break;

      case SETTINGS_INTERP_SEL:

        Display_DPOL(0, BUTTON1_Color);

        if((ACB[ACB_FLAGS] & INTERP_ENABLE) == 0) { Display_INTP(54, OFF_Color); }
        else { Display_INTP(54, ON_Color); }

        Display_FILT(108, BUTTON3_Color);

        break;

      case SETTINGS_FILTER_SEL:

        Display_INTP(0, BUTTON1_Color);

        if((ACB[ACB_FLAGS] & FILTER_ENABLE) == 0) { Display_FILT(54, OFF_Color); }
        else { Display_FILT(54, ON_Color); }

        Display_FRRN(108, BUTTON3_Color);

        break;

      case SETTINGS_FREE_RUN_SEL:

        Display_FILT(0, BUTTON1_Color);

        if((ACB[ACB_FLAGS] & FREE_RUN_ENABLE) == 0) { Display_FRRN(54, OFF_Color); }
        else { Display_FRRN(54, ON_Color); }

        Display_EXIT(108, BUTTON3_Color);

        break;

      case SETTINGS_EXIT_SEL:

        Display_FRRN(0, BUTTON1_Color);
        Display_EXIT(54, EXIT_Color);

        break;

      case SETTINGS_MODE_GAIN_RUN:

        Display_LIMIT_PLUSMINUS (MODE_GAIN, 8, 64);
        Display_Param_Value (66, MODE_GAIN, MODE_Color);
        Display_MDGN(54, MODE_Color);


        break;

      case SETTINGS_SFPI_GAIN_RUN:

        Display_LIMIT_PLUSMINUS (SFPI_GAIN, 8, 64);
        Display_Param_Value (66, SFPI_GAIN, SFPI_Color);
        Display_SFGN(54, SFPI_Color);
  
        break;

      case UTILITIES_SEL:

        Display_SETG(0, BUTTON1_Color);
        Display_UTIL(54, UTILITIES_Color);

        break;

      case UTILITIES_RUN:

        Display_UTIL(54, UTILITIES_Color);
        Display_LOCK(108, BUTTON3_Color);

        break;

      case UTILITIES_LOCK_DISPLAY_SEL:

        Display_UTIL(0, BUTTON1_Color);

        if((Function & 7) != HOLD1) {
          Display_LOCK(54, OFF_Color);
        }

        else {
          Display_LOCK(54, ON_Color);
        }

        Display_DUMP(108, BUTTON3_Color);

        break;

      case UTILITIES_DUMP_DATA_SEL:

        Display_LOCK(0, BUTTON1_Color);
        Display_DUMP(54, DUMP_Color);
        Display_LPAR(108,BUTTON3_Color);

        break;

      case UTILITIES_LOAD_PARAM_SEL:

        Display_DUMP(0, BUTTON1_Color);
        Display_LPAR(54, MAGENTA);
        Display_SPAR(108, BUTTON3_Color);

        break;

      case UTILITIES_SAVE_PARAM_SEL:

        Display_LPAR(0,BUTTON1_Color);
        Display_SPAR(54, MAGENTA);
        Display_DFLT(108,BUTTON3_Color);

        break;

        currentState = UTILITIES_DEFAULTS_SEL;

      case UTILITIES_DEFAULTS_SEL:

        Display_SPAR(0, BUTTON1_Color);
        Display_DFLT(54, MAGENTA);
        Display_EXIT(108, BUTTON3_Color);

        break;

      case UTILITIES_EXIT_SEL:

        Display_DFLT(0, BUTTON1_Color);
        Display_EXIT(54, EXIT_Color);

        break;

      case UTILITIES_LOAD_PARAM_CONF:
      case UTILITIES_SAVE_PARAM_CONF:
      case UTILITIES_DEFAULTS_CONF:

        Display_NOYES();

        break;
      }
    }
  }
}

void Display_CAL(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("CAL"));
}

void Display_CHRT(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("CHRT"));
}

void Display_COMB(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("COMB"));
}

void Display_DFLT(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("DFLT"));
}

void Display_DPOL(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("DPOL"));
}

void Display_DUMP(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("DUMP"));
}

void Display_EXIT(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("EXIT"));
}

void Display_FILT(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("FILT"));
}

void Display_FRRN(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("FRRN"));
}

void Display_INTP(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("INTP"));
}

void Display_KERN(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("KERN"));
}

void Display_LOCK(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("LOCK"));
}

void Display_LPAR(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("LPAR"));
}

void Display_MDGN(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("MDGN"));
}

void Display_MINUS(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("-"));
}

void Display_NO(uint8_t x1) {
  tft.setTextColor(DARKRED);
  tft.setCursor(x1,0);
  tft.println(F("NO"));
}

void Display_OFST(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("OFST"));
}

void Display_PERS(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("PERS"));
}

void Display_PLUS(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("+"));
}

void Display_PWR(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("PWR"));
}

void Display_RATE(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("RATE"));
}

void Display_SCPE(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("SCPE"));
}

void Display_SETG(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("SETG"));
}

void Display_SFGN(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("SFGN"));
}

void Display_SFPI(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("SFPI"));
}

void Display_SPAN(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("SPAN"));
}

void Display_SPAR(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("SPAR"));
}
void Display_TOG(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("TOG"));
}

void Display_UTIL(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("UTIL"));
}

void Display_YES(uint8_t x1,  uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
  tft.println(F("YES"));
}

void Display_NOYES() {
  tft.setCursor(14,0);
  tft.setTextColor(DARKRED);
  tft.println(F("NO"));
  tft.setCursor(108,0);
  tft.setTextColor(DARKGREEN);
  tft.println(F("YES"));
}

void Display_LIMIT_PLUSMINUS (int8_t value, int8_t lowlim, int8_t highlim) {
  if (ACB[value] > lowlim) Display_MINUS(35, MINUS_Color);
  if (ACB[value] < highlim) Display_PLUS(108, PLUS_Color);
}

void PARAM_DEC(uint8_t x, int8_t value, int8_t limit, uint16_t color) {
  Flag = 1;

  while((digitalRead(Function_Select0) == LOW) || (Flag == 1)) {
    Flag = 0;

    if(ACB[value] > limit) {
      ACB[value]--;
    }
    else {
      ACB[value] = limit;
    }

    if (ACB[value] > limit) {
      Display_MINUS(35, MINUS_Color);
    }

    else {
      Display_MINUS(35, BG_Color);
    }
  Display_Param_Value (x, value, color);
  Display_PLUS(108, PLUS_Color);
  }
  tft.fillRect(54, 0, 52, 15, BG_Color);
}

void PARAM_INC(uint8_t x, int8_t value, int8_t limit, uint16_t color) {
  Flag = 1;

  while((digitalRead(Function_Select2) == LOW) || (Flag == 1)) {
    Flag = 0;

    if(ACB[value] < limit) {
      ACB[value]++;
   }

    else {
      ACB[value] = limit;
    }
   
    if (ACB[value] < limit) {
      Display_PLUS(108, PLUS_Color);
    }

    else {
      Display_PLUS(108, BG_Color);
    }
    Display_Param_Value (x, value, color);
    Display_MINUS(35, MINUS_Color);
  }
  tft.fillRect(54, 0, 52, 15, BG_Color);
}

void Display_Param_Value (uint8_t x, int8_t value, uint16_t color) {   
  tft.fillRect(54, 0, 52, 15, BG_Color);
  TBI_Display(x, color);
  tft.println(ACB[value]);
  delay_Millis (250);
  tft.fillRect(54, 0, 52, 15, BG_Color);
}

void TBI_Display(uint8_t x1, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x1,0);
}

void Dump_Data() {
  Serial.println();
  Serial.println(F("P and S Trace Buffer Dump"));
  Serial.println();

  tft.fillRect(236, 6, 2, 4, STATUS_Color);
      
  for(j=0; j<240; j++) {  // Format is trace buffer address following by ASCII integers for P and S on each line.
    Serial.print(j);
    Serial.print(F(" "));
    Serial.print(PTrace[j]);
    Serial.print(F(" "));
    Serial.println(STrace[j]);
  }

  Serial.println();
  Serial.println(F("Dump Complete"));
  Serial.println();

  delay_Millis (250); 
  tft.fillRect(236, 6, 2, 4, BG_Color);
}

// Load ACB from EEPROM
void LOAD_PARAMS() {
  EEPROM.get(0, i); // Reads valid byte in saved CB

  if(i == EEPROMFormatValid) { // Indicates valid data in EEPROM
    for(i = 0; i < 16; i++) { // Load ACB fron EEPROM
      EEPROM.get(i, ACB[i]);
    }

    UCBValid = 1;
    ACB_Startup ();
  }
}

// Save ACB to EEPROM
void SAVE_PARAMS() {
  ACB[0] = EEPROMFormatValid;
  ACB[STARTUP_FUNCTION] = Function & 7;
  ACB[STARTUP_STATE] = Startup_State[Function & 7];

  for(i = 0; i < 16; i++) {
    EEPROM.update(i, ACB[i]);
  }

  UCBValid = 1;
}

// Load ACB from DCB and save to EEPROM
void LOAD_DEFAULTS() {
  for(i = 0; i < 16; i++) {   // ACB loaded from Defaults and saved to EEPROM
    ACB[i] = DCB[i];
  }

  ACB_Startup ();
}

void ACB_Startup () {
  Function_Changed = 1;
  Buttons_Function = ACB[STARTUP_FUNCTION];
  Function = (Function & ~7) | ACB[STARTUP_FUNCTION];
  currentState = ACB[STARTUP_STATE];
}
    
ISR(PCINT2_vect) {
  // If interrupts come faster than ~160 ms, assume it's a bounce and ignore
  if(((Time_Stamp - last_Time_Stamp) > 1600) && (Buttons_Used_Flag == 1)) {  // abs to handle overflow after ~18 hours
    Time_Stamp = 0;
    last_Time_Stamp = 0;

    Interrupt_Flag = 1;
    pressedButton = 0;

    if(Header_Displayed_Flag == 0) {
      if(digitalRead(Function_Select0) == LOW) { pressedButton = 1; }
      else if(digitalRead(Function_Select1) == LOW) { pressedButton = 2; }
      else if(digitalRead(Function_Select2) == LOW) { pressedButton = 3; }

      Buttons_Used_Flag = 0;
    }

    else {
      Buttons_Used_Flag = 0;
      Skip_Flag = 1;
    }
  }
}
