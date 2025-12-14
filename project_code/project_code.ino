//////////
// Author: Alex Hoffman
// Last Updated: 12/12/2025
// Disc: Final project program for Arduino Mega
// Class: CPE 301-1001
// Date Due: 12/13/2025
//////////

/////
// Allowed libraries
/////
#include <Stepper.h>
#include <LiquidCrystal.h>
#include <DHT.h>
#include <RTClib.h>

/////
// Global state & enums
/////
typedef enum {
    STATE_DISABLED,
    STATE_IDLE,
    STATE_RUNNING,
    STATE_ERROR
} CoolerState;

volatile CoolerState currentState = STATE_DISABLED;

// ISR flags
volatile bool startPressed = false;
volatile bool resetPressed = false;

/////
// GPIO for LEDs
/////
volatile char* DDRB = (char*)0x24;
volatile char* PORTB = (char*)0x25;

volatile char* DDRH = (char*)0x101;
volatile char* PORTH = (char*)0x102;

#define LED_GREEN   4 // PH4
#define LED_YELLOW  5 // PH5
#define LED_RED     6 // PH6

void gpioInit() {
    *DDRH |= (1 << LED_GREEN) | (1 << LED_YELLOW) | (1 << LED_RED);
}

void allLedsOff() {
    *PORTH &= ~((1 << LED_GREEN) | (1 << LED_YELLOW) | (1 << LED_RED));
}

void setGreenLed() {
    allLedsOff(); *PORTH |= (1 << LED_GREEN);
}
void setYellowLed() {
    allLedsOff(); *PORTH |= (1 << LED_YELLOW);
}
void setRedLed() {
    allLedsOff(); *PORTH |= (1 << LED_RED);
}

/////
// Fan motor
/////
#define FAN_ENABLE_PIN  9 

#define FAN_SPEED   200

void fanInit() {
    
}

// Arduino setup fxn
void setup() {

}

// Arduino loop fxn
void loop() {
    
}