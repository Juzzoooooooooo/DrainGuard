# Drain Guard Setup Using Powerbank

Complete wiring guide kung gumagamit ka ng **powerbank** instead of power supply.

## Kailangan Mo

### Hardware Na Meron Ka:
- ✅ **Powerbank** (5V, 2A output minimum)
- ✅ ESP32 DevKit V1
- ✅ PCA9685 Servo Controller
- ✅ 4× Servos (SG90 or similar)
- ✅ HC-SR04 Ultrasonic Sensor
- ✅ A9G GPS Module
- ✅ A7670 GSM Module

### Kailangan Mo Pang Bilhin:
1. **2× 18650 batteries + holder** (~₱200-300)
   - Para sa TB6612 motors (7.4V)
   - Alternative: 6× AA batteries = 9V
2. **USB cables** (at least 2-3 pieces)
   - Para sa connections, pwede i-cut
3. **Breadboard + Jumper wires** (~₱150)
4. **TB6612FNG Motor Driver** (if motors)
5. **2× DC Motors** (if kailangan)

## Wiring Diagram

```
┌─────────────────────────────────────────────────┐
│         POWERBANK (5V USB OUTPUT)               │
└──────────┬────────────┬─────────────────────────┘
           │            │
           │            │
    ┌──────▼──────┐  ┌──▼──────────────────┐
    │  USB Cable  │  │  USB Cable (cut)    │
    │  (intact)   │  │  Red = 5V           │
    └──────┬──────┘  │  Black = GND        │
           │         └──┬──────────────────┘
           │            │
    ┌──────▼──────┐     │
    │   ESP32     │     │
    │   (USB)     │     │
    └──────┬──────┘     │
           │            │
      GPIO 21/22        │
           │            │
    ┌──────▼────────────▼───┐
    │     PCA9685            │
    │  VCC ← 5V from ESP32   │
    │  V+ ← 5V from USB      │ ← Servos power
    │  SDA, SCL              │
    └────────────────────────┘
           │
      Servos (4×)


┌─────────────────────────────┐
│  2× 18650 BATTERIES         │
│  (7.4V total in series)     │
└──────────┬──────────────────┘
           │
    ┌──────▼──────┐
    │   TB6612    │
    │   VM ← 7.4V │
    └─────────────┘
           │
      Motors (2×)

⚠️ IMPORTANT: Connect ALL GND together!
```

## Step-by-Step Wiring

### Step 1: Powerbank → ESP32
```
Powerbank USB Output
  └─> USB Cable → ESP32 USB port

Simple! Plug and play.
```

### Step 2: Powerbank → Servos (via PCA9685)

**Cut a USB cable:**
```
USB Cable Cut:
├─> Red wire (5V) → PCA9685 V+ terminal
└─> Black wire (GND) → PCA9685 GND terminal
```

**PCA9685 Connections:**
```
PCA9685          Connection
─────────────────────────────────
VCC      ← ESP32 3.3V pin
GND      ← ESP32 GND + Powerbank GND
SDA      ← ESP32 GPIO 21
SCL      ← ESP32 GPIO 22
V+       ← Powerbank 5V (from cut USB)
GND (V+) ← Powerbank GND
```

### Step 3: PCA9685 → Servos
```
Servo 1 (Base)      → PCA9685 Channel 0
Servo 2 (Shoulder)  → PCA9685 Channel 1
Servo 3 (Elbow)     → PCA9685 Channel 2
Servo 4 (Gripper)   → PCA9685 Channel 3

Each servo:
  Brown wire  → GND (on PCA9685)
  Red wire    → V+ (on PCA9685)
  Orange wire → PWM signal (channel 0-3)
```

### Step 4: ESP32 → A7670 (SMS)
```
A7670        ESP32
────────────────────
VCC   ← 5V from powerbank (cut another USB)
GND   ← Common GND
TXD   ← GPIO 16 (RX2)
RXD   ← GPIO 17 (TX2)
```

### Step 5: ESP32 → A9G (GPS)
```
A9G          ESP32
────────────────────
VCC   ← 5V from powerbank
GND   ← Common GND
TX    ← GPIO 33 (RX)
RX    ← GPIO 32 (TX)
```

### Step 6: ESP32 → HC-SR04 (Ultrasonic)
```
HC-SR04      ESP32
────────────────────
VCC   ← 5V from powerbank
GND   ← Common GND
TRIG  ← GPIO 25
ECHO  ← GPIO 34
```

