/*
 * TM1637 driver pro Raspberry Pi 1 zalozeny na pigpio knihovne
 * Portovano z ATtiny13A implementace
 *
 * Implementace TM1637 protokolu s externimi pull-up rezistory
 * Open-drain komunikace s pigpio INPUT/OUTPUT prepinacim
 *
 * Hardware pripojeni:
 * TM1637 VCC -> RPi 3.3V
 * TM1637 GND -> RPi GND  
 * TM1637 CLK -> RPi GPIO 23 + 4.7k pull-up na 3.3V
 * TM1637 DIO -> RPi GPIO 24 + 4.7k pull-up na 3.3V
 *
 * Kompilace: gcc -o tm1637_test tm1637_rpi.c -lpigpio -lrt -lpthread
 * Spusteni:  sudo ./tm1637_test
 *
 * V1.0/280825 - Port z ATtiny implementace
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pigpio.h>
#include "tm1637_rpi_pigpio.h"

// TM1637 konstanty (zachovane z puvodniho kodu)
#define ADR 0xC0                // Adresa prvniho segmentu 
#define DATA_COMMAND 0x40       // Autoincrement
#define DISPLAY_COMMAND 0x88    // 0x80-display off, 0x88..0x8F jas od nejmensiho po nejvetsi
#define DISPLAY_OFF 0x80        // Display off

// Navratove hodnoty
#define ACK  1
#define NACK 0

// Kody cifer
static const uint8_t digits[] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,  // 0, 1, 2, 3, 4
    0x6D, 0x7D, 0x07, 0x7F, 0x6F   // 5, 6, 7, 8, 9
};

// Globalni promenne (zachovane z puvodniho kodu)
static uint8_t data_to_send[5];
static uint8_t digit[3];

// Makra pro ovladani pinu - open-drain emulace (pigpio verze)
#define DIO_RELEASE()   gpioSetMode(DIO_PIN, PI_INPUT)      // Uvolnit linku (high-Z)
#define DIO_PULL()      do { gpioSetMode(DIO_PIN, PI_OUTPUT); gpioWrite(DIO_PIN, 0); } while(0)  // Stahnout k zemi
#define CLK_RELEASE()   gpioSetMode(CLK_PIN, PI_INPUT)      // Uvolnit linku (high-Z)
#define CLK_PULL()      do { gpioSetMode(CLK_PIN, PI_OUTPUT); gpioWrite(CLK_PIN, 0); } while(0)  // Stahnout k zemi
#define DIO_READ()      gpioRead(DIO_PIN)                   // Precist stav DIO

/*
 * Validace GPIO pinu - kontrola pull-up rezistoru
 */
static int TM1637_validate_gpio_pins() {
    // Nastavit piny jako INPUT (high-Z stav)
    if (gpioSetMode(DIO_PIN, PI_INPUT) != 0 || gpioSetMode(CLK_PIN, PI_INPUT) != 0) {
        fprintf(stderr, "Chyba: Nelze pristupovat k GPIO pinum %d,%d\n", DIO_PIN, CLK_PIN);
        return -1;
    }
    
    // Kratka pauza pro stabilizaci
    gpioDelay(1000); // 1ms
    
    // S pull-up rezistory musi byt oba piny HIGH
    int dio_state = gpioRead(DIO_PIN);
    int clk_state = gpioRead(CLK_PIN);
    
    if (dio_state != 1 || clk_state != 1) {
        fprintf(stderr, "Chyba: Chybi pull-up rezistory\n");
        fprintf(stderr, "GPIO %d (DIO): %s, GPIO %d (CLK): %s\n", 
                DIO_PIN, (dio_state == 1) ? "HIGH" : "LOW",
                CLK_PIN, (clk_state == 1) ? "HIGH" : "LOW");
        fprintf(stderr, "Zkontrolujte zapojeni 4.7kΩ pull-up rezistoru na 3.3V\n");
        return -1;
    }
    
    return 0;
}

/*
 * Inicializace TM1637 komunikace
 * Nastavi piny jako INPUT (high-Z stav), pull-up rezistory je vytahnou na HIGH
 */
int TM1637_init() {
    // Inicializace pigpio knihovny
    if (gpioInitialise() < 0) {  // Chyba pri inicializaci knihovny
        fprintf(stderr, "Chyba: Nelze inicializovat pigpio knihovnu\n");
        return -1;
    }
    
    // Validace GPIO pinu a pull-up rezistoru
    if (TM1637_validate_gpio_pins() < 0) {
        gpioTerminate();
        return -1;
    }
    
    fprintf(stderr,"TM1637 piny validovany a inicializovany (GPIO %d=DIO, GPIO %d=CLK)\n", DIO_PIN, CLK_PIN);

    return 0;
}

/*
 * Cleanup - ukonceni pigpio
 */
void TM1637_cleanup() {
    // Uvolnit piny
    DIO_RELEASE();
    CLK_RELEASE();
    
    // Ukoncit pigpio
    gpioTerminate();
}

