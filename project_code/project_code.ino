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
// GPIO registers for LEDs and fan
/////
volatile unsigned char* ddr_b = (unsigned char*)0x24;
volatile unsigned char* port_b = (unsigned char*)0x25;

volatile unsigned char* ddr_h = (unsigned char*)0x101;
volatile unsigned char* port_h = (unsigned char*)0x102;

/////
// LEDs
/////
#define LED_GREEN   4 // PH4
#define LED_YELLOW  5 // PH5
#define LED_RED     6 // PH6

void gpioInit() {
    *ddr_h |= (1 << LED_GREEN) | (1 << LED_YELLOW) | (1 << LED_RED);
}

void allLedsOff() {
    *port_h &= ~((1 << LED_GREEN) | (1 << LED_YELLOW) | (1 << LED_RED));
}

void setGreenLed() {
    allLedsOff(); *port_h |= (1 << LED_GREEN);
}
void setYellowLed() {
    allLedsOff(); *port_h |= (1 << LED_YELLOW);
}
void setRedLed() {
    allLedsOff(); *port_h |= (1 << LED_RED);
}

/////
// Fan motor
/////
#define FAN_ENABLE_PIN  9 
#define FAN_INT1_BIT    6
#define FAN_INT2_BIT    7

#define FAN_SPEED   200

void fanInit() {
    // Set PB6 and PB7 as outputs
    *ddr_b |= (1 << FAN_INT1_BIT) | (1 << FAN_INT2_BIT);

    // Sets direction
    *port_b |= (1 << FAN_INT1_BIT);
    *port_b &= ~(1 << FAN_INT2_BIT);

    analogWrite(FAN_ENABLE_PIN, 0);
}

void fanOn() {
    analogWrite(FAN_ENABLE_PIN, FAN_SPEED);
}
void fanOff() {
    analogWrite(FAN_ENABLE_PIN, 0);
}

/////
// ADC - Water sensor
/////
volatile unsigned char* my_admux = (unsigned char*)0x7C;
volatile unsigned char* my_adcsrb = (unsigned char*)0x7B;
volatile unsigned char* my_adcsra = (unsigned char*)0x7A;
volatile unsigned char* my_adcl = (unsigned char*)0x78;
volatile unsigned char* my_adch = (unsigned char*)0x79;

#define WATER_LOW_THRESHOLD 315

void adc_init() {
    *my_adcsra |= 0x80;
    *my_adcsra &= 0xDF;
    *my_adcsra &= 0xF7;
    *my_adcsra &= 0xF8;
    *my_adcsrb &= 0xF7;
    *my_adcsrb &= 0xF8;
    *my_admux &= 0x7F;
    *my_admux |= 0x40;
    *my_admux &= 0xDF;
    *my_admux &= 0xE0;
}

unsigned int readWaterLevelADC() {
    *my_admux &= 0xE0;
    *my_adcsrb &= 0xF7;
    *my_adcsra |= 0x40;
    while(*my_adcsra & 0x40);

    return (((unsigned int)*my_adch << 8) | *my_adcl) & 0x03FF;
}

bool isWaterLow() {
    return readWaterLevelADC() < WATER_LOW_THRESHOLD;
}

/////
// UART
/////


// Arduino setup fxn
void setup() {

}

// Arduino loop fxn
void loop() {
    
}