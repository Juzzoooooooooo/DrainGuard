# ESP32 Pin Mapping Table

Complete pin assignments for the Drain Guard system based on ESP32 DevKit V1 (30-pin).

## Complete Pin Assignment Table

| Component | Component Pin | ESP32 GPIO | Function | Notes |
|-----------|--------------|------------|----------|-------|
| **A7670 GSM/LTE** | | | | |
| | TXD | GPIO 16 | ESP32 RX2 | Serial2 receive |
| | RXD | GPIO 17 | ESP32 TX2 | Serial2 transmit |
| | PWR | GPIO 15 | Power control | Pull HIGH to power on |
| | GND | GND | Common ground | |
| **A9G GPS/GPRS** | | | | |
| | TX | GPIO 32 | ESP32 RX | Serial1 receive |
| | RX | GPIO 33 | ESP32 TX | Serial1 transmit |
| | PWR | GPIO 5 | Power control | Pull HIGH to power on |
| | GND | GND | Common ground | |
| **Ultrasonic HC-SR04** | | | | |
| | TRIG | GPIO 25 | Digital output | Trigger pulse |
| | ECHO | GPIO 34 | Digital input | Echo response (input only) |
| | VCC | 5V | Power | Needs 5V for reliable operation |
| | GND | GND | Common ground | |
| **TB6612FNG Motor Driver** | | | | |
| | PWMA | GPIO 26 | Motor A PWM | Speed control motor A |
| | AIN1 | GPIO 27 | Motor A direction | Direction bit 1 |
| | AIN2 | GPIO 14 | Motor A direction | Direction bit 2 |
| | PWMB | GPIO 13 | Motor B PWM | Speed control motor B |
| | BIN1 | GPIO 12 | Motor B direction | Direction bit 1 |
| | BIN2 | GPIO 23 | Motor B direction | Direction bit 2 |
| | STBY | GPIO 4 | Standby control | HIGH = active, LOW = standby |
| | VCC | 3.3V | Logic power | |
| | VM | Motor Battery | Motor power | 12V for motors |
| | GND | GND | Common ground | |
| **PCA9685 Servo Controller** | | | | |
| | SDA | GPIO 21 | I2C Data | Shared I2C bus |
| | SCL | GPIO 22 | I2C Clock | Shared I2C bus |
| | VCC | 3.3V or 5V | Logic power | 5V preferred |
| | V+ | External 5-6V | Servo power | Separate power for servos |
| | GND | GND | Common ground | |
| **ESP32-CAM** | | | | |
| | (WiFi) | - | Separate module | Runs own firmware |
| | Optional I2C | GPIO 21/22 | I2C communication | If wired control needed |

## Power Distribution

```
12V Power Supply (3A recommended)
├── TB6612 VM pin (Motor power)
└── 5V Buck Converter
    ├── ESP32 VIN (or USB)
    ├── A9G VCC
    ├── A7670 VCC
    ├── HC-SR04 VCC
    ├── PCA9685 V+ (servo power)
    └── ESP32-CAM VCC

3.3V (ESP32 onboard regulator)
├── TB6612 VCC
├── PCA9685 VCC (optional, can use 5V)
└── All logic signals
```

## GPIO Summary by Type

### Input Only Pins (No pull-up/pull-down, ADC capable)
- GPIO 34 - Ultrasonic ECHO
- GPIO 35 - Available
- GPIO 36 - Available
- GPIO 39 - Available

### Output/Input Pins
- GPIO 4 - TB6612 STBY
- GPIO 5 - A9G Power
- GPIO 12 - TB6612 BIN1 (boot fail if pulled HIGH)
- GPIO 13 - TB6612 PWMB
- GPIO 14 - TB6612 AIN2
- GPIO 15 - A7670 Power (boot fail if pulled HIGH)
- GPIO 16 - A7670 RX (Serial2 TX)
- GPIO 17 - A7670 TX (Serial2 RX)
- GPIO 21 - I2C SDA
- GPIO 22 - I2C SCL
- GPIO 23 - TB6612 BIN2
- GPIO 25 - Ultrasonic TRIG
- GPIO 26 - TB6612 PWMA
- GPIO 27 - TB6612 AIN1
- GPIO 32 - A9G RX (Serial1 TX)
- GPIO 33 - A9G TX (Serial1 RX)

### Reserved/Used by System
- GPIO 0 - Boot mode (must be HIGH at boot)
- GPIO 1 - USB TX (Serial debugging)
- GPIO 2 - Boot mode / LED (often onboard LED)
- GPIO 3 - USB RX (Serial debugging)
- GPIO 6-11 - Flash memory (DO NOT USE)

## Serial Ports Summary

| Port | TX Pin | RX Pin | Usage | Baud Rate |
|------|--------|--------|-------|-----------|
| Serial | GPIO 1 | GPIO 3 | USB/Debug | 115200 |
| Serial1 | GPIO 32 | GPIO 33 | A9G GPS | 115200 |
| Serial2 | GPIO 17 | GPIO 16 | A7670 SMS | 115200 |

## I2C Bus Summary

| Device | Address | SDA | SCL |
|--------|---------|-----|-----|
| PCA9685 | 0x40 (default) | GPIO 21 | GPIO 22 |
| ESP32-CAM (optional) | Custom | GPIO 21 | GPIO 22 |

## PWM Channels Usage

ESP32 has 16 PWM channels (0-15):
- Channel 0: Motor A PWM (GPIO 26)
- Channel 1: Motor B PWM (GPIO 13)
- Channels 2-15: Available

Note: PCA9685 handles servo PWM independently via I2C.

## Important Notes

1. **GPIO 34-39**: Input only, no internal pull-up/pull-down resistors
2. **Boot Strapping Pins**: GPIO 0, 2, 12, 15 affect boot mode
   - GPIO 0: Must be HIGH at boot (LOW = programming mode)
   - GPIO 12: Must be LOW at boot (boot fail if HIGH)
   - GPIO 15: Must be LOW at boot (boot fail if HIGH)
3. **Flash Pins**: GPIO 6-11 are used for flash memory - DO NOT USE
4. **ADC2**: Cannot be used when WiFi is active (GPIO 0, 2, 4, 12-15, 25-27)
5. **I2C**: Can share multiple devices on same bus (SDA/SCL)
6. **Power**: Ensure common ground for all components

## Testing Checklist

- [ ] Verify 5V at ultrasonic sensor VCC
- [ ] Verify 12V at TB6612 VM pin
- [ ] Verify 3.3V at all logic VCC pins
- [ ] Test each serial port independently
- [ ] Verify I2C devices detected (use I2C scanner)
- [ ] Test each GPIO output with LED
- [ ] Verify all grounds are common
- [ ] Check for no shorts between power rails

## Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| Won't boot | GPIO 12 or 15 pulled HIGH | Check connections, add pull-down if needed |
| Serial not working | TX/RX swapped | Swap TX/RX connections |
| I2C not detected | Wrong SDA/SCL or no pull-ups | Check wiring, add 4.7kΩ pull-ups |
| Motor not spinning | STBY pin LOW or no motor power | Set GPIO 4 HIGH, verify 12V at VM |
| Ultrasonic no reading | Needs 5V, not 3.3V | Connect VCC to 5V rail |
| GPS no fix | No antenna or indoors | Connect antenna, test outdoors |
