# TM1637 Temperature Display (MQTT)

Temperature display on 4-digit 7-segment TM1637 display using libgpiod. Subscribes to MQTT broker for temperature data and displays with 0.1°C resolution.

**Compatibility:** Any Linux SBC with /dev/gpiochip0 (Raspberry Pi 1-5, Orange Pi, etc.)

## Why libgpiod (not pigpio)

**DO NOT USE pigpio for new projects.** This project originally used pigpio but was rewritten due to serious problems:

### Problems with pigpio

1. **Constant ~17-23% CPU usage** - pigpio runs a background thread that continuously samples all GPIO pins at 5-10μs intervals, even when your application is idle. On Raspberry Pi 1 this means permanent 17-23% CPU load just for GPIO access.

2. **Not compatible with Raspberry Pi 5** - pigpio does not work on RPi5 and development appears stalled.

3. **Requires daemon or root** - either run pigpiod daemon or your application needs root privileges.

4. **Overkill for simple GPIO** - pigpio is designed for precise timing and PWM, not for simple bit-banging protocols like TM1637.

5. **Large dependency** - pulls in pthread, rt libraries unnecessarily.

### libgpiod advantages

- **Zero CPU usage when idle** - no background threads, no polling
- **Works on all Linux SBCs** - Raspberry Pi 1-5, Orange Pi, Rock Pi, etc.
- **Modern kernel interface** - uses /dev/gpiochip character device
- **Simple and clean API** - no daemons, no magic
- **Actively maintained** - part of Linux kernel ecosystem

## Hardware

```
3.3V ──┬────── TM1637 VCC
       ├─ 4.7k ─┬─ GPIO23 (CLK) ─── TM1637 CLK
       └─ 4.7k ─┴─ GPIO24 (DIO) ─── TM1637 DIO
GND ─────────────────────────────── TM1637 GND
```

**WARNING:** Use only 3.3V! 5V will damage GPIO pins.

Pull-up resistors (4.7kΩ) are mandatory for open-drain communication.

## Build

```bash
# Install dependencies
sudo apt install libgpiod-dev libmosquitto-dev

# Compile
make

# Install
sudo make install

# Install with systemd service
sudo make install-service
```

## Usage

```bash
./tm1637_temperature -b <broker> -t <topic> [-p port] [-i interval] [-h]

Options:
  -b broker   MQTT broker hostname/IP (required)
  -t topic    MQTT topic for temperature (required)
  -p port     MQTT port (default: 1883)
  -i interval Display update interval in seconds (default: 60)
  -h          Display this help
```

### Examples

```bash
# Basic usage
sudo ./tm1637_temperature -b 192.168.200.12 -t sensors/r4dcb08/1/temperature/ch1

# Custom port and interval
sudo ./tm1637_temperature -b mqtt.local -t home/temperature -p 1884 -i 5
```

### Testing with mosquitto_pub

```bash
# Publish test temperature
mosquitto_pub -h 192.168.200.12 -t sensors/r4dcb08/1/temperature/ch1 -m "23.5"
mosquitto_pub -h 192.168.200.12 -t sensors/r4dcb08/1/temperature/ch1 -m "-5.2"
mosquitto_pub -h 192.168.200.12 -t sensors/r4dcb08/1/temperature/ch1 -m "123.4"
```

### Display states

- `----` - Waiting for first MQTT message
- ` 23.5` - Temperature 0.0 to 99.9°C (leading space)
- `123.4` - Temperature 100.0 to 999.9°C
- `-5.2` - Negative temperature -99.9 to -0.1°C
- `Err` - Parse error (invalid MQTT payload)
- `OFL` - Overflow (temperature out of range -99.9 to 999.9°C)
- `OLd` - Stale data (no message received within watchdog timeout)

## Watchdog

Program includes a watchdog that detects stale data. If no valid MQTT message is received within the timeout period, display shows `OLd`.

**Timeout:** `interval × 2` (default: 120s)

**What watchdog detects:**
- Connection loss between subscriber and broker
- Broker crash/restart

**What watchdog does NOT detect:**
- Publisher stopped (if using retained messages, the old retained value appears fresh)

For immediate detection of publisher status, the publisher should send to a `status` topic with Last Will and Testament (LWT) set to `offline`.

## Custom GPIO pins

```bash
make CFLAGS="-DDIO_PIN=18 -DCLK_PIN=19"
```

## Files

```
main.c           - main loop, CLI, signal handling
tm1637_gpiod.c   - TM1637 protocol via libgpiod
tm1637_gpiod.h   - API and pin definitions
mqtt_temp.c      - MQTT subscriber (libmosquitto)
mqtt_temp.h      - MQTT API, TEMP_ERROR/TEMP_NO_DATA constants
```

## Systemd

The service file needs to be configured with your MQTT broker and topic before installation.

Edit `tm1637_temperature.service`:
```ini
ExecStart=/usr/local/bin/tm1637_temperature -b YOUR_BROKER -t YOUR_TOPIC
```

Then install:
```bash
# Install and enable
sudo make install-service

# Management
sudo systemctl status tm1637_temperature
sudo systemctl stop tm1637_temperature
sudo journalctl -u tm1637_temperature -f

# Uninstall
sudo make uninstall
```

## Troubleshooting

**"Cannot open /dev/gpiochip0"**
- Run with sudo
- Check: `ls -la /dev/gpiochip*`

**"Missing pull-up resistors"**
- Connect 4.7kΩ resistors between GPIO pins and 3.3V
- Check wiring with multimeter

**"Unable to connect to MQTT broker"**
- Check broker is running: `mosquitto_pub -h BROKER -t test -m test`
- Verify network connectivity: `ping BROKER`
- Check firewall allows port 1883

**Display shows "----" permanently**
- No MQTT messages received yet
- Verify topic is correct: `mosquitto_sub -h BROKER -t TOPIC`
- Check broker logs

**Display shows "Err"**
- MQTT payload is not a valid float
- Expected format: "23.5", "-5.2", "100"

**Display shows "OLd"**
- No MQTT message received within watchdog timeout (default 120s)
- Check publisher is running
- Check network connectivity to broker
- Verify topic with: `mosquitto_sub -h BROKER -t TOPIC`

## License

MIT