### Step 7: Batteries → TB6612 → Motors
```
2× 18650 in series (7.4V)
  ├─> TB6612 VM pin
  └─> TB6612 GND

TB6612       ESP32
─────────────────────
VCC   ← ESP32 3.3V
GND   ← Common GND
STBY  ← GPIO 4
PWMA  ← GPIO 26
AIN1  ← GPIO 27
AIN2  ← GPIO 14
PWMB  ← GPIO 13
BIN1  ← GPIO 12
BIN2  ← GPIO 23

TB6612 → Motors:
  AO1, AO2 → Motor A
  BO1, BO2 → Motor B
```

## Complete Pin Mapping Table

| Component | Pin | ESP32 GPIO | Power Source |
|-----------|-----|------------|--------------|
| **ESP32** | USB | - | Powerbank USB |
| **PCA9685** | VCC | 3.3V | ESP32 3.3V pin |
| | GND | GND | Common GND |
| | SDA | GPIO 21 | - |
| | SCL | GPIO 22 | - |
| | V+ | - | Powerbank 5V (cut USB) |
| **Servos** | All | Ch 0-3 | PCA9685 V+ terminal |
| **A7670** | VCC | - | Powerbank 5V |
| | TXD | GPIO 16 | - |
| | RXD | GPIO 17 | - |
| **A9G** | VCC | - | Powerbank 5V |
| | TX | GPIO 33 | - |
| | RX | GPIO 32 | - |
| **HC-SR04** | VCC | - | Powerbank 5V |
| | TRIG | GPIO 25 | - |
| | ECHO | GPIO 34 | - |
| **TB6612** | VCC | 3.3V | ESP32 3.3V |
| | VM | - | 18650 7.4V |
| | STBY | GPIO 4 | - |
| | PWMA | GPIO 26 | - |
| | AIN1 | GPIO 27 | - |
| | AIN2 | GPIO 14 | - |
| | PWMB | GPIO 13 | - |
| | BIN1 | GPIO 12 | - |
| | BIN2 | GPIO 23 | - |

## Power Consumption Estimate

### From Powerbank (5V):
| Device | Current | Notes |
|--------|---------|-------|
| ESP32 | 200-500mA | WiFi active |
| Servos (4×) | 200-800mA | Moving |
| A7670 | 100-500mA | Peak 2A when transmitting |
| A9G | 100-300mA | Peak 800mA |
| HC-SR04 | 15mA | Minimal |
| **TOTAL** | **1-3A** | Need 10,000mAh powerbank for ~3-5 hours |

### From Batteries (7.4V):
| Device | Current |
|--------|---------|
| Motors (2×) | 500mA-2A |

## Recommended Powerbank Specs

- **Capacity**: 10,000mAh minimum (20,000mAh better)
- **Output**: 5V 2A minimum (2.1A or 2.4A better)
- **Ports**: 2× USB ports (para multiple connections)

### Magandang Brands:
- Xiaomi Mi Powerbank (~₱500-800)
- Anker (~₱800-1500)
- Romoss (~₱400-600)

## Battery Options for Motors

### Option 1: 2× 18650 (7.4V) ⭐ RECOMMENDED
- **Voltage**: 7.4V (3.7V × 2)
- **Capacity**: 2000-3000mAh
- **Price**: ₱200-300 with holder
- **Where**: Shopee, Lazada, electronics shops

### Option 2: 6× AA Batteries (9V)
- **Voltage**: 9V (1.5V × 6)
- **Capacity**: 2000mAh
- **Price**: ₱100-150
- **Where**: Grocery, hardware

### Option 3: 9V Battery
- **Voltage**: 9V
- **Capacity**: 500mAh (hindi gaano katagal)
- **Price**: ₱50-100
- **Not recommended**: Mabilis maubos

## Shopping List with Prices

| Item | Estimated Price | Where to Buy |
|------|----------------|--------------|
| Powerbank (if wala) | ₱500-800 | SM, Shopee |
| 2× 18650 + holder | ₱200-300 | Shopee, Lazada |
| USB cables (3-4×) | ₱100-200 | Shopee |
| Breadboard | ₱80-150 | Raon, Lazada |
| Jumper wires (40pcs) | ₱50-100 | Raon, Lazada |
| TB6612 module | ₱100-150 | Shopee |
| 2× DC motors | ₱100-200 | Shopee |
| **TOTAL** | **₱630-1100** | (kung may powerbank na) |

