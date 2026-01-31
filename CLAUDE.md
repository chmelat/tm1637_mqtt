# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

C application that displays temperature readings on a TM1637 4-digit 7-segment display connected to Raspberry Pi. Reads temperature from external sensor binary (`/usr/local/bin/r4dcb08`) and displays with 0.1°C resolution. Uses pigpio library for GPIO control.

**Compatibility:** RPi 1, 2, 3, 4 only. RPi5 is NOT supported (pigpio incompatibility).

## Build Commands

```bash
make                    # Compile
make debug              # Compile with debug symbols (-g -DDEBUG)
sudo make install       # Install to /usr/local/bin
sudo make install-service  # Install + enable systemd service
sudo make uninstall     # Remove binary and systemd service
make clean              # Remove object files and binary
make syntax-check       # Check C syntax without compiling
make check-pigpio       # Verify pigpio library is installed
```

**Custom GPIO pins:**
```bash
make CFLAGS="-DDIO_PIN=18 -DCLK_PIN=19"
```

**Run:**
```bash
sudo ./tm1637_temperature        # Default 60s interval
sudo ./tm1637_temperature -i 30  # Custom interval
```

## Architecture

```
main.c              - Entry point, CLI parsing (-i interval, -h help), signal handling (SIGINT)
tm1637_rpi_pigpio.c - TM1637 protocol implementation, open-drain GPIO via pigpio
tm1637_rpi_pigpio.h - GPIO pin definitions (DIO_PIN=24, CLK_PIN=23), display commands
get_temp.c          - Executes r4dcb08 binary, parses float output, returns int16_t (temp*10)
get_temp.h          - TEMP_ERROR constant (-9999)
```

**Key functions:**
- `TM1637_init()` / `TM1637_cleanup()` - Initialize/terminate pigpio
- `TM1637_write_num(int16_t)` - Display temperature (value is temp*10 for 0.1° resolution)
- `TM1637_write_err()` - Display "Err" on sensor failure
- `get_temp()` - Returns temperature*10 or TEMP_ERROR

## Hardware Requirements

- 4.7kΩ pull-up resistors on CLK and DIO lines (mandatory)
- TM1637 powered at 3.3V only (5V damages GPIO)
- Default pins: GPIO 23 (CLK), GPIO 24 (DIO)
- Requires `r4dcb08` binary in `/usr/local/bin/`

## CPU Usage Note

pigpio uses continuous GPIO sampling which causes ~12% CPU on RPi1 (reduced from ~23% by using 10μs sampling period instead of default 5μs). This is configured via `gpioCfgClock()` before `gpioInitialise()`.

## Dependencies

```bash
sudo apt install libpigpio-dev libpigpio1
```
