# Updated Wiring Diagram with Corrected Pins

Complete wiring diagram for Drain Guard system with corrected pin assignments and PCA9685 servo controller.

## System Overview

```
ESP32 DevKit V1 (30-pin)
├── A7670 (SMS) - Serial2 (GPIO 16/17)
├── A9G (GPS) - Serial1 (GPIO 32/33)
├── HC-SR04 (Ultrasonic) - GPIO 25/34
├── TB6612FNG (Motors) - GPIO 4,12,13,14,23,26,27
├── PCA9685 (Servos) - I2C (GPIO 21/22)
└── ESP32-CAM (WiFi) - Separate module
```

## Detailed Connections

### 1. A7670 GSM/LTE Module (SMS)

```
A7670          ESP32
─────────────────────
VCC     ──────> 5V
GND     ──────> GND
TXD     ──────> GPIO 16 (RX2)
RXD     ──────> GPIO 17 (TX2)
PWRKEY  ──────> GPIO 15
ANT     ──────> GSM Antenna
SIM     ──────> Insert SIM card
```

**Notes:**
- Requires 5V power supply
- Peak current: 1-2A during transmission
- Keep antenna away from other antennas
- SIM card must be activated

### 2. A9G GPS/GPRS Module

```
A9G            ESP32
─────────────────────
VCC     ──────> 5V
GND     ──────> GND
TX      ──────> GPIO 33 (RX)
RX      ──────> GPIO 32 (TX)
PWRKEY  ──────> GPIO 5
ANT     ──────> GPS Antenna
```

**Notes:**
- Requires 5V power supply
- Peak current: 500-800mA
- GPS antenna must be external
- Takes 1-3 minutes for GPS fix outdoors

### 3. HC-SR04 Ultrasonic Sensor

```
HC-SR04        ESP32
─────────────────────
VCC     ──────> 5V
GND     ──────> GND
TRIG    ──────> GPIO 25
ECHO    ──────> GPIO 34 (input only)
```

**Notes:**
- Requires 5V for reliable operation
- GPIO 34 is input-only (no pull-up/down)
- Max range: 4 meters
- Mounting: Face downward to water surface

### 4. TB6612FNG Motor Driver

```
TB6612         ESP32          External
──────────────────────────────────────
VCC     ──────> 3.3V         (logic)
VM      ──────────────────> 12V PSU
GND     ──────> GND    ───> GND

PWMA    ──────> GPIO 26      (Motor A speed)
AIN1    ──────> GPIO 27      (Motor A dir)
AIN2    ──────> GPIO 14      (Motor A dir)

PWMB    ──────> GPIO 13      (Motor B speed)
BIN1    ──────> GPIO 12      (Motor B dir)
BIN2    ──────> GPIO 23      (Motor B dir)

STBY    ──────> GPIO 4       (standby control)

AO1     ──────────────────> Motor A+
AO2     ──────────────────> Motor A-
BO1     ──────────────────> Motor B+
BO2     ──────────────────> Motor B-
```

**Notes:**
- VCC is logic voltage (3.3V)
- VM is motor voltage (12V for high torque)
- STBY must be HIGH to enable motors
- Include flyback diodes if not on board
- GPIO 12: Must be LOW at boot
- GPIO 14: Strapping pin, keep LOW at boot

### 5. PCA9685 Servo Controller

```
PCA9685        ESP32          External
──────────────────────────────────────
VCC     ──────> 3.3V or 5V   (logic)
GND     ──────> GND    ───> GND
SDA     ──────> GPIO 21
SCL     ──────> GPIO 22
OE      ──────> GND           (always enable)

V+      ──────────────────> 5-6V PSU (servos)
GND     ──────────────────> GND

Channel 0 ─────────────────> Base Servo
Channel 1 ─────────────────> Shoulder Servo
Channel 2 ─────────────────> Elbow Servo
Channel 3 ─────────────────> Gripper Servo
```

**Servo Connections (each channel):**
```
Yellow/White wire → PWM signal
Red wire         → V+ (from V+ terminal)
Brown/Black wire → GND (from GND terminal)
```

**Notes:**
- I2C address: 0x40 (default)
- Servo power: External 5-6V, 2A+
- Do NOT power servos from ESP32
- Add 4.7kΩ pull-ups on SDA/SCL if needed
- Can control up to 16 servos

### 6. ESP32-CAM Module (Optional)

```
ESP32-CAM      Power
──────────────────────
5V      ──────> 5V PSU
GND     ──────> GND
```

**Notes:**
- Runs independent firmware
- Connects via WiFi (same network)
- Can optionally wire I2C for control
- Needs stable 5V (brownout common issue)

## Complete Schematic Diagram

```
                    12V Power Supply (3A+)
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
   TB6612 VM          5V Regulator      6V Regulator
        │                  │                  │
        │                  ├─> ESP32 VIN      ├─> PCA9685 V+
        │                  ├─> A7670 VCC      │   (Servo Power)
        │                  ├─> A9G VCC        │
        │                  ├─> HC-SR04 VCC    │
        │                  └─> ESP32-CAM      │
        │                                     │
        │                                     │
    ┌───┴────┐                         ┌─────┴─────┐
    │ Motors │                         │  Servos   │
    │  (2x)  │                         │   (4x)    │
    └────────┘                         └───────────┘
           
           Common GND for all components
           ═══════════════════════════════
```

