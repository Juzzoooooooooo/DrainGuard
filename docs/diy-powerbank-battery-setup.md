# DIY Power Supply: Powerbank Board + Cellphone Battery

Guide kung paano gumawa ng custom power supply using powerbank PCB at cellphone battery.

## Bakit Maganda To?

### Advantages:
- ✅ **Mura** - gamitin lumang powerbank/phone battery
- ✅ **Compact** - mas maliit kaysa full powerbank
- ✅ **Regulated 5V output** - may built-in step-up circuit
- ✅ **USB output** - pwede multiple devices
- ✅ **May protection** - overcharge, overdischarge, short circuit

### Typical Powerbank Board Features:
```
Cellphone Battery (3.7V, 2000-4000mAh)
         ↓
   Powerbank PCB
   ├─> Step-up to 5V (boost converter)
   ├─> USB output (5V 1-2A)
   ├─> Battery protection circuit
   ├─> Charging circuit (micro USB input)
   └─> LED indicators
```

---

## Kailangan Mo

### Materials:
1. **Powerbank PCB board** 
   - From old/broken powerbank
   - Or buy new (~₱50-150 sa Shopee/Lazada)
   - Hanap: "powerbank circuit board" or "5V boost module with charging"

2. **Cellphone Battery (3.7V Li-ion/LiPo)**
   - From old phone
   - Or buy new (~₱150-300)
   - Capacity: 2000-4000mAh recommended
   - **Important**: Must have protection circuit

3. **Tools**:
   - Soldering iron + solder
   - Wire cutters
   - Multimeter (para check voltage)
   - Heat shrink tubing or electrical tape

---

## Powerbank Board Pinout

Typical powerbank board may ganito:
```
┌─────────────────────────────────┐
│     POWERBANK PCB BOARD         │
│                                 │
│  [B+] [B-]  ← Battery terminals │
│                                 │
│  [Micro USB] ← Charging port    │
│                                 │
│  [USB OUT] ← 5V output          │
│                                 │
│  [LEDs] ← Battery indicators    │
└─────────────────────────────────┘
```

### Connections:
| Terminal | Connection | Notes |
|----------|------------|-------|
| B+ | Battery positive (red) | 3.7V input |
| B- | Battery negative (black) | Ground |
| USB OUT | 5V output | For devices |
| Micro USB | Charging input | 5V charger |

---

## Step-by-Step Assembly

### Step 1: Extract Powerbank Board

**Kung may lumang powerbank:**
```
1. Buksan ang casing (careful with clips)
2. Disconnect battery (tanggalin wires)
3. Kunan ang PCB board
4. Check kung okay pa (no burns, cracks)
```

**Test Board:**
```
1. Connect cellphone charger sa micro USB
2. Measure USB output with multimeter
3. Should read: 5V ± 0.2V
4. If okay, ready to use!
```

### Step 2: Prepare Cellphone Battery

**⚠️ IMPORTANT SAFETY:**
```
❌ Huwag i-puncture ang battery
❌ Huwag i-short ang terminals
❌ Huwag i-overheat
❌ Check for swelling (puffed = BASURAIN NA)
✅ Handle with care
✅ Keep away from metal objects
```

**Battery Terminals:**
```
Typical cellphone battery:
┌────────────────┐
│    3.7V LION   │
│   2000-4000mAh │
│                │
│  [+] [-] [T]   │ ← 3 or 4 terminals
└────────────────┘

Terminals:
+ = Positive (red wire)
- = Negative (black wire)
T = Temperature sensor (ignore, or cut)
```

**Kung may 3rd/4th terminal:**
- Usually temperature sensor
- Pwede mo i-ignore or i-tape
- Gamitin lang yung + at -

### Step 3: Connect Battery to Board

**Soldering:**
```
Battery → Powerbank Board
  (+) red   → B+ (positive terminal)
  (-) black → B- (negative terminal)

Tips:
1. Use 20-22 AWG wire (kung papalitan)
2. Solder firmly (good connection)
3. Add heat shrink tubing
4. Double check polarity (+ to +, - to -)
```

**Wire Gauge:**
```
For 2A current:
- 22 AWG wire = OK
- 20 AWG wire = Better
- 18 AWG wire = Best (pero thick)
```

### Step 4: Test Assembly

