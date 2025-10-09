/*  Declare of global functions */

// GPIO pin configuration (can be overridden at compile time)
#ifndef DIO_PIN
#define DIO_PIN  24
#endif

#ifndef CLK_PIN
#define CLK_PIN  23
#endif

extern int TM1637_init();
extern void TM1637_cleanup();
extern int TM1637_write_num(int16_t num);
extern int TM1637_write_err(); 