/*
 * Casove zpozdeni pro TM1637 protokol
 * ~50us pro 10kHz rychlost (jako v puvodnim kodu)
 */
static void TM1637_delay() {
    gpioDelay(50);  // 50 mikrosekund
}

/*
 * START podminka pro TM1637 protokol
 * DIO prechod HIGH->LOW pri CLK HIGH
 */
static void TM1637_start() {
    DIO_RELEASE();  // DIO na HIGH (pull-up)
    CLK_RELEASE();  // CLK na HIGH (pull-up)
    TM1637_delay();
    DIO_PULL();     // DIO prejde na LOW pri CLK HIGH
    TM1637_delay();
    CLK_PULL();     // CLK jde na LOW
}

/*
 * STOP podminka pro TM1637 protokol  
 * DIO prechod LOW->HIGH pri CLK HIGH
 */
static void TM1637_stop() {
    DIO_PULL();     // DIO na LOW
    CLK_RELEASE();  // CLK na HIGH
    TM1637_delay();
    DIO_RELEASE();  // DIO jde na HIGH pri CLK HIGH
    TM1637_delay();
}

/*
 * Odeslani jednoho bajtu pres TM1637 protokol
 * Vraci ACK (1) nebo NACK (0)
 */
static uint8_t TM1637_write_byte(uint8_t byte) {
    // Poslat 8 bitu LSB first
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
        byte >>= 1;         // Posun na dalsi bit vpravo
    }
    
    // Cekat na ACK od slave
    DIO_RELEASE();          // Uvolnit DIO pro slave
    TM1637_delay();
    CLK_RELEASE();          // Clock pulse pro ACK
    TM1637_delay();
    
    uint8_t ack = !DIO_READ();  // 0 = ACK (DIO LOW), 1 = NACK (DIO HIGH)
    
    CLK_PULL();             // Clock zpet na LOW
    
    return ack;  // Vratit 1 pokud ACK, 0 pokud NACK
}

/*
 * Odeslani prikazu do TM1637
 */
static void TM1637_send_command(uint8_t cmd) {
    TM1637_start();
    TM1637_write_byte(cmd);
    TM1637_stop();
}

/*
 * Pomocna funkce pro komunikaci - odeslani dat
 */
static uint8_t TM1637_write_to(uint8_t *data, uint8_t length) {
    
    /* Data command */
    TM1637_send_command(DATA_COMMAND);
    
    /* Send data */
    TM1637_start();
    for(uint8_t i = 0; i < length; i++) {
        if(!TM1637_write_byte(data[i])) {
            TM1637_stop();
            printf("Chyba: NACK pri odesilani dat na pozici %d\n", i);
            return NACK;  // Chyba pri odeslani dat
        }
    }
    TM1637_stop();
    
    /* Display on */
    TM1637_send_command(DISPLAY_COMMAND);
    
    return ACK; // Uspech
}

/*
 * Zobrazeni 3 ciferneho cisla na displeji
 */
int TM1637_write_num(int16_t num) {
    
    data_to_send[0] = ADR;  // Adresa prvniho znaku
    
    // Znamenko (zachovane z puvodniho kodu)
    if (num >= 0) {
        data_to_send[1] = 0x00;  // Zadne znamenko
    }
    else {
        data_to_send[1] = 0x40;  // Minus
        num = -num;
    }
    
    uint16_t unum = (uint16_t)num;
    
    // Rozklad cisla na cifry (zachovane z puvodniho kodu)
    digit[2] = unum % 10;
    unum /= 10;
    digit[1] = unum % 10;
    digit[0] = unum / 10;
    
    if (digit[0] < 10) {
        // Normalni zobrazeni s desetinnou teckou
        for (uint8_t j = 0; j < 3; j++) {
            data_to_send[j + 2] = digits[digit[j]];
        }
        data_to_send[3] |= 0x80; // Pridat desetinnou tecku
    } else {  
        // Overflow - zobrazit "OFL"
        data_to_send[2] = 0x3F; // O
        data_to_send[3] = 0x71; // F  
        data_to_send[4] = 0x38; // L
    }
    
    // Odeslat data na displej
    if (TM1637_write_to(data_to_send, 5) == NACK) {
        fprintf(stderr,"Chyba pri zobrazovani cisla %d\n", num);
        return -1;
    }
    return 0;
}
/*
 *  Zobrazi Err na displeji
 */
int TM1637_write_err() {
  // Zobrazit Err
  data_to_send[0] = ADR;  // Adresa prvniho znaku
  data_to_send[1] = 0x79; // E
  data_to_send[2] = 0x60; // r
  data_to_send[3] = 0x60; // r

  // Odeslat data na displej
  if (TM1637_write_to(data_to_send, 4) == NACK) {
    fprintf(stderr,"Chyba pri zobrazeni Err\n");
    return -1;
  }
  return 0;
}

