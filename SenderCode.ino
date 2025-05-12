#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <esp_now.h>

// === Pin Definitions ===
#define IR_SENSOR1_PIN 2    // Infrared sensor 1 for entry detection (outside)
#define IR_SENSOR2_PIN 4    // Infrared sensor 2 for exit detection (inside)
#define RFID_POWER_PIN 26   // Controls power to the RFID reader
#define SS_PIN 5            // SPI SS (Slave Select) pin for RFID
#define RST_PIN 27          // Reset pin for RFID reader
#define SERVO_PIN 15        // Servo motor control pin

// Create MFRC522 RFID reader and Servo objects
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo myServo;

// === Global State Variables ===
bool rfidPowered = false;  // Tracks RFID power status
bool doorOpened = false;   // Tracks whether the door is open
unsigned long doorOpenTime = 0;  // Records the time the door was opened
bool accessEnabled = true;  // Flag for access control (managed via ESP-NOW)

// === Authorized RFID Tags ===
byte allowedUIDs[][4] = {
  {0x6F, 0x51, 0x51, 0x1F},  // Authorized RFID tag 1
  {0x9A, 0xD4, 0x78, 0x00}   // Authorized RFID tag 2
};

// Check if scanned UID matches any allowed UID
bool isAuthorizedUID(byte *uid, byte size) {
  for (int i = 0; i < sizeof(allowedUIDs) / sizeof(allowedUIDs[0]); i++) {
    bool match = true;
    for (int j = 0; j < size; j++) {
      if (allowedUIDs[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

// Function to open the door by moving the servo to the open position
void openDoor() {
  myServo.write(150);   // Set servo to 150° (open)
  doorOpened = true;
  doorOpenTime = millis();  // Record the time door is opened
  Serial.println("Door OPENED (150°)");
}

// Function to close the door by moving the servo to the closed position
void closeDoor() {
  myServo.write(0);  // Set servo to 0° (closed)
  doorOpened = false;
  Serial.println("Door CLOSED (0°)");
}

// === ESP-NOW Receive Callback ===
// Called when data is received via ESP-NOW
void onReceiveData(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  char msg[len + 1];
  memcpy(msg, data, len);  // Copy received data to a char array
  msg[len] = '\0';  // Null-terminate the string
  Serial.print("ESP-NOW Received: ");
  Serial.println(msg);

  // Handle received commands
  if (strcmp(msg, "no_object") == 0) {
    if (accessEnabled) {
      accessEnabled = false;  // Disable access
      Serial.println("Access DISABLED - Door locked");
      if (doorOpened) {
        Serial.println("Force closing due to no object...");
        closeDoor();
      }
    }
  } else if (strcmp(msg, "object_detected") == 0) {
    if (!accessEnabled) {
      accessEnabled = true;  // Enable access
      Serial.println("Access ENABLED - Door operational");
    }
  }
}

void setup() {
  Serial.begin(115200);  // Initialize serial communication

  // Initialize hardware components
  pinMode(IR_SENSOR1_PIN, INPUT_PULLUP);  // Entry sensor as input
  pinMode(IR_SENSOR2_PIN, INPUT_PULLUP);  // Exit sensor as input
  pinMode(RFID_POWER_PIN, OUTPUT);        // RFID power control
  digitalWrite(RFID_POWER_PIN, LOW);      // Turn off RFID initially

  // Setup the servo motor
  myServo.setPeriodHertz(50);             // Set servo frequency to 50 Hz
  myServo.attach(SERVO_PIN, 500, 2400);   // Attach servo to the defined pin
  closeDoor();  // Start with door in closed position

  // Initialize RFID reader
  SPI.begin(18, 19, 23, SS_PIN);
  mfrc522.PCD_Init();

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed!");
    while (1);  // Halt the system if initialization fails
  }
  esp_now_register_recv_cb(onReceiveData);
  Serial.println("System Ready");
}

void loop() {
  // Close the door if access is disabled
  if (!accessEnabled && doorOpened) {
    Serial.println("Access disabled & door is still open – closing now");
    closeDoor();
    delay(100);
    return;
  }

  // === Entry Detection (IR1) ===
  if (digitalRead(IR_SENSOR1_PIN) == LOW && !doorOpened) {
    Serial.println("Entry detected - Activating RFID");
    digitalWrite(RFID_POWER_PIN, HIGH);  // Power on the RFID reader
    delay(500);  // Wait for the RFID to initialize

    // RFID Tag Reading
    bool tagRead = false;
    unsigned long timeout = millis() + 3000;
    while (millis() < timeout) {
      if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        tagRead = true;
        Serial.print("RFID UID:");
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          Serial.print(" ");
          Serial.print(mfrc522.uid.uidByte[i], HEX);
        }

        // Check authorization
        if (isAuthorizedUID(mfrc522.uid.uidByte, mfrc522.uid.size)) {
          Serial.println(" - ACCESS GRANTED");
          openDoor();

          // Wait for inside sensor trigger or timeout
          unsigned long waitTime = millis() + 5000;
          while (millis() < waitTime) {
            if (digitalRead(IR_SENSOR2_PIN) == LOW) {
              Serial.println("Person entered");
              break;
            }
            delay(50);
          }
        } else {
          Serial.println(" - ACCESS DENIED");
        }
        mfrc522.PICC_HaltA();
        break;
      }
      delay(50);
    }
    digitalWrite(RFID_POWER_PIN, LOW);  // Turn off RFID reader
    if (!tagRead) Serial.println("No RFID tag detected");
  }

  // === Exit Detection (IR2) ===
  if (digitalRead(IR_SENSOR2_PIN) == LOW) {
    if (doorOpened) {
      Serial.println("Exit detected - closing door");
      closeDoor();
    } else {
      Serial.println("Inside motion - opening door");
      openDoor();
      delay(3000);
      closeDoor();
    }
  }

  // Auto-close after timeout
  if (doorOpened && (millis() - doorOpenTime > 5000)) {
    Serial.println("Auto-closing door after timeout");
    closeDoor();
  }

  delay(50);  // Short delay for loop iteration
}
