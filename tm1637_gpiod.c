/*
 * TM1637 driver using libgpiod
 *
 * Open-drain emulation: switching between input (pull-up -> HIGH) and output LOW
 *
 * Hardware:
 * TM1637 VCC -> 3.3V
 * TM1637 GND -> GND
 * TM1637 CLK -> GPIO 23 + 4.7k pull-up to 3.3V
 * TM1637 DIO -> GPIO 24 + 4.7k pull-up to 3.3V
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <gpiod.h>
#include "tm1637_gpiod.h"

#define ADR 0xC0
#define DATA_COMMAND 0x40
#define DISPLAY_COMMAND 0x88

#define ACK  1
#define NACK 0

#define DELAY_US 50

static const uint8_t digits[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

static struct gpiod_chip *chip;
static struct gpiod_line *dio;
static struct gpiod_line *clk;

static uint8_t buf[5];

/*
 * Open-drain: release = input mode, external pull-up pulls HIGH
 */
static inline void dio_hi(void) {
    gpiod_line_release(dio);
    gpiod_line_request_input(dio, "tm1637");
}

static inline void dio_lo(void) {
    gpiod_line_release(dio);
    gpiod_line_request_output(dio, "tm1637", 0);
}

static inline void clk_hi(void) {
    gpiod_line_release(clk);
    gpiod_line_request_input(clk, "tm1637");
}

static inline void clk_lo(void) {
    gpiod_line_release(clk);
    gpiod_line_request_output(clk, "tm1637", 0);
}

static void TM1637_start(void) {
    dio_hi();
    clk_hi();
    usleep(DELAY_US);
    dio_lo();
    usleep(DELAY_US);
    clk_lo();
}

static void TM1637_stop(void) {
    dio_lo();
    clk_hi();
    usleep(DELAY_US);
    dio_hi();
    usleep(DELAY_US);
}

static uint8_t TM1637_write_byte(uint8_t b) {
    for (uint8_t i = 0; i < 8; i++) {
        (b & 1) ? dio_hi() : dio_lo();
        usleep(DELAY_US);
        clk_hi();
        usleep(DELAY_US);
        clk_lo();
        b >>= 1;
    }

    dio_hi();
    usleep(DELAY_US);
    clk_hi();
    usleep(DELAY_US);
    uint8_t ack = !gpiod_line_get_value(dio);
    clk_lo();

    return ack;
}

static void TM1637_cmd(uint8_t cmd) {
    TM1637_start();
    TM1637_write_byte(cmd);
    TM1637_stop();
}

static uint8_t TM1637_send(uint8_t *data, uint8_t len) {
    TM1637_cmd(DATA_COMMAND);

    TM1637_start();
    for (uint8_t i = 0; i < len; i++) {
        if (!TM1637_write_byte(data[i])) {
            TM1637_stop();
            return NACK;
        }
    }
    TM1637_stop();

    TM1637_cmd(DISPLAY_COMMAND);
    return ACK;
}

int TM1637_init(void) {
    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        fprintf(stderr, "Error: Cannot open /dev/gpiochip0\n");
        return -1;
    }

    dio = gpiod_chip_get_line(chip, DIO_PIN);
    clk = gpiod_chip_get_line(chip, CLK_PIN);
    if (!dio || !clk) {
        fprintf(stderr, "Error: Cannot get GPIO lines %d, %d\n", DIO_PIN, CLK_PIN);
        gpiod_chip_close(chip);
        return -1;
    }

    if (gpiod_line_request_input(dio, "tm1637") < 0 ||
        gpiod_line_request_input(clk, "tm1637") < 0) {
        fprintf(stderr, "Error: Cannot request GPIO lines\n");
        gpiod_chip_close(chip);
        return -1;
    }

    usleep(1000);

    if (gpiod_line_get_value(dio) != 1 || gpiod_line_get_value(clk) != 1) {
        fprintf(stderr, "Error: Missing pull-up resistors on GPIO %d, %d\n", DIO_PIN, CLK_PIN);
        gpiod_chip_close(chip);
        return -1;
    }

    fprintf(stderr, "TM1637 initialized (DIO=%d, CLK=%d)\n", DIO_PIN, CLK_PIN);
    return 0;
}

void TM1637_cleanup(void) {
    if (dio) gpiod_line_release(dio);
    if (clk) gpiod_line_release(clk);
    if (chip) gpiod_chip_close(chip);
}

int TM1637_write_num(int16_t num) {
    buf[0] = ADR;

    if (num >= 0) {
        buf[1] = 0x00;
    } else {
        buf[1] = 0x40;
        num = -num;
    }

    uint16_t n = (uint16_t)num;
    uint8_t d2 = n % 10; n /= 10;
    uint8_t d1 = n % 10;
    uint8_t d0 = n / 10;

    if (d0 < 10) {
        buf[2] = digits[d0];
        buf[3] = digits[d1] | 0x80;  /* decimal point */
        buf[4] = digits[d2];
    } else {
        buf[2] = 0x3F;  /* O */
        buf[3] = 0x71;  /* F */
        buf[4] = 0x38;  /* L */
    }

    return (TM1637_send(buf, 5) == ACK) ? 0 : -1;
}

int TM1637_write_err(void) {
    buf[0] = ADR;
    buf[1] = 0x79;  /* E */
    buf[2] = 0x50;  /* r */
    buf[3] = 0x50;  /* r */

    return (TM1637_send(buf, 4) == ACK) ? 0 : -1;
}
