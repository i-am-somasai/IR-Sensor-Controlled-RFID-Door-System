🚪 IR-Sensor-Controlled-RFID-Door-System:
This project demonstrates a smart, wireless door control system using two ESP32 boards communicating via ESP-NOW. It integrates infrared (IR) sensors, an RFID reader, and a servo motor to control access securely. The system enables or disables RFID-based access dynamically based on object presence detection.

🔧 Components Used
2 × ESP32 development boards
1 × MFRC522 RFID Reader
2 × IR Sensors (entry and exit detection)
1 × Servo Motor
Jumper wires, breadboard
Power supply (e.g., USB or 5V regulated)

🧠 System Architecture

🅰️ Sender ESP32 (IR Sensor Node)
Monitors a single IR sensor (placed outside the door).
Detects object presence.
Sends status ("Object Detected" or "No Object") to the receiver via ESP-NOW.

🅱️ Receiver ESP32 (Main Controller)
Controls access via RFID authentication.
Manages two IR sensors:
IR1 (Outside) – Activates RFID on presence detection.
IR2 (Inside) – Confirms entry or detects exit.
Verifies RFID tag against an authorized list.
Controls a servo motor to open/close the door.
Implements auto-close logic and anti-theft safety by disabling access if the object disappears unexpectedly.

⚙️ How to Use

1. Setup Sender ESP32 (IR Sensor Node)
Upload SenderCode.ino.
Connect an IR sensor to GPIO2.
Replace receiverMAC with the MAC address of the receiver ESP32.

2. Setup Receiver ESP32 (Main Controller)
Upload ReceiverCode.ino.

Wire components:
IR1 → GPIO2
IR2 → GPIO4
RFID (MFRC522) → SPI + SS (GPIO5), RST (GPIO27)
Servo → GPIO15
RFID power control → GPIO26

3. Power Both ESP32 Boards
Monitor serial output for real-time access decisions and status logs.

✅ Features
🔒 RFID-based secure access
🧠 Intelligent object detection to reduce false access triggers
🔀 ESP-NOW communication for fast, Wi-Fi-free control
🔁 Auto-close timeout and motion-based door control
⚠️ Anti-theft logic when the object is removed improperly
⚙️ Modular and easily extensible
