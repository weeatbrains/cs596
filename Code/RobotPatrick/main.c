// AVR-GCC library reference: http://www.nongnu.org/avr-libc/user-manual/modules.html

// NOTE: Must be defined before including "FFT.h".
//! The number of samples per FFT.
#define FFT_N 128
// Enable linear output magnitude.
#define LIN_OUT 1

// Includes
// ArduinoFFT library from: http://wiki.openmusiclabs.com/wiki/ArduinoFFT
//#include "FFT.h"
#include "globals.h"
#include "LCD.h"
#include "motors.h"
#include "serial.h"
#include "utility.h"
#include "ThinkGearStreamParser.h"
#include <util/atomic.h>
#include <stdio.h>

#define DRIVE_POWER 30
#define SPIN_POWER 50

volatile u08 batteryLevel;
volatile u08 poorSignal;
volatile u08 attention;
volatile u08 meditation;
volatile u16 samples;

//! Circular buffer containing the latest EEG data received.
volatile s16 rawData[FFT_N];

//! Index of circular buffer's head element.
volatile u16 rawHead = 0;

ThinkGearStreamParser parser;

// Local prototypes.
static void connectHeadset();
void runFFT();
static void spin(u08 speed);
static void drive(u08 speed);
static void stop();
static void uart0TransmitString(const char * const string);
static void uart0TransmitPString(PGM_P const string);

void handleDataValueFunc(unsigned char extendedCodeLevel, unsigned char code,
    unsigned char valueLength, const unsigned char *value,
    void *customData);

//! Interrupt Service Routine that receives data on UART1 from the MindWave headset.
ISR(USART1_RX_vect)
{
  u08 data = UDR1;
  THINKGEAR_parseByte(&parser, data);
}

//! Initializes XiphosLibrary and runs main loop.
int main()
{
  // Initialize XiphosLibrary.
  initialize();

  // Initialize ThinkGear parser.
  THINKGEAR_initParser(&parser, PARSER_TYPE_PACKETS, handleDataValueFunc, NULL);

  // Initialize UARTs.
  uart0Init();
  uart1Init();

  // Enable interrupts.
  sei();

  connectHeadset();

  // Super loop.
  u16 cycles = 0;
  u16 samplesCopy;
  char str[10];
  
    
while (1)
  {
    // Print 4 individual values across the top line.
    upperLine();
    print_u08(attention);
    printChar(' ');
    print_u08(meditation);
    printChar(' ');
    print_u08(poorSignal);
    printChar(' ');
    print_u08(batteryLevel);

    // Disable interrupts momentarily and make a contiguous copy of the circular buffer.
    ATOMIC_BLOCK(ATOMIC_FORCEON)
        {
      u16 i = 0;
      u16 r;
      for (r = rawHead; r < FFT_N; r++)
      {
        // Put real data into even bins.
        fft_input[i++] = rawData[r]; //TODO (rawData[r] << 6)?
        // Set odd bins to 0.
        fft_input[i++] = 0;
      }
      for (r = 0; r < rawHead; r++)
      {
        // Put real data into even bins.
        fft_input[i++] = rawData[r]; //TODO (rawData[r] << 6)?
        // Set odd bins to 0.
        fft_input[i++] = 0;
      }

      // Also copy the samples variable while we have interrupts disabled.
      samplesCopy = samples;
    }

    // Run FFT on the contiguous copy.
    runFFT();

    // Send FFT input data to PC.
    uart0TransmitPString(PSTR("fft_input=[ "));
    for (u16 n = 0; n < FFT_N*2; n+=2)
    {
      s16 s = fft_input[n];
      sprintf(str, "%d ", s);
      uart0TransmitString(str);
    }
    uart0TransmitPString(PSTR("];\r\n"));

    // Send FFT spectrum output to PC.
    uart0TransmitPString(PSTR("fft_lin_out=[ "));
    for (u16 n = 0; n < FFT_N / 2; n++)
    {
      u16 s = fft_lin_out[n];
      sprintf(str, "%d ", s);
      uart0TransmitString(str);
    }
    uart0TransmitPString(PSTR("];\r\n"));

    // TODO Do something useful with the spectrum output fft_lin_out.

    // Control motors based on MindWave headset readings.
    if (attention > 60)
    {
      drive(DRIVE_POWER);
    }
    else if (meditation > 80)
    {
      spin(SPIN_POWER);
    }
    else
    {
      stop();
    }

    // Print the number of cycles run and samples collected on the lower line.
    lowerLine();
    print_u16(cycles++);
    printChar(' ');
    print_u16(samplesCopy);
  }
}

// This separate function was necessary to isolate the FFT assembly code.
void runFFT()
{
  fft_window(); // window the data for better frequency response
  fft_reorder(); // reorder the data before doing the fft
  fft_run(); // process the data in the fft
  fft_mag_lin(); // take the linear output of the fft
}

static void uart0TransmitString(const char * const string)
{
  u08 i = 0;
  //keep printing until we encounter a null terminator
  char ch;
  while ((ch = string[i]) != 0)
  {
    uart0Transmit(ch);
    i++;
  }
}

static void uart0TransmitPString(PGM_P const string)
{
  u08 i = 0;
  //keep printing until we encounter a null terminator
  char ch;
  while ((ch = pgm_read_byte_near(string + i)) != 0)
  {
    uart0Transmit(ch);
    i++;
  }
}

inline static void spin(u08 speed)
{
  motor0(127 + speed);
  motor1(127 + speed);
}

inline static void drive(u08 speed)
{
  motor0(127 + speed);
  motor1(127 - speed);
}

inline static void stop()
{
  motor0(127);
  motor1(127);
    stop(); //call recursively to make absolute stop
    
}

static void connectHeadset()
{
  // Reset saved data values.
  batteryLevel = 0;
  poorSignal = 0;
  attention = 0;
  meditation = 0;
  samples = 0;

  // Signal to dongle to connect.
  uart1Transmit(0xC2);
}

void handleDataValueFunc(unsigned char extendedCodeLevel, unsigned char code,
    unsigned char valueLength, const unsigned char *value,
    void *customData)
{
  // Only process non-extended codes (extended codes are undocumented).
  if (extendedCodeLevel == 0)
  {
    switch (code)
    {
      // 16-bit RAW wave value
      case PARSER_CODE_RAW_SIGNAL:
        // TODO: this if-statement block can be removed if it doesn't get triggered.
        if (valueLength != 2)
        {
          // Code expects valueLength for RAW to always be 2.
          // If headset can send multiple RAW values back-to-back,
          // we need to change our code, so print an alert and halt program.
          clearScreen();
          printString_P(PSTR("raw>2!: "));
          printPlain_u08(valueLength);
          // Halt the program.
          cli();
          while (1)
            ;
        }

        // Store this sample into the head element of the circular buffer.
        rawData[rawHead] = (value[0] << 8) | value[1];
        // Increment head to the next slot, wrapping around when needed.
        rawHead++;
        if (rawHead >= FFT_N)
        {
          rawHead = 0;
        }
        samples++;
        break;

        // BATTERY Level
      case PARSER_CODE_BATTERY:
        batteryLevel = value[0];
        break;

        // POOR_SIGNAL Quality
      case PARSER_CODE_POOR_QUALITY:
        poorSignal = value[0];
        break;

        // ATTENTION eSense
      case PARSER_CODE_ATTENTION:
        attention = value[0];
        break;

        // MEDITATION eSense
      case PARSER_CODE_MEDITATION:
        meditation = value[0];
        break;

        // Ignore any other received codes.
      default:
        break;
    }
  }
}
