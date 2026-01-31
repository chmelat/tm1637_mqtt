/*
 * TM1637 driver for Raspberry Pi 1 based on pigpio library
 * Ported from ATtiny13A implementation
 *
 * TM1637 protocol implementation with external pull-up resistors
 * Open-drain communication with pigpio INPUT/OUTPUT switching
 *
 * Hardware connections:
 * TM1637 VCC -> RPi 3.3V
 * TM1637 GND -> RPi GND
 * TM1637 CLK -> RPi GPIO 23 + 4.7k pull-up to 3.3V
 * TM1637 DIO -> RPi GPIO 24 + 4.7k pull-up to 3.3V
 *
 * Compilation: gcc -o tm1637_test tm1637_rpi.c -lpigpio -lrt -lpthread
 * Run:  sudo ./tm1637_test
 *
 * V1.0/280825 - Port from ATtiny implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pigpio.h>
#include "tm1637_rpi_pigpio.h"

// TM1637 constants (preserved from original code)
#define ADR 0xC0                // Address of first segment
#define DATA_COMMAND 0x40       // Autoincrement
#define DISPLAY_COMMAND 0x88    // 0x80-display off, 0x88..0x8F brightness from lowest to highest
#define DISPLAY_OFF 0x80        // Display off

// Return values
#define ACK  1
#define NACK 0

// Digit codes
static const uint8_t digits[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,  // 0, 1, 2, 3, 4
    0x6D, 0x7D, 0x07, 0x7F, 0x6F   // 5, 6, 7, 8, 9
};

// Global variables (preserved from original code)
static uint8_t data_to_send[5];
static uint8_t digit[3];

// Macros for pin control - open-drain emulation (pigpio version)
#define DIO_RELEASE()   gpioSetMode(DIO_PIN, PI_INPUT)      // Release line (high-Z)
#define DIO_PULL()      do { gpioSetMode(DIO_PIN, PI_OUTPUT); gpioWrite(DIO_PIN, 0); } while(0)  // Pull to ground
#define CLK_RELEASE()   gpioSetMode(CLK_PIN, PI_INPUT)      // Release line (high-Z)
#define CLK_PULL()      do { gpioSetMode(CLK_PIN, PI_OUTPUT); gpioWrite(CLK_PIN, 0); } while(0)  // Pull to ground
#define DIO_READ()      gpioRead(DIO_PIN)                   // Read DIO state

/*
 * GPIO pin validation - check pull-up resistors
 */
static int TM1637_validate_gpio_pins() {
    // Set pins as INPUT (high-Z state)
    if (gpioSetMode(DIO_PIN, PI_INPUT) != 0 || gpioSetMode(CLK_PIN, PI_INPUT) != 0) {
        fprintf(stderr, "Error: Cannot access GPIO pins %d,%d\n", DIO_PIN, CLK_PIN);
        return -1;
    }

    // Short pause for stabilization
    gpioDelay(1000); // 1ms

    // With pull-up resistors both pins must be HIGH
    int dio_state = gpioRead(DIO_PIN);
    int clk_state = gpioRead(CLK_PIN);

    if (dio_state != 1 || clk_state != 1) {
        fprintf(stderr, "Error: Missing pull-up resistors\n");
        fprintf(stderr, "GPIO %d (DIO): %s, GPIO %d (CLK): %s\n",
                DIO_PIN, (dio_state == 1) ? "HIGH" : "LOW",
                CLK_PIN, (clk_state == 1) ? "HIGH" : "LOW");
        fprintf(stderr, "Check wiring of 4.7kΩ pull-up resistors to 3.3V\n");
        return -1;
    }

    return 0;
}

/*
 * TM1637 communication initialization
 * Sets pins as INPUT (high-Z state), pull-up resistors pull them to HIGH
 */
int TM1637_init() {
    // Configure pigpio sampling rate to reduce CPU usage
    // Default is 5us which causes ~23% CPU on RPi1
    // Maximum period is 10us (still ~12% on RPi1, but half the load)
    gpioCfgClock(10, PI_CLOCK_PCM, 0);

    // Initialize pigpio library
    if (gpioInitialise() < 0) {  // Error initializing library
        fprintf(stderr, "Error: Cannot initialize pigpio library\n");
        return -1;
    }

    // Validate GPIO pins and pull-up resistors
    if (TM1637_validate_gpio_pins() < 0) {
        gpioTerminate();
        return -1;
    }

    fprintf(stderr,"TM1637 pins validated and initialized (GPIO %d=DIO, GPIO %d=CLK)\n", DIO_PIN, CLK_PIN);

    return 0;
}

/*
 * Cleanup - terminate pigpio
 */
void TM1637_cleanup() {
    // Release pins
    DIO_RELEASE();
    CLK_RELEASE();

    // Terminate pigpio
    gpioTerminate();
}

