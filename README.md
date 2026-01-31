# TM1637 Temperature Display for Raspberry Pi

Temperature display on 4-digit 7-segment TM1637 display for Raspberry Pi using the pigpio library. The program reads temperature from an external sensor and displays it with 0.1°C resolution.

**⚠️ COMPATIBILITY:** Works on Raspberry Pi 1, 2, 3, 4. **Does NOT work on RPi5** (pigpio is not compatible with RPi5 as of 2025-10-09).

## Features

- ✅ **High precision**: Temperature display with 0.1°C resolution
- ✅ **Configurable GPIO pins**: Ability to change pins at compile time
- ✅ **Configurable interval**: Adjustable measurement interval from command line
- ✅ **Open-drain communication**: TM1637 protocol implementation with external pull-up resistors
- ✅ **Robust error handling**: Displays "Err" on temperature reading failure
- ✅ **Systemd support**: Ready-made service file for automatic startup
- ✅ **Graceful shutdown**: Correct termination with Ctrl+C
- ✅ **Hardware validation**: Pull-up resistor presence check at startup

## Hardware Requirements

### Raspberry Pi
- Raspberry Pi 1, 2, 3, or 4
- **⚠️ WARNING:** Raspberry Pi 5 **IS NOT SUPPORTED** - pigpio library does not work correctly on RPi5 (as of 2025-10-09)
- Raspbian/Raspberry Pi OS
- Root access for GPIO operations

### TM1637 Display
- 4-digit 7-segment display with TM1637 controller
- **Power only at 3.3V** (5V may damage RPi GPIO pins!)
- 2x 4.7kΩ pull-up resistors

### External Temperature Sensor
- Binary `r4dcb08` installed in `/usr/local/bin/`
- Output in float format to stdout

## Hardware Wiring

**⚠️ WARNING:** Use only **3.3V power** for TM1637! 5V power can **permanently damage GPIO pins** of Raspberry Pi!

```
TM1637 Display    Raspberry Pi      Pull-up Resistor
┌─────────────┐   ┌──────────────┐   ┌──────────────┐
│ VCC         │───│ 3.3V (Pin 1) │   │              │
│ GND         │───│ GND (Pin 6)  │   │              │
│ CLK         │───│ GPIO 23      │───│ 4.7kΩ to 3.3V│
│ DIO         │───│ GPIO 24      │───│ 4.7kΩ to 3.3V│
└─────────────┘   └──────────────┘   └──────────────┘
```

### Wiring Diagram

```
3.3V ────┬────────── TM1637 VCC
         │
         ├─── 4.7kΩ ─┬─── GPIO 23 (CLK) ─── TM1637 CLK
         │           │
         └─── 4.7kΩ ─┴─── GPIO 24 (DIO) ─── TM1637 DIO

GND ──────────────────────────────────────── TM1637 GND
```

**⚠️ Important:**
- Pull-up resistors are **mandatory** for proper open-drain communication!
- **Never use 5V power** - only 3.3V!

## Installation

### 1. Install Dependencies
```bash
# Install pigpio library
sudo apt update
sudo apt install -y libpigpio-dev libpigpio1

# Or use Makefile
make install-pigpio
```

### 2. Verify pigpio Installation
```bash
make check-pigpio
```

### 3. Compile and Install
```bash
make
sudo make install
```

This installs `tm1637_temperature` to `/usr/local/bin/`.

To uninstall:
```bash
sudo make uninstall
```

## Compilation

### Basic Compilation
```bash
make
```

### Compile with Custom GPIO Pins
```bash
make CFLAGS="-DDIO_PIN=18 -DCLK_PIN=19"
```

### Debug Version
```bash
make debug
```

### Available Makefile Commands
```bash
make              # compile program
make install      # install to /usr/local/bin
make install-service # install + enable systemd service
make uninstall    # complete uninstall (incl. systemd)
make clean        # clean object files
make run          # compile and run (60s interval)
make run-fast     # run with 10s interval
make debug        # compile debug version
make install-pigpio # install pigpio library
make check-pigpio # check pigpio
make syntax-check # syntax check
make info         # project information
make help         # display help
```

## Usage

### Basic Run
```bash
# Default 60 second interval
sudo tm1637_temperature

# With custom interval
sudo tm1637_temperature -i 30    # 30 seconds
sudo tm1637_temperature -i 120   # 2 minutes

# Display help
tm1637_temperature -h
```

### Example Output
```
Temperature display on TM1637 display on Raspberry Pi 1 (pigpio)
==================================================================
Measurement interval: 60 seconds
Press Ctrl+C to exit

TM1637 pins validated and initialized (GPIO 24=DIO, GPIO 23=CLK)
23.5
```

### Systemd Service

#### Service Installation
```bash
# Automatic installation (compile, install, enable, start)
make install-service
```

Or manually:
```bash
sudo cp tm1637_temperature.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable tm1637_temperature
sudo systemctl start tm1637_temperature
```

#### Complete Uninstall
```bash
# Removes binary and systemd service
make uninstall
```

#### Service Management
```bash
# Service status
sudo systemctl status tm1637_temperature

# Display logs
sudo journalctl -u tm1637_temperature -f

# Stop service
sudo systemctl stop tm1637_temperature

# Disable automatic startup
sudo systemctl disable tm1637_temperature
```

## Configuration

### GPIO Pins
Default GPIO pins can be changed at compile time:

```bash
# Change to GPIO 18 (DIO) and GPIO 19 (CLK)
make CFLAGS="-DDIO_PIN=18 -DCLK_PIN=19"
```

### Measurement Interval
Interval can be set using the `-i` argument:

