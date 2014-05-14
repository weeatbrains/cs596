
/*  Includes    */
#include <stdlib.h>
#include <stdio.h>
#include "ThinkGearStreamParser.h"
#include "ffft.h"
#include "globals.h"
#include "LCD.h"
#include "motors.h"
#include "serial.h"
#include "utility.h"



/*
    variables for capture_wave defined externally libraries
 */
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "suart.h"

//added
#include <util/delay.h>
#include <avr/interrupt.h>

#define	SYSCLK		16000000

// AVR-GCC library reference: http://www.nongnu.org/avr-libc/user-manual/modules.html


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


// Define the digital pin to use for scrolling.
#define SWITCH_SCROLL 9

const u08 SYNC = 0xAA;

void capture_wave (int16_t *buffer, uint16_t count);

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

/*
 main.c: In function 'main':
 main.c:114:9: error: passing argument 1 of 'capture_wave' discards 'volatile' qualifier from pointer target type [-Werror]
 
 
 so taking out volatile qualifier
 */
/*volatile*/ int rawdata[2048];
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
		//test02();
        
        //fft
        capture_wave (rawdata, 2048);
        
        //index whcih sets data into raw data
        //continuously run fft
        rd = 0;
        dofft = FALSE;
		
	}
	else{
		clearScreen();
		upperLine();
		printInt(attention );
		lowerLine();
		printInt(meditation);
		//delayMs(500);
	}
    
    //motor control
    if(attention>60) {motor0(157);motor1(97);}
    else if(meditation>80) {motor0(180);motor1(127);}
    else {motor0(127);motor1(127);}
    delayMs(500);
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
					rawdata[rd]= value[0];//(value[0]<<8) | value[1];
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


void capture_wave (int16_t *buffer, uint16_t count)
{
	ADMUX = _BV(REFS0)|_BV(ADLAR)|_BV(MUX2)|_BV(MUX1)|_BV(MUX0);	// channel
    
	do {
		ADCSRA = _BV(ADEN)|_BV(ADSC)|_BV(ADFR)|_BV(ADIF)|_BV(ADPS2)|_BV(ADPS1);
		while(bit_is_clear(ADCSRA, ADIF));
		*buffer++ = ADC - 32768;
	} while(--count);
    
	ADCSRA = 0;
}

//debugging
//void test02 ( void )
//
//{
//
//}