**Before connecting anything:**
```
1. Visual inspection
   ✓ No exposed wires
   ✓ No shorts
   ✓ Polarity correct

2. Charge test
   ✓ Plug charger to micro USB
   ✓ LED should light up
   ✓ Battery charging (warm, not hot)

3. Output test
   ✓ Measure USB output voltage
   ✓ Should be 5V ± 0.2V
   ✓ Test with small load (LED, phone)
```

### Step 5: Mount Everything

**Enclosure Options:**

**Option A - Simple (Tape/Zip ties):**
```
1. Place board on battery (insulated side)
2. Wrap with electrical tape
3. Expose USB port at edge
4. Secure with zip ties
```

**Option B - 3D Print/Case:**
```
1. Design/find case for battery size
2. Mount board with screws/glue
3. Add USB port hole
4. Add charging port hole
```

**Option C - Project Box:**
```
1. Buy small plastic box (~₱20-50)
2. Drill holes for USB ports
3. Mount with double-sided tape
4. Secure with hot glue (not on battery!)
```

---

## Wiring to Your Project

### Main Power Distribution:

```
CELLPHONE BATTERY (3.7V 3000mAh)
         ↓
   POWERBANK BOARD
         ↓
     USB OUTPUT (5V 2A)
         ↓
    ┌────┴────┬─────────┬──────────┐
    │         │         │          │
  ESP32   PCA9685   A7670       A9G
  (USB)     V+       VCC        VCC
    │         │         │          │
    └─────────┴─────────┴──────────┘
           Common GND
```

### Multiple USB Outputs:

Kung may 2 USB ports ang board:
```
USB Port 1 → ESP32 (intact cable)
USB Port 2 → Cut cable
              ├─> Red (+5V) → PCA9685 V+
              ├─> Red (+5V) → A7670 VCC
              ├─> Red (+5V) → A9G VCC
              └─> Black (GND) → Common GND
```

Kung 1 USB port lang:
```
USB Port → USB hub (4 ports, ~₱100-200)
          ├─> ESP32
          ├─> Cut cable for PCA9685
          ├─> Cut cable for A7670
          └─> Cut cable for A9G
```

---

## Battery Capacity Planning

### Calculate Runtime:

**Your System Current Draw:**
| Component | Current | Notes |
|-----------|---------|-------|
| ESP32 | 200-500mA | WiFi active |
| 4× Servos | 200-800mA | Moving |
| A7670 | 100-500mA | Peak 2A |
| A9G | 100-300mA | Peak 800mA |
| HC-SR04 | 15mA | Minimal |
| **Average** | **1-2A** | Typical use |

**Runtime Formula:**
```
Runtime (hours) = Battery Capacity (mAh) / Average Current (mA) × 0.8

Example with 3000mAh battery:
Runtime = 3000 / 1500 × 0.8 = 1.6 hours

Para mas matagal:
- Use 4000mAh battery = 2 hours+
- Use multiple batteries in parallel = 2× capacity
```

### Battery Options:

| Battery Source | Voltage | Capacity | Price |
|----------------|---------|----------|-------|
| Old smartphone | 3.7V | 2000-3000mAh | Free |
| Samsung Note battery | 3.7V | 3000-4000mAh | ₱200-400 |
| Generic 18650 | 3.7V | 2000-3500mAh | ₱100-150 ea |
| Tablet battery | 3.7V | 5000-8000mAh | ₱300-600 |

---

## Safety Considerations

### ⚠️ Li-ion Battery Safety:

**DO:**
- ✅ Use batteries with protection circuit
- ✅ Charge with proper 5V charger (1-2A)
- ✅ Stop using if battery swells
- ✅ Store in cool, dry place
- ✅ Insulate terminals to prevent shorts
- ✅ Monitor temperature during charging

**DON'T:**
- ❌ Puncture or crush battery
- ❌ Short circuit terminals
- ❌ Overcharge (>4.2V)
- ❌ Over-discharge (<3.0V)
- ❌ Expose to heat/fire
- ❌ Use swollen/damaged batteries

### Signs of Problem:

```
⚠️ STOP USING IF:
- Battery is swollen/puffy
- Battery is hot (>45°C)
- Strange smell
- Hissing sound
- Leaking
- Physical damage

→ Safely dispose at proper facility
```

### Fire Safety:

Keep nearby:
- Fire extinguisher or sand
- Metal container (for emergency containment)
- Don't leave charging unattended

---

## Charging Your DIY Power Supply