## Power Distribution Detail

### Main Power Bus
```
12V PSU (3A minimum)
├─> TB6612 VM pin
└─> 5V Buck Converter (3A)
    ├─> ESP32 VIN (0.5A)
    ├─> A7670 VCC (2A peak)
    ├─> A9G VCC (0.8A peak)
    ├─> HC-SR04 VCC (0.1A)
    ├─> ESP32-CAM (0.5A)
    └─> 6V Regulator (optional for servos)
        └─> PCA9685 V+ (2A for 4 servos)
```

### Current Requirements
| Component | Typical | Peak | Notes |
|-----------|---------|------|-------|
| ESP32 | 200mA | 500mA | WiFi active |
| A7670 | 100mA | 2A | During transmission |
| A9G | 100mA | 800mA | During transmission |
| HC-SR04 | 15mA | 20mA | Per measurement |
| TB6612 | - | - | Pass-through |
| Motors (2x) | 500mA | 2A | Each motor |
| Servos (4x) | 400mA | 2A | Total, under load |
| ESP32-CAM | 200mA | 500mA | During streaming |
| **TOTAL** | ~2A | **8A** | Size accordingly |

## Wiring Color Code (Recommended)

- **Red**: +12V, +5V, +3.3V
- **Black**: GND
- **Yellow**: Signal/Data/PWM
- **Green**: TX (transmit)
- **Blue**: RX (receive)
- **Orange**: Control signals
- **White**: I2C (SDA/SCL)

## PCB Layout Recommendations

### Component Placement
```
┌─────────────────────────────────┐
│  [GPS ANT]         [GSM ANT]    │
│                                  │
│  [A9G]             [A7670]       │
│                                  │
│      [ESP32 DevKit V1]           │
│                                  │
│  [PCA9685]    [HC-SR04]          │
│                                  │
│  [TB6612]     [Power Jack]       │
└─────────────────────────────────┘
```

**Tips:**
- Keep antennas at opposite corners
- Place ESP32 centrally
- Position power components at edge
- Keep I2C lines short
- Route motor wires away from signals

## Assembly Checklist

### Before Power-On
- [ ] All grounds connected
- [ ] No shorts between power rails
- [ ] Correct voltage at each component
- [ ] Servos connected to V+ terminal (not ESP32)
- [ ] Antennas properly connected
- [ ] SIM card inserted in A7670

### Initial Power-On
- [ ] ESP32 boots (check LED)
- [ ] Serial output shows startup
- [ ] WiFi connects
- [ ] I2C devices detected (PCA9685)
- [ ] GPS module responding
- [ ] GSM module responding

### Component Tests
- [ ] Ultrasonic reading valid values
- [ ] Motors spin both directions
- [ ] Servos move to home position
- [ ] GPS gets fix (outdoors)
- [ ] SMS send/receive works
- [ ] Camera streams video

## Troubleshooting

### ESP32 Won't Boot
- Check GPIO 0 is HIGH (not grounded)
- GPIO 12 must be LOW at boot
- GPIO 15 must be LOW at boot
- Check power supply voltage

### I2C Not Working
- Verify SDA/SCL connections (21/22)
- Add 4.7kΩ pull-up resistors
- Check PCA9685 address (0x40)
- Use I2C scanner to detect devices

### Servos Not Moving
- Check V+ power (5-6V at PCA9685)
- Verify common ground
- Test servo separately
- Check PCA9685 initialization

### Motors Not Spinning
- Verify GPIO 4 (STBY) is HIGH
- Check 12V at VM terminal
- Test motors with direct power
- Verify PWM output

### GPS No Fix
- Check antenna connection
- Test outdoors (not indoors)
- Wait 3-5 minutes
- Verify serial communication

### SMS Not Working
- Check SIM card activation
- Verify antenna connection
- Check cellular signal
- Test AT commands manually

## Safety Warnings

⚠️ **Critical Safety Points:**

1. **Never cross 12V and 3.3V** - Will destroy ESP32
2. **Common ground essential** - Connect all GND together
3. **Servo power separate** - Don't power from ESP32 regulator
4. **Fuse the 12V line** - Protect against shorts
5. **Waterproof enclosure** - IP65+ rating required
6. **Heat management** - Regulators may need heatsinks

## Testing Procedure

1. **Bench test** (no load): Verify all voltages
2. **Communication test**: Check serial ports and I2C
3. **Sensor test**: Verify readings
4. **Motor test** (no load): Test movement
5. **Servo test** (no load): Test each servo
6. **Integration test**: Run full sequence
7. **Load test**: Test with actual mechanism
8. **Endurance test**: Run for 24 hours

## Circuit Protection

### Recommended
- 3A fuse on 12V input
- Reverse polarity protection (diode)
- TVS diodes on exposed lines
- Decoupling capacitors (100nF) on each IC
- Large capacitor (1000µF) on servo power

### Optional
- Voltage monitoring circuit
- Over-current detection
- Thermal shutdown
- Backup battery for ESP32
