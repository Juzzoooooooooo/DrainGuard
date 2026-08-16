# PCA9685 Servo Controller Guide

Complete guide for integrating the PCA9685 16-channel PWM servo driver with ESP32 for robotic arm control.

## Overview

The PCA9685 is a 16-channel, 12-bit PWM I2C servo driver. It's perfect for controlling multiple servos with just 2 I2C wires (SDA/SCL).

## Hardware Connections

### ESP32 to PCA9685

| PCA9685 Pin | ESP32 Pin | Description |
|-------------|-----------|-------------|
| VCC | 3.3V or 5V | Logic power (5V preferred) |
| GND | GND | Common ground |
| SCL | GPIO 22 | I2C Clock |
| SDA | GPIO 21 | I2C Data |
| V+ | External 5-6V | Servo power supply |
| GND (V+) | GND | Servo power ground |

### Servo Connections

Connect servos to PCA9685 channels 0-15:
- **Channel 0**: Base servo (rotation)
- **Channel 1**: Shoulder servo (up/down)
- **Channel 2**: Elbow servo (extend/retract)
- **Channel 3**: Gripper servo (open/close)

Each servo channel has 3 pins:
- **GND**: Brown wire
- **V+**: Red wire (powered from V+ terminal)
- **PWM**: Orange/Yellow/White wire (signal)

## Power Supply

### Critical: Separate Power for Servos

```
5-6V Power Supply (2A+ recommended)
├── PCA9685 V+ terminal
├── Connect to all servo V+ pins
└── Common GND with ESP32

ESP32 Power
├── USB or external 5V
└── Common GND with PCA9685
```

**Important Notes:**
- Never power servos from ESP32 3.3V regulator
- Use external 5-6V power supply for servos (V+ terminal)
- Ensure common ground between ESP32 and servo power
- Typical servo draws 100-500mA under load
- 4 servos can draw 2A total - size your power supply accordingly

## Library Installation

### PlatformIO (platformio.ini)
```ini
lib_deps = 
    adafruit/Adafruit PWM Servo Driver Library@^2.4.1
    adafruit/Adafruit BusIO@^1.14.1
    wire
```

### Arduino IDE
1. Go to Sketch → Include Library → Manage Libraries
2. Search "Adafruit PWM Servo Driver"
3. Install "Adafruit PWM Servo Driver Library"
4. Install dependencies (Adafruit BusIO)

## Basic Usage

### Initialization
```cpp
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

void setup() {
  Wire.begin(21, 22); // SDA=21, SCL=22
  pwm.begin();
  pwm.setPWMFreq(60); // Servos run at 60Hz
  delay(10);
}
```

### Setting Servo Position
```cpp
// Set servo on channel 0 to position 300
pwm.setPWM(0, 0, 300);
```

## Pulse Width Calculation

PCA9685 uses 12-bit resolution (0-4095) at 60Hz:

### Servo Timing
- 50Hz = 20ms period
- 60Hz = 16.67ms period (we use 60Hz)
- Servo pulse: 1-2ms typically

### Position Mapping
At 60Hz (16.67ms period):
- 0 degrees ≈ 150 (1ms pulse)
- 90 degrees ≈ 330 (1.5ms pulse)
- 180 degrees ≈ 510 (2ms pulse)

Formula: `pulseWidth = (degrees / 180) * (510 - 150) + 150`

### Your Servo Values (from reference code)
- Base: 150-450 range
- Shoulder: 150-380 range
- Elbow: 300-380 range
- Gripper: 410-510 range

## API Endpoints

The firmware exposes these REST endpoints:

### Servo Control
```bash
# Set individual servo
POST /api/servo/base?position=300
POST /api/servo/shoulder?position=200
POST /api/servo/elbow?position=350
POST /api/servo/gripper?position=450

# Get all servo positions
GET /api/servo/status
```

### Arm Operations
```bash
# Pre-programmed sequences
POST /api/arm/open     # Open drain with arm
POST /api/arm/close    # Close drain with arm
POST /api/arm/home     # Return to home position
POST /api/arm/demo     # Run demo sequence
```

## Calibration Guide