## Step-by-Step Assembly

### 1. Prepare USB Cables
```
Cut 2-3 USB cables:
1. Expose red (+5V) and black (GND) wires
2. Strip 5mm of insulation
3. Tin with soldering iron (optional)
```

### 2. Setup Breadboard
```
Breadboard rails:
- Red rail = +5V from powerbank
- Blue rail = GND (common ground)
- Connect all GNDs together!
```

### 3. Connect ESP32
```
1. Plug ESP32 to powerbank USB (intact cable)
2. Note which GPIO pins you'll use
3. Connect ESP32 GND to breadboard GND rail
```

### 4. Connect PCA9685
```
1. Place PCA9685 on breadboard
2. VCC → ESP32 3.3V pin
3. GND → GND rail
4. SDA → GPIO 21
5. SCL → GPIO 22
6. V+ terminal → Powerbank +5V (cut USB)
7. GND terminal → GND rail
```

### 5. Connect Servos
```
For each servo:
1. Brown → PCA9685 GND
2. Red → PCA9685 V+
3. Orange → PCA9685 channel (0-3)
```

### 6. Connect Sensors
```
A7670:
- VCC → Powerbank +5V
- GND → GND rail
- TXD → GPIO 16
- RXD → GPIO 17

A9G:
- VCC → Powerbank +5V
- GND → GND rail
- TX → GPIO 33
- RX → GPIO 32

HC-SR04:
- VCC → Powerbank +5V
- GND → GND rail
- TRIG → GPIO 25
- ECHO → GPIO 34
```

### 7. Connect Motors (Optional)
```
18650 batteries → TB6612 VM
TB6612 connections as per pin table above
```

## Testing Procedure

### Test 1: Power Only
```
✓ Connect powerbank to ESP32
✓ ESP32 LED should light up
✓ Check Serial Monitor (115200 baud)
✓ Should see boot messages
```

### Test 2: Servo Test
```
✓ Upload servo test code
✓ Servos should move to home position
✓ Test each servo individually
```

### Test 3: Sensors
```
✓ Test ultrasonic - should read distance
✓ Test GPS - should get satellite count
✓ Test GSM - should connect to network
```

### Test 4: Full System
```
✓ All components working
✓ Mobile app connects
✓ Can control servos from app
```

## Troubleshooting

### Powerbank Shuts Off
**Problem**: Some powerbanks auto-shutoff with low current
**Solution**: 
- Use "always on" powerbank
- Add small LED to increase current draw
- Get powerbank with "low current mode"

### Servos Jittering
**Problem**: Not enough current
**Solution**:
- Use powerbank with 2A+ output
- Reduce number of servos moving at once
- Add 1000µF capacitor on V+ line

### ESP32 Resets
**Problem**: Voltage drop when servos move
**Solution**:
- Use separate powerbank for servos
- Add capacitors
- Check all connections tight

### Motors Don't Spin
**Problem**: Battery voltage too low
**Solution**:
- Check battery voltage (should be 6V+)
- Replace batteries
- Check TB6612 connections

## Runtime Estimates

### 10,000mAh Powerbank:
- **Idle mode**: 8-10 hours
- **Normal use**: 4-6 hours
- **Heavy use** (servos moving): 2-3 hours

### 2× 18650 (2000mAh):
- **Motors only**: 1-2 hours continuous

## Pro Tips

1. **Use quality powerbank** - cheap ones may not provide stable 5V
2. **Common ground essential** - connect all GNDs together
3. **Test components individually** - easier to debug
4. **Monitor battery levels** - check powerbank % regularly
5. **Keep batteries charged** - have spare 18650s ready

## Upgrades Later

Kung gusto mo mas permanent setup:
1. Get proper 12V power supply (~₱300)
2. Get 5V buck converter (~₱50-100)
3. Solder connections instead of breadboard
4. Add battery monitoring circuit
5. Use 3S LiPo battery (11.1V) for longer runtime

## Need Help?

Check these if may problema:
- ✓ Lahat ba ng GND connected?
- ✓ Tama ba ang pin connections?
- ✓ May power ba ang powerbank?
- ✓ Upload na ba ang firmware?
- ✓ WiFi configured ba sa firmware?
