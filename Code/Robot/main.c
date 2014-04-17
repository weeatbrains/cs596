#include "globals.h"
#include "LCD.h"
#include "motors.h"
#include "serial.h"
#include "utility.h"

// AVR-GCC library reference: http://www.nongnu.org/avr-libc/user-manual/modules.html

// Define the digital pin to use for scrolling.
#define SWITCH_SCROLL 9

// Local prototypes
static void mainMenu();
static void testMotors();

//! Initializes XiphosLibrary, pullups, and timers, prints version.
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

  while (1)
  {
    mainMenu();
  }
}

//! Main Menu options.
enum {
  Option0,
  Option1,
  Option2,
  NUM_Options
};

/*! Displays the main menu and runs the option that the user selects with the
 *  scroll switches.
 */
static void mainMenu()
{
  u08 choice = 0;
  u08 prevChoice = 255;

  clearScreen();
  printString_P(PSTR("Main Menu"));

  // Loop until user makes a selection.
  do
  {
    if (digitalInput(SWITCH_SCROLL) == 0)
    {
      if (++choice >= NUM_Options)
      {
        choice = 0;
      }
      // Wait for switch to be released.
      while (digitalInput(SWITCH_SCROLL) == 0)
        ;
    }

    // Redraw menu only when choice has changed.
    if (choice != prevChoice)
    {
      prevChoice = choice;

      lowerLine();
      switch (choice)
      {
        case Option0:
          printString_P(PSTR("1 Motor Test"));
          break;
        case Option1:
          printString_P(PSTR("2 Unused    "));
          break;
        case Option2:
          printString_P(PSTR("3 Unused    "));
          break;
        default:
          printString_P(PSTR("invalid choice"));
          break;
      }
    }
  } while (getButton1() == 0);
  // Debounce button.
  buttonWait();

  clearScreen();

  // Run the chosen mode.
  switch (choice)
  {
    case Option0:
      testMotors();
      break;
    case Option1:
      // TODO
      break;
    case Option2:
      // TODO
      break;
    default:
      break;
  }
}

static void stop()
{
  motor0(127);
  motor1(127);
}

static void testMotors()
{
  clearScreen();
  printString_P(PSTR("Test Motors"));

  lowerLine();
  printString_P(PSTR("Forward"));
  motor0(255);
  motor1(255);
  delayMs(3000);
  stop();
  delayMs(1000);

  lowerLine();
  printString_P(PSTR("Turn Right"));
  motor0(255);
  motor1(0);
  delayMs(1000);
  stop();
  delayMs(200);

  lowerLine();
  printString_P(PSTR("Backward  "));
  motor0(0);
  motor1(0);
  delayMs(3000);
  stop();
  delayMs(1000);

  lowerLine();
  printString_P(PSTR("Turn Left "));
  motor0(0);
  motor1(255);
  delayMs(1000);
  stop();
  delayMs(1000);
}