### Step 1: Find Center Position
```cpp
// Set servo to approximate center
pwm.setPWM(0, 0, 330);
```

### Step 2: Find Limits
Gradually increase/decrease values until servo reaches physical limits:
```cpp
// Find minimum
for(int i = 150; i <= 600; i += 10) {
  pwm.setPWM(0, 0, i);
  delay(500);
}
```

### Step 3: Set Safe Limits
Update `config.h` with safe min/max values (leave margin from physical limits):
```cpp
#define SERVO_BASE_MIN 150
#define SERVO_BASE_MAX 450
```

## Smooth Movement

For smooth servo motion:
```cpp
void moveServoSmooth(int channel, int target, int current, int delayMs = 10) {
  if (target > current) {
    for(int pos = current; pos <= target; pos++) {
      pwm.setPWM(channel, 0, pos);
      delay(delayMs);
    }
  } else {
    for(int pos = current; pos >= target; pos--) {
      pwm.setPWM(channel, 0, pos);
      delay(delayMs);
    }
  }
}
```

## Mobile App Integration

The mobile app includes a Servo Control screen with:
- Individual servo sliders for manual control
- Quick action buttons (Open/Close/Home/Demo)
- Real-time position display
- Color-coded servo indicators

Access via Home screen → "Servo Arm" button

## Troubleshooting

### Servo Jittering
- Check power supply (needs adequate current)
- Ensure common ground
- Add capacitor (1000µF) across V+ and GND
- Reduce PWM frequency to 50Hz if needed

### Servo Not Moving
- Verify I2C connection (use I2C scanner)
- Check servo power (V+ terminal)
- Verify servo is functional (test with Arduino)
- Check position value is within servo range

### I2C Not Detected
```cpp
// I2C Scanner code
Wire.begin(21, 22);
Wire.beginTransmission(0x40);
byte error = Wire.endTransmission();
if (error == 0) {
  Serial.println("PCA9685 found at 0x40");
} else {
  Serial.println("PCA9685 not found!");
}
```

### Wrong I2C Address
Default is 0x40, but can be changed with solder bridges:
- A0-A5 pins on PCA9685 board
- Each pin adds to address: 0x40 + binary(A5-A0)

## Multiple PCA9685 Boards

Can control up to 62 PCA9685 boards on same I2C bus:
```cpp
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm2 = Adafruit_PWMServoDriver(0x41);
```

## Advanced Features

### External Clock
For precise timing, use external clock:
```cpp
pwm.setOscillatorFrequency(27000000);
pwm.setPWMFreq(60);
```

### Output Enable Pin
PCA9685 has OE (Output Enable) pin:
- Pull LOW to enable outputs
- Pull HIGH to disable all outputs (servos free)

## Safety Tips

1. **Power First**: Connect power before signals
2. **Test Limits**: Always test with low speeds first
3. **Emergency Stop**: Implement manual override
4. **Current Limits**: Monitor total current draw
5. **Mechanical Stops**: Add physical limiters
6. **Smooth Moves**: Use gradual movements, not instant jumps

## Example Sequences

### Pick and Place
```cpp
void pickAndPlace() {
  // Move to object
  pwm.setPWM(SERVO_BASE, 0, 250);
  delay(500);
  pwm.setPWM(SERVO_SHOULDER, 0, 380);
  delay(500);
  
  // Open gripper
  pwm.setPWM(SERVO_GRIPPER, 0, 510);
  delay(500);
  
  // Lower to object
  pwm.setPWM(SERVO_ELBOW, 0, 380);
  delay(500);
  
  // Close gripper
  pwm.setPWM(SERVO_GRIPPER, 0, 410);
  delay(1000);
  
  // Lift object
  pwm.setPWM(SERVO_ELBOW, 0, 300);
  delay(500);
  
  // Return to home
  pwm.setPWM(SERVO_SHOULDER, 0, 150);
  delay(500);
  pwm.setPWM(SERVO_BASE, 0, 330);
}
```

## Resources

- [PCA9685 Datasheet](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf)
- [Adafruit PCA9685 Guide](https://learn.adafruit.com/16-channel-pwm-servo-driver)
- [ESP32 I2C Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html)
