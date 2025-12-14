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
#define FAN_ENABLE_PIN  10 // OC2A - PB4
#define FAN_INT1_BIT    6 // PB6
#define FAN_INT2_BIT    7 // PB7

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

#define WATER_LOW_THRESHOLD 200

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
#define TBE 0x20

volatile unsigned char* my_ucsr0a = (unsigned char*)0x00C0;
volatile unsigned char* my_ucsr0b = (unsigned char*)0x00C1;
volatile unsigned char* my_ucsr0c = (unsigned char*)0x00C2;
volatile unsigned int* my_ubrr0 = (unsigned int*)0x00C4;
volatile unsigned char* my_udr0 = (unsigned char*)0x00C6;

void U0init(unsigned long baud) {
    unsigned int tbaud = (16000000/16/baud) - 1;
    *my_ucsr0a = 0x20;
    *my_ucsr0b = 0x18;
    *my_ucsr0c = 0x06;
    *my_ubrr0 = tbaud;
}

void U0putchar(unsigned char c) {
    while(!(*my_ucsr0a & TBE));
    *my_udr0 = c;
}

void uartPrint(const char* s) {
    while(*s) U0putchar(*s++);
}

/////
// RTC
/////
RTC_DS1307 rtc;

void rtcInit() {
    rtc.begin();
    if(!rtc.isrunning()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

void logEvent(const char* msg) {
    DateTime now = rtc.now();
    char buf[64];
    snprintf(buf, sizeof(buf),
        "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
        now.year(), now.month(), now.day(),
        now.hour(), now.minute(), now.second(), msg);
    uartPrint(buf);
}

/////
// Stepper
/////
#define STEPS_PER_REV 2048
#define STEPPER_PIN_1 22
#define STEPPER_PIN_2 24
#define STEPPER_PIN_3 26
#define STEPPER_PIN_4 28

Stepper ventStepper(STEPS_PER_REV,
    STEPPER_PIN_1, STEPPER_PIN_3,
    STEPPER_PIN_2, STEPPER_PIN_4);

int currentVentStep = 0;

void stepperInit() {
    ventStepper.setSpeed(10);
}

/////
// DHT11 & LCD
/////
#define DHT_PIN  55
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

float currentTempC = 0;
float currentHumidity = 0;
unsigned long lastDHTRead = 0;

#define TEMP_HIGH_THRESHOLD 10.0
#define DHT_INTERVAL        60000UL

bool tempAboveThreshold() {
    return currentTempC > TEMP_HIGH_THRESHOLD;
}

void updateTempHumidity() {
    unsigned long now = millis();
    if(now - lastDHTRead < DHT_INTERVAL) return;

    lastDHTRead = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if(!isnan(t) && !isnan(h)) {
        currentTempC = t;
        currentHumidity = h;
    }
}

LiquidCrystal lcd(49, 48, 47, 46, 45, 44);

/////
// ISR fxns
/////
void startISR() {
    startPressed = true;
}
void resetISR() {
    resetPressed = true;
}

/////
// State machine logic
/////
void transitionTo(CoolerState next) {
    if(currentState == next) return;
    fanOff();
    logEvent("STATE CHANGE");
    currentState = next;
}

void handleISRFlags() {
    if(startPressed && currentState == STATE_DISABLED) {
        startPressed = false;
        transitionTo(STATE_IDLE);
    }
    if(resetPressed && currentState == STATE_ERROR && !isWaterLow()) {
        resetPressed = false;
        transitionTo(STATE_IDLE);
    } 
}

/////
// setup() and loop()
/////
void setup() {
    gpioInit();
    volatile unsigned char* port_e = (unsigned char*)0x2E;
    *port_e |= (1 << 4) | (1 << 5);
    fanInit();
    adc_init();
    U0init(9600);
    uartPrint("UART OK\n");
    rtcInit();
    stepperInit();
    dht.begin();
    lcd.begin(16, 2);
    lcd.setCursor(0,0);
    lcd.print("LCD OK");

    attachInterrupt(digitalPinToInterrupt(2), startISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(3), resetISR, FALLING);
}

void loop() {
    updateTempHumidity();
    handleISRFlags();

    switch (currentState) {
        case STATE_DISABLED:
            setYellowLed();
            fanOff();
            break;
        
        case STATE_IDLE:
            setGreenLed();
            fanOff();
            if(isWaterLow()) transitionTo(STATE_ERROR);
            else if(tempAboveThreshold()) transitionTo(STATE_RUNNING);
            break;
            
        case STATE_RUNNING:
            fanOn();
            if(isWaterLow()) transitionTo(STATE_ERROR);
            else if(!tempAboveThreshold()) transitionTo(STATE_IDLE);
            break;
        
        case STATE_ERROR:
            setRedLed();
            fanOff();
            break;
    }
}