#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "get_temp.h"

/*
 * Fce cte teplotu ze std vystupu prikazu  "./r4dcb08 -f", prevede
 * ji na cele trojciferne cislo o rozmeru [0.1C]
 */
int16_t get_temp() {

  FILE *fp;
  char buffer[256];
  float temp;

  // Kontrola existence r4dcb08 binary
  if (access("./r4dcb08", X_OK) != 0) {
    fprintf(stderr, "Error: r4dcb08 binary not found or not executable\n");
    fprintf(stderr, "       Please ensure r4dcb08 is in the current directory with execute permissions\n");
    return TEMP_ERROR;
  }

  fp = popen("./r4dcb08 -f", "r");  // Ziskej teplotu
  if (fp == NULL) {
    perror("Error: popen failed!");
    return TEMP_ERROR;
  }
    
  // Přečte první řádek
  if (fgets(buffer, sizeof(buffer), fp) != NULL) {
    printf("%s\n",buffer);
    // Najde první float v řádku
    if (sscanf(buffer, "%f", &temp) == 1) {
      if (pclose(fp) != 0) {
        fprintf(stderr, "Error: r4dcb08 command failed\n");
        return TEMP_ERROR;
      }
      return (int16_t)(temp*10);
    } else {
      printf("Error: %s\n", buffer);
      pclose(fp);
      return TEMP_ERROR;
    }
  } else {
    fprintf(stderr, "Error: No data received from r4dcb08\n");
    pclose(fp);
    return TEMP_ERROR;
  }
}