```bash
sudo ./tm1637_temperature -i 10     # 10 seconds
sudo ./tm1637_temperature -i 300    # 5 minutes
```

### Systemd Service Configuration
To change arguments in service file, edit:

```ini
ExecStart=/usr/local/bin/tm1637_temperature -i 30
```

## Project Architecture

```
TM1637_temperature/
├── main.c                    # Main program loop
├── tm1637_rpi_pigpio.c      # TM1637 protocol implementation
├── tm1637_rpi_pigpio.h      # TM1637 header with GPIO definitions
├── get_temp.c               # Temperature reading from ext. sensor
├── get_temp.h               # Header for get_temp
├── Makefile                 # Build system
├── tm1637_temperature.service # Systemd service file
├── CLAUDE.md                # Claude Code instructions
└── README.md                # This file
```

### Modules

#### main.c
- Main program loop with argument parsing
- Signal handling for graceful shutdown
- Calling TM1637 and temperature functions

#### tm1637_rpi_pigpio.c/.h
- Complete TM1637 protocol implementation
- Open-drain communication with pigpio library
- Hardware validation of pull-up resistors
- Display of numbers and error messages

#### get_temp.c/.h
- Interface for external temperature sensor
- Calling `/usr/local/bin/r4dcb08 -f` command
- Parsing float values from stdout

## Troubleshooting

### Hardware Compatibility

**⚠️ Raspberry Pi 5 is not supported**
```
Problem: pigpio library does not work correctly on Raspberry Pi 5
Status: As of 2025-10-09, pigpio is not compatible with RPi5
```

**Solution:**
- Use Raspberry Pi 1, 2, 3, or 4
- On RPi5 consider alternative GPIO libraries (gpiod, lgpio)
- Monitor pigpio development for future RPi5 support

### Program Won't Start

**Error: "Permission denied"**
```bash
# Solution: Run with sudo privileges
sudo ./tm1637_temperature
```

**Error: "Error initializing pigpio library"**
```bash
# Check pigpio installation
make check-pigpio

# Reinstall pigpio
make install-pigpio
```

### Hardware Problems

**Error: "Missing pull-up resistors"**
```
Error: Missing pull-up resistors
GPIO 24 (DIO): LOW, GPIO 23 (CLK): LOW
Check wiring of 4.7kΩ pull-up resistors to 3.3V
```

**Solution:**
1. Check wiring of 4.7kΩ pull-up resistors
2. Resistors must be connected between GPIO pins and 3.3V
3. Use multimeter to verify connections

**Display not lit**
1. Check TM1637 power (only 3.3V!)
2. Verify correct GPIO pins in configuration
3. Try another TM1637 display

**⚠️ Damaged GPIO pins**
- If you accidentally used 5V power, GPIO pins may be permanently damaged
- Symptoms: GPIO pins don't respond, program reports initialization errors
- Solution: Use different GPIO pins or replace Raspberry Pi

### Software Problems

**Displays "Err" on display**
```bash
# Check presence of r4dcb08
ls -la /usr/local/bin/r4dcb08

# Test r4dcb08 manually
/usr/local/bin/r4dcb08 -f
```

**High CPU usage on RPi1**

The pigpio library uses a background thread that continuously samples GPIO pins. This causes noticeable CPU load, especially on older Raspberry Pi models:

| Model | Default (5μs) | Optimized (10μs) |
|-------|---------------|------------------|
| RPi1  | ~23% CPU      | ~12% CPU         |
| RPi2+ | <5% CPU       | <3% CPU          |

The program uses the maximum sampling period (10μs) to minimize CPU usage. This is configured via `gpioCfgClock(10, PI_CLOCK_PCM, 0)` before pigpio initialization.

Note: The measurement interval (`-i` flag) has minimal impact on CPU usage - the load comes from pigpio's continuous GPIO sampling, not from temperature readings.

### Systemd Service Problems

**Service won't start**
```bash
# Check status
sudo systemctl status tm1637_temperature

# Display detailed logs
sudo journalctl -u tm1637_temperature -n 50
```

**Service restarts too often**
```bash
# Check StartLimitBurst in service file
# Change RestartSec to higher value
```

## Development

### Debug Mode
```bash
make debug
sudo tm1637_temperature -i 5
```

### Syntax Check
```bash
make syntax-check
```

### Adding New Features
1. Edit respective .c/.h files
2. Update Makefile if needed
3. Compile: `make`
4. Test: `make run`

## Security Warning

**⚠️ CRITICAL:**
- **Always use only 3.3V power** for TM1637 display
- **Never connect 5V** to Raspberry Pi GPIO pins
- **5V power can permanently damage** GPIO pins and entire Raspberry Pi
- Always check voltage with multimeter before connecting

## License

MIT License - see LICENSE file for details.

## Author

Original implementation ported from ATtiny13A to Raspberry Pi with pigpio library.

## Changelog

### v1.2 (2026-01-31)
- ✅ Installation to `/usr/local/bin` via `make install`
- ✅ Binary `r4dcb08` expected in `/usr/local/bin/`
- ✅ Updated systemd service for installed paths
- ✅ Added `make install-service` for automatic systemd setup
- ✅ Added `make uninstall` with complete cleanup (binary + systemd)

### v1.1 (2025-09-03)
- ✅ Configurable GPIO pins via CFLAGS
- ✅ Configurable measurement interval from CLI
- ✅ Improved Makefile commands
- ✅ Added README.md
- ✅ Security warning for 3.3V power

### v1.0 (2025-08-28)
- ✅ Basic TM1637 implementation
- ✅ Temperature reading from external sensor
- ✅ Systemd service support
- ✅ Hardware validation of pull-up resistors

## Support

For bug reports or feature suggestions, create an issue in the repository or contact the author.
