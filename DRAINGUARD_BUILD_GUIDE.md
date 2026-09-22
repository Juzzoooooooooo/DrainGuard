DrainGuard Robot Prototype - Build Guide


Tools Required

- Mini drill with drill bits (for acrylic holes)
- Soldering iron and solder wire
- Phillips screwdriver
- Flat screwdriver
- Hot glue gun and glue sticks
- Wire stripper and cutter


Materials

- Acrylic sheet (casing panels)
- Hinges (for access panel and lid)
- Phillips screws and nuts
- Jumper wires
- ESP32 DevKit V1
- PCA9685 PWM Servo Driver
- TB6612FNG Motor Driver
- 4 positional arm servos; the base moves in small 10 ms steps
- 2x DC Motors and wheels
- Ultrasonic Sensor HC-SR04
- 1000uF 16V Capacitor x2 (motor and servo power)
- Power bank (5V, dual USB)
- USB cables x2


Step 1 - Design the Casing in Blender

1. Open Blender and create a new project
2. Model the main body box using dimensions that fit all components
3. Model the following separate parts:
   - Base panel with holes for motor shafts and component mounting
   - Side panels with motor shaft openings
   - Front panel with cutout for ultrasonic sensor
   - Back panel with USB access hole for power bank
   - Top lid panel that opens with hinges
   - Robot arm brackets for each servo joint (base, shoulder, elbow, gripper)
4. Add holes in the models where screws will go
5. Export each part as STL or keep as reference for cutting the acrylic


Step 2 - Prepare the Acrylic Casing

1. Use your Blender model as reference for measurements
2. Mark hole positions on the acrylic sheets based on your 3D design for:
   - ESP32 mounting holes
   - PCA9685 mounting holes
   - TB6612FNG mounting holes
   - Motor shaft holes on side panels
   - Ultrasonic sensor cutout on front panel
   - USB access hole for power bank
   - Hinge mounting holes on lid and back panel
3. Use the mini drill to drill all marked holes
   - Use a small bit first then widen to correct size
   - Go slow on acrylic because it cracks if you rush
4. Drill hinge holes on the top panel and back panel for the lid


Step 3 - Solder All Pins

1. PCA9685 - solder all header pins on both sides
2. TB6612FNG - solder all header pins
3. ESP32 - if not pre-soldered, solder header pins
4. Ultrasonic sensor - solder 4-pin header

Tip: Use a breadboard to hold components upright while soldering


Step 4 - Mount Components to Acrylic

1. Place ESP32, PCA9685, and TB6612FNG on the base acrylic panel
2. Secure with Phillips screws and nuts through the drilled holes
3. Mount DC motors on the side panels and secure with screws
4. Mount ultrasonic sensor on the front panel and secure with screws
5. Apply hot glue on all corners of mounted components for extra stability


Step 5 - Wire the Electronics

Power Wiring

    Power Bank USB 1 goes to ESP32 via USB cable
    Power Bank USB 2 goes to TB6612FNG VM pin and PCA9685 V+ pin
    ESP32 3.3V goes to TB6612FNG VCC and PCA9685 VCC for logic only
    All GND connect to common ground

Capacitors for brownout protection

    Capacitor 1 positive leg to TB6612FNG VM pin, negative leg to GND
    Capacitor 2 positive leg to PCA9685 V+ pin, negative leg to GND

Motor Driver TB6612FNG to ESP32

    AIN1 to GPIO 27
    AIN2 to GPIO 14
    BIN1 to GPIO 12
    BIN2 to GPIO 23
    PWMA to GPIO 26
    PWMB to GPIO 13
    STBY to GPIO 4

Servo Driver PCA9685 to ESP32

    SDA to GPIO 21
    SCL to GPIO 22

Ultrasonic Sensor

    TRIG to GPIO 25
    ECHO to GPIO 34
    VCC to 3.3V
    GND to GND

Servos on PCA9685 channels

    Channel 0 - Base servo
    Channel 1 - Shoulder servo
    Channel 2 - Elbow servo
    Channel 3 - Gripper servo


Step 6 - Assemble the Robot Arm

1. Use your Blender arm bracket designs as reference for assembly
2. Attach servos to arm brackets in order: base, shoulder, elbow, gripper
3. Route servo wires carefully to avoid tangling
4. Connect servo connectors to PCA9685 channels 0 to 3
5. Apply hot glue on servo mounting points for stability
6. Make sure wires have enough slack for full range of motion


Step 7 - Attach Wheels and Motors

1. Press fit or screw wheels onto DC motor shafts
2. Connect motor wires to TB6612FNG outputs
   - Motor A left to A01 and A02
   - Motor B right to B01 and B02
3. Apply hot glue on motor brackets to prevent movement


Step 8 - Assemble the Acrylic Casing

1. Attach side panels to base panel using Phillips screws
2. Attach front and back panels
3. Apply hot glue on all inner corners of the casing for rigidity
4. Attach hinges to lid and top panel using flat screwdriver and screws
5. Ensure lid opens cleanly for access to electronics


Step 9 - Final Assembly and Routing

1. Route all wires neatly inside the casing
2. Use cable ties or hot glue to secure wire bundles
3. Place power bank inside casing or mount externally
4. Thread USB cables through the USB access hole
5. Close the lid and check hinge movement


Step 10 - Upload Firmware

1. Open Arduino IDE
2. Install required libraries:
   - Adafruit PWM Servo Driver
   - ArduinoJson
   - WebSockets by Markus Sattler
3. Open firmware/DrainGuard/DrainGuard.ino
4. Select board: ESP32 Dev Module
5. Select correct COM port
6. Click Upload


Step 11 - Test

1. Power on via power bank
2. Connect phone to WiFi: DrainGuard-Robot, password: DrainGuard123
3. Open the DrainGuard app
4. Test each function:
   - Forward and Backward by holding the button
   - Left and Right turns by tapping
   - Press each BASE, SHOULDER, ELBOW, and GRIPPER direction button separately. Every joint moves one PWM count every 10 ms for at most 200 ms per press; releasing the button stops the position change.
   - Open and Close arm sequence
   - Dashboard water level reading


Troubleshooting

Problem: WiFi drops when motor runs
Solution: Check capacitor on TB6612FNG VM pin

Problem: Servo brownout
Solution: Check capacitor on PCA9685 V+ pin

Problem: Servo goes wrong direction
Solution: Swap fwd and rev in firmware

Problem: App cannot connect
Solution: Make sure phone is on DrainGuard-Robot WiFi

Problem: Arm rotates on boot
Solution: Check the loaded firmware and PCA9685 wiring. Current firmware leaves the servo outputs off at startup and moves the arm only after a control command.