### Charging Setup:
```
Phone Charger (5V 1-2A)
    ↓
Micro USB cable
    ↓
Powerbank Board Micro USB Port
    ↓
Battery charges (3.7V → 4.2V full)

Typical charge time:
- 2000mAh battery = 2-3 hours
- 3000mAh battery = 3-4 hours
- 4000mAh battery = 4-5 hours
```

### LED Indicators (typical):
```
Red LED = Charging
Green LED = Fully charged
Blue LED (optional) = Discharging

Some boards:
4 LEDs = Battery level indicator
Flashing = Low battery
```

---

## Upgrading: Multiple Batteries

### Parallel Connection (More Capacity):

```
Battery 1 (3.7V 3000mAh)
  (+) ───┬──→ B+
  (-) ───┼──→ B-
         │
Battery 2 (3.7V 3000mAh)
  (+) ───┘
  (-) ───┘

Result: 3.7V 6000mAh (double runtime)
```

**⚠️ Important for Parallel:**
- Use same voltage batteries (both 3.7V)
- Use same capacity (or close)
- Same chemistry (both Li-ion)
- Same charge level before connecting

---

## Advanced: Adding Switch

### Power Switch:
```
Battery (+) → Switch → Board B+
Battery (-) → Direct → Board B-

Benefits:
- Turn off completely when not in use
- Save battery when stored
- Emergency shutoff
```

### Recommended Switch:
- Toggle switch or rocker switch
- Rating: 3A or higher
- Small, panel mount type
- Price: ₱20-50

---

## Troubleshooting

### Board Not Working:
```
Problem: No output
Check:
✓ Battery voltage (should be 3.7-4.2V)
✓ Battery connection (soldering OK?)
✓ Board LEDs (lighting up?)
✓ Fuse on board (not blown?)
```

### Low Output Voltage:
```
Problem: USB output <4.8V
Cause:
- Battery low (<3.5V)
- Board faulty
- Overloaded (too much current)

Solution:
- Charge battery fully
- Reduce load
- Replace board if damaged
```

### Battery Not Charging:
```
Problem: Red LED not lighting
Check:
✓ Charger working? (test with phone)
✓ Micro USB cable OK?
✓ Battery terminals connected?
✓ Battery not dead? (>2.5V)
```

### Overheating:
```
Problem: Board/battery hot
Cause:
- Too much current draw
- Short circuit
- Bad battery

Solution:
- Disconnect immediately
- Let cool down
- Reduce load or use bigger battery
```

---

## Cost Breakdown

### If Buying New:

| Item | Price |
|------|-------|
| Powerbank board | ₱50-150 |
| Cellphone battery (3000mAh) | ₱200-300 |
| Wires, solder, tape | ₱50-100 |
| Enclosure/box | ₱20-100 |
| **TOTAL** | **₱320-650** |

### If Using Old Parts:

| Item | Price |
|------|-------|
| Old powerbank (salvage board) | Free |
| Old phone battery | Free |
| Wires, solder | ₱50 |
| **TOTAL** | **₱50 or FREE!** |

---

## Recommended Powerbank Boards

### Where to Buy:

**Shopee/Lazada Search:**
- "Powerbank PCB board"
- "TP4056 + boost converter"
- "5V powerbank circuit"
- "lithium battery charger module"

**Good Models:**
1. **5V 2A Boost Module with TP4056**
   - Price: ₱50-100
   - Features: Charging + boost + protection
   
2. **Dual USB Powerbank Board**
   - Price: ₱100-150
   - Features: 2× USB outputs, LED indicators

3. **Complete Powerbank Kit**
   - Price: ₱150-250
   - Includes: Board + case + batteries

---

## Pro Tips

1. **Test everything before final assembly**
   - Saves time troubleshooting later

2. **Label polarity clearly**
   - Mark + and - to avoid mistakes

3. **Use quality wire and solder**
   - Prevents loose connections

4. **Add fuse (optional)**
   - 3A fuse between battery and board
   - Extra protection

5. **Monitor first few charges**
   - Check temperature
   - Check charge time
   - Make sure working properly

6. **Keep battery level above 20%**
   - Prolongs battery life
   - Prevents over-discharge

---

## Next Steps

After you build this power supply:

1. **Test with small load first** (LED, phone charging)
2. **Measure output voltage and current**
3. **Connect to your ESP32 project**
4. **Monitor during operation**
5. **Enjoy portable power!** 🎉

**Questions? May problema? Just ask!** 👍
