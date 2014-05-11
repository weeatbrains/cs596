#include "globals.h"
#include "LCD.h"
#include "motors.h"
#include "serial.h"
#include "utility.h"
#include <stdlib.h>
#include <stdio.h>
#include "ThinkGearStreamParser.h"






// AVR-GCC library reference: http://www.nongnu.org/avr-libc/user-manual/modules.html

// Define the digital pin to use for scrolling.
#define SWITCH_SCROLL 9
/* Parser types */
#define PARSER_TYPE_NULL       0x00
#define PARSER_TYPE_PACKETS    0x01 /* Stream bytes as ThinkGear Packets */
#define PARSER_TYPE_2BYTERAW   0x02 /* Stream bytes as 2-byte raw data */

/* Data CODE definitions */
#define PARSER_BATTERY_CODE         0x01
#define PARSER_POOR_SIGNAL_CODE     0x02
#define PARSER_ATTENTION_CODE       0x04
#define PARSER_MEDITATION_CODE      0x05
#define PARSER_RAW_CODE             0x80
const u08 SYNC = 0xAA;

const float potMin = 14.0 - 1; //NEED TO ADJUST FOR OUR ROBOT
const float potMax = 236.0 - 5; // NEED TO ADJUST FOR OUR ROBOT

float tr = 1.0;
volatile int attention = 0;
volatile int meditation = 0;
float speed = 0.0;
volatile int state =0;
volatile unsigned char plength = 170;
volatile int pcount=0;
volatile unsigned char payload[256];
volatile int checksum;
volatile double rawdata[2048];
volatile int rd =0;
volatile bool dofft = FALSE;
ThinkGearStreamParser parser;


// Local prototypes

static void connectHeadset();
void test02 ( void );
void handleDataValueFunc( unsigned char extendedCodeLevel,
unsigned char code,
unsigned char valueLength,
const unsigned char *value,
void *customData );




//! Initializes XiphosLibrary, pullups, and timers, prints version.

ISR(USART1_RX_vect)
{
	
	u08 data = UDR1;
	THINKGEAR_parseByte( &parser, data);


}


int main()
{
	
  // Initialize XiphosLibrary.
  initialize();

  // Initialize UARTs.
  uart0Init();
  uart1Init();
	
  // Enable interrupts.
  sei();
  // Configure scroll wheel digital pin as an input, with pullup resistor enabled.
  digitalDirection(SWITCH_SCROLL, Direction_INPUT_PULLUP);

  // Enable pullup resistors for all 8 analog inputs.
  analogPullups(0xFF);


 THINKGEAR_initParser( &parser, PARSER_TYPE_PACKETS,
 handleDataValueFunc, NULL );
connectHeadset();

while(1)
{
	if(dofft){
		clearScreen();
		upperLine();
		printString_P(PSTR("Doing FFT"));
		lowerLine();
		printString_P(PSTR("Please Hold..."));
		test02();
		
	}
	else{
		clearScreen();
		upperLine();
		printInt(attention );
		lowerLine();
		printInt(meditation);
		delayMs(500);
	}
}

}



static void connectHeadset()
{
	delayMs(3000);
	uart1Transmit(0xc2); //Signal to dongle to connect
	attention = 0;
}


void handleDataValueFunc( unsigned char extendedCodeLevel,
unsigned char code,
unsigned char valueLength,
const unsigned char *value,
void *customData ) {

	int i ;
	if(extendedCodeLevel== 0 ) {
		
		switch( code ) {
			
			/* [CODE]: ATTENTION eSense */
			case( 0x04 ):
				attention= value[0];	
			break;
			
			/* [CODE]: MEDITATION eSense */
			case( 0x05 ):
				meditation = value[0];

			break;

			case( 0x80 ):
				if (rd<2048){
					rawdata[rd]= (value[0]<<8) | value[1];
					rd++;
				}
				else
				{
					dofft = TRUE;
				}
				

			break;
			
			/* Other [CODE]s */
			default:
			//clearScreen();
			//printString_P(PSTR("No Data"));
			i=1;
			i=i;
			//lowerLine();
			//printString_P(PSTR("Boohoo"));
		}
}
	else 
	
{
	clearScreen();
	printHexDigit(extendedCodeLevel );
	delayMs(2000);

	
	}
}

void test02 ( void )

{

}


