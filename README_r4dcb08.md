# R4DCB08 Temperature Sensor Utility

A command-line utility for communicating with R4DCB08 temperature sensor modules via serial port.

## Overview

This utility allows you to:
- Read temperature data from up to 8 channels in real-time
- Configure device settings (address, baudrate)
- Read and set temperature correction values
- Monitor temperature with configurable intervals

## Installation

### Prerequisites

- C compiler (GCC/Clang)
- Make utility
- Serial port support (Linux/Unix-based systems)

### Compilation

```bash
make
```

## Usage

The utility provides several operation modes based on the command-line options used.

### Basic Examples

1. Read temperature from the default channel:
```bash
./r4dcb08
```

2. Read temperature from multiple channels with 2 second intervals:
```bash
./r4dcb08 -n 4 -t 2
```

3. Read temperature correction values:
```bash
./r4dcb08 -c
```

4. Set temperature correction for channel 3 to 1.5°C:
```bash
./r4dcb08 -s 3,1.5
```

5. Change device address from 1 to 5:
```bash
./r4dcb08 -a 1 -w 5
```

6. Set device baudrate to 19200:
```bash
./r4dcb08 -a 1 -x 4
```

7. Enable three-point median filter for temperature readings:
```bash
./r4dcb08 -m
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-p [name]` | Select port | `/dev/ttyUSB0` |
| `-a [address]` | Select device address | `01H` |
| `-b [n]` | Set baudrate on serial port {1200, 2400, 4800, 9600, 19200} | 9600 |
| `-t [time]` | Time step [s] | 1s |
| `-n [num]` | Number of channels to read (1-8) | 1 |
| `-c` | Read correction temperature [°C] | - |
| `-w [address]` | Write new device address (1-254) | - |
| `-x [n]` | Set baudrate on R4DCB08 device {0:1200, 1:2400, 2:4800, 3:9600, 4:19200} | - |
| `-s [ch,Tc]` | Set temperature correction Tc for channel ch | - |
| `-m` | Enable three-point median filter for temperature readings | Off |
| `-f` | Enable one shot measure without timestamp | Off |
| `-h` or `-?` | Display help | - |

## Notes

- Temperature readings are in degrees Celsius
- Temperature range: -55.0°C to 125.0°C (values outside this range will display as NaN)
- When reading temperatures continuously, press Ctrl+C to stop
- Device address changes and baudrate settings take effect immediately or after reboot (depending on device model)

## Technical Details

### Protocol

The utility communicates with R4DCB08 modules using Modbus RTU protocol over serial port:
- Function code 0x03 for reading data
- Function code 0x06 for writing registers

### Register Map

| Register | Description |
|----------|-------------|
| 0x0000-0x0007 | Temperature readings (channels 1-8) |
| 0x0008-0x000F | Temperature correction values (channels 1-8) |
| 0x00FE | Device address |
| 0x00FF | Baudrate setting |

### Architecture

The utility has a modular architecture:
- **Configuration Management**: Uses a central `ProgramConfig` structure for all settings
- **Consistent Error Handling**: Unified error code system via `AppStatus` enum
- **Signal Handling**: Clean termination with Ctrl+C, handling both SIGINT and SIGTERM
- **Module Communication**: Enhanced robustness with improved monada function
- **Data Filtering**: Optional three-point median filter for temperature readings

### Error Codes

The program uses the following error code categories:
- 0: Successful operation
- -10 to -19: Command-line argument errors
- -20 to -29: Communication errors
- -30 to -39: Operation errors

## Version

V1.7 (2025-06-06)

## Changelog

### V1.7 (2025-06-06)
- **Major refactoring and code improvements:**
- Unified error handling system - integrated MonadaStatus into central AppStatus
- Centralized constants in constants.h (eliminated duplicate definitions)
- Enhanced packet functions with proper AppStatus return codes
- Fixed resource leak in port initialization
- Improved progname initialization
- Added comprehensive error messages for all packet operations
- Updated Makefile to auto-extract VERSION from revision.h
- Enhanced code reliability and maintainability

### V1.6 (2025-06-06)
- Fixed baud rate table bug (1200 baud correction)
- Improved error handling consistency
- Code cleanup and optimization

### V1.5 (2025-04-17)
- Implemented consistent error handling system with `AppStatus` codes
- Modularized main function with `ProgramConfig` structure
- Improved signal handling for clean termination
- Consolidated reading and writing functions
- Enhanced code organization and maintainability

### V1.4 (2025-04-14/16)
- Enhanced error handling in monada function
- Added constants to replace magic values
- Added median filter for temperature readings
- Improved input validation
