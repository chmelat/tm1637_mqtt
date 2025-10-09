#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>

#include "get_temp.h"
#include "tm1637_rpi_pigpio.h"

#define DEFAULT_INTERVAL 60

static volatile bool running = true;

void sigint_handler(int sig) {
    (void)sig;
    running = false;
}

/*
 * Print usage information
 */
void print_usage(const char *progname) {
    printf("Pouziti: %s [-i interval]\n", progname);
    printf("  -i interval  Interval mereni v sekundach (vychozi: %d)\n", DEFAULT_INTERVAL);
    printf("  -h          Zobrazit tuto napovedu\n");
}

/*
 * Main
 */
int main(int argc, char *argv[]) {

    int16_t temp; /* Teplota [0.1C] */
    int interval = DEFAULT_INTERVAL;
    int opt;

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "i:h")) != -1) {
        switch (opt) {
            case 'i':
                interval = atoi(optarg);
                if (interval <= 0) {
                    fprintf(stderr, "Chyba: Interval musi byt kladne cislo\n");
                    return 1;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case '?':
                print_usage(argv[0]);
                return 1;
        }
    }

    printf("Zobrazeni teploty na dispeji TM1637 na Raspberry Pi 1 (pigpio)\n");
    printf("==============================================================\n");
    printf("Interval mereni: %d sekund\n", interval);
    printf("Pro ukonceni stiskni Ctrl+C\n\n");

    // Nastavit SIGINT handler
    signal(SIGINT, sigint_handler);

    // Inicializace
    if (TM1637_init() < 0) {
        printf("Chyba pri inicializaci knihovny pigpio!\n");
        return -1;
    }


/* Merici smycka */
    while(running) {
      temp = get_temp();
      if (temp != TEMP_ERROR) {
        TM1637_write_num(temp);
      } else {
        TM1637_write_err();
      }

      // Prerusitelne cekani
      for(int i = 0; i < interval && running; i++) {
        sleep(1);
      }
    }

    printf("\nUkoncovani programu...\n");
    TM1637_cleanup();
    return 0;
}
