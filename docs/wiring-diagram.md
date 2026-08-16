# Drain Guard - Wiring Diagram

## ESP32 DevKit V1 (30-pin) Pin Connections

### HC-SR04 Ultrasonic Sensor
- **VCC** → 5V
- **GND** → GND
- **TRIG** → GPIO 5
- **ECHO** → GPIO 18

### TB6612FNG Motor Driver
- **VCC** → 3.3V
- **GND** → GND
- **VM** → 12V (External power supply)
- **PWMA** → GPIO 32
- **AIN1** → GPIO 25
- **AIN2** → GPIO 26
- **PWMB** → GPIO 33
- **BIN1** → GPIO 27
- **BIN2** → GPIO 14
- **STBY** → GPIO 19
- **Motor A** → DC Motor 1 (Left/Right)
- **Motor B** → DC Motor 2 (Left/Right)

### A9G GPS/GPRS Module
- **VCC** → 5V
- **GND** → GND
- **TXD** → GPIO 16 (RX2)
- **RXD** → GPIO 17 (TX2)
- **PWRKEY** → GPIO 4
- **Antenna** → GPS Antenna (required for GPS fix)

### A7670 GSM/LTE Module
- **VCC** → 5V
- **GND** → GND
- **TXD** → GPIO 13 (RX1)
- **RXD** → GPIO 12 (TX1)
- **PWRKEY** → GPIO 15
- **SIM Card** → Insert valid SIM card

### ESP32-CAM (Separate Module)
The ESP32-CAM runs its own firmware and connects via WiFi.
If wiring to main ESP32:
- **SDA** → GPIO 21 (Optional I2C)
- **SCL** → GPIO 22 (Optional I2C)
- **VCC** → 5V
- **GND** → GND

## Power Supply

### Main Power
- **12V DC Power Supply** (2A or higher recommended)
  - 12V → Motor Driver VM
  - Use voltage regulator or buck converter for:
    - 5V → ESP32 VIN, A9G, A7670, Ultrasonic sensor
    - 3.3V is provided by ESP32 onboard regulator

### Power Distribution
```
12V Power Supply
├── TB6612 VM (Motors)
├── 5V Buck Converter
│   ├── ESP32 VIN
│   ├── A9G VCC
│   ├── A7670 VCC
│   ├── HC-SR04 VCC
│   └── ESP32-CAM VCC
└── Common GND
```

## Important Notes

1. **Serial Ports**: ESP32 has 3 hardware serial ports:
   - Serial0 (USB): Debugging/Programming
   - Serial1 (GPIO 12/13): A7670 SMS Module
   - Serial2 (GPIO 16/17): A9G GPS Module

2. **Power Requirements**:
   - A9G: 500-800mA peak (during transmission)
   - A7670: 1-2A peak (during transmission)
   - Motors: 500-1000mA each
   - ESP32: 500mA typical
   - Use adequate power supply (12V 3A recommended)

3. **Antenna Connections**:
   - A9G requires GPS antenna for location fix
   - A7670 requires GSM antenna for cellular connection
   - Keep antennas away from each other to avoid interference

4. **Motor Wiring**:
   - Connect motors to appropriate gearbox/mechanism
   - Add flyback diodes if not included in TB6612
   - Test motor direction and adjust in code if needed

5. **ESP32-CAM**:
   - Runs separate firmware (esp32cam.ino)
   - Can be programmed via FTDI adapter
   - Requires good 5V power supply (brown-out issues common)
   - Connects to same WiFi network as main ESP32

## Pin Assignment Summary

| Component | ESP32 GPIO | Function |
|-----------|-----------|----------|
| Ultrasonic TRIG | 5 | Output |
| Ultrasonic ECHO | 18 | Input |
| Motor AIN1 | 25 | Output |
| Motor AIN2 | 26 | Output |
| Motor BIN1 | 27 | Output |
| Motor BIN2 | 14 | Output |
| Motor PWMA | 32 | PWM Output |
| Motor PWMB | 33 | PWM Output |
| Motor STBY | 19 | Output |
| A9G RX | 16 | Serial2 TX |
| A9G TX | 17 | Serial2 RX |
| A9G Power | 4 | Output |
| A7670 RX | 13 | Serial1 TX |
| A7670 TX | 12 | Serial1 RX |
| A7670 Power | 15 | Output |
| I2C SDA | 21 | I2C (optional) |
| I2C SCL | 22 | I2C (optional) |

## Circuit Diagram

```
                    ┌─────────────────┐
                    │  ESP32 DevKit   │
                    │    V1 (30-pin)  │
                    └─────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
  ┌─────▼─────┐      ┌─────▼─────┐      ┌─────▼─────┐
  │ Ultrasonic│      │   TB6612  │      │    A9G    │
  │  Sensor   │      │   Motor   │      │    GPS    │
  └───────────┘      │  Driver   │      └───────────┘
                     └─────┬─────┘
                           │
                     ┌─────▼─────┐
                     │ DC Motors │
                     │  (2x)     │
                     └───────────┘

        ┌───────────────────┐
        │      A7670        │
        │    GSM/LTE        │
        └───────────────────┘

        ┌───────────────────┐
        │   ESP32-CAM       │
        │  (Separate WiFi)  │
        └───────────────────┘
```

## Testing Checklist

- [ ] Verify all power connections (voltages)
- [ ] Test ultrasonic sensor readings
- [ ] Test motor direction (forward/reverse)
- [ ] Verify A9G GPS fix (takes 1-3 minutes outdoors)
- [ ] Test A7670 SMS send/receive
- [ ] Verify ESP32-CAM stream
- [ ] Test WiFi connectivity
- [ ] Check all grounds are common
- [ ] Measure current draw under load
- [ ] Test emergency stop functionality