/*
 * Time delay for TM1637 protocol
 * ~50us for 10kHz speed (as in original code)
 */
static void TM1637_delay() {
    gpioDelay(50);  // 50 microseconds
}

/*
 * START condition for TM1637 protocol
 * DIO transition HIGH->LOW while CLK HIGH
 */
static void TM1637_start() {
    DIO_RELEASE();  // DIO to HIGH (pull-up)
    CLK_RELEASE();  // CLK to HIGH (pull-up)
    TM1637_delay();
    DIO_PULL();     // DIO transitions to LOW while CLK HIGH
    TM1637_delay();
    CLK_PULL();     // CLK goes to LOW
}

/*
 * STOP condition for TM1637 protocol
 * DIO transition LOW->HIGH while CLK HIGH
 */
static void TM1637_stop() {
    DIO_PULL();     // DIO to LOW
    CLK_RELEASE();  // CLK to HIGH
    TM1637_delay();
    DIO_RELEASE();  // DIO goes to HIGH while CLK HIGH
    TM1637_delay();
}

/*
 * Send one byte via TM1637 protocol
 * Returns ACK (1) or NACK (0)
 */
static uint8_t TM1637_write_byte(uint8_t byte) {
    // Send 8 bits LSB first
    for(uint8_t i = 0; i < 8; i++) {
        if(byte & 0x01) {
            DIO_RELEASE();  // Bit 1 (HIGH)
        } else {
            DIO_PULL();     // Bit 0 (LOW)
        }
        TM1637_delay();
        CLK_RELEASE();      // Clock pulse HIGH
        TM1637_delay();
        CLK_PULL();         // Clock pulse LOW
        byte >>= 1;         // Shift to next bit right
    }

    // Wait for ACK from slave
    DIO_RELEASE();          // Release DIO for slave
    TM1637_delay();
    CLK_RELEASE();          // Clock pulse for ACK
    TM1637_delay();

    uint8_t ack = !DIO_READ();  // 0 = ACK (DIO LOW), 1 = NACK (DIO HIGH)

    CLK_PULL();             // Clock back to LOW

    return ack;  // Return 1 if ACK, 0 if NACK
}

/*
 * Send command to TM1637
 */
static void TM1637_send_command(uint8_t cmd) {
    TM1637_start();
    TM1637_write_byte(cmd);
    TM1637_stop();
}

/*
 * Helper function for communication - send data
 */
static uint8_t TM1637_write_to(uint8_t *data, uint8_t length) {

    /* Data command */
    TM1637_send_command(DATA_COMMAND);

    /* Send data */
    TM1637_start();
    for(uint8_t i = 0; i < length; i++) {
        if(!TM1637_write_byte(data[i])) {
            TM1637_stop();
            printf("Error: NACK when sending data at position %d\n", i);
            return NACK;  // Error sending data
        }
    }
    TM1637_stop();

    /* Display on */
    TM1637_send_command(DISPLAY_COMMAND);

    return ACK; // Success
}

/*
 * Display 3-digit number on display
 */
int TM1637_write_num(int16_t num) {

    data_to_send[0] = ADR;  // Address of first character

    // Sign (preserved from original code)
    if (num >= 0) {
        data_to_send[1] = 0x00;  // No sign
    }
    else {
        data_to_send[1] = 0x40;  // Minus
        num = -num;
    }

    uint16_t unum = (uint16_t)num;

    // Decompose number into digits (preserved from original code)
    digit[2] = unum % 10;
    unum /= 10;
    digit[1] = unum % 10;
    digit[0] = unum / 10;

    if (digit[0] < 10) {
        // Normal display with decimal point
        for (uint8_t j = 0; j < 3; j++) {
            data_to_send[j + 2] = digits[digit[j]];
        }
        data_to_send[3] |= 0x80; // Add decimal point
    } else {
        // Overflow - display "OFL"
        data_to_send[2] = 0x3F; // O
        data_to_send[3] = 0x71; // F
        data_to_send[4] = 0x38; // L
    }

    // Send data to display
    if (TM1637_write_to(data_to_send, 5) == NACK) {
        fprintf(stderr,"Error displaying number %d\n", num);
        return -1;
    }
    return 0;
}
/*
 *  Display Err on display
 */
int TM1637_write_err() {
  // Display Err
  data_to_send[0] = ADR;  // Address of first character
  data_to_send[1] = 0x79; // E
  data_to_send[2] = 0x60; // r
  data_to_send[3] = 0x60; // r

  // Send data to display
  if (TM1637_write_to(data_to_send, 4) == NACK) {
    fprintf(stderr,"Error displaying Err\n");
    return -1;
  }
  return 0;
}

