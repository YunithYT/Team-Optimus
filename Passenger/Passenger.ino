#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// LCD Configuration (I2C Address: 0x3F, 16x2 Display)
LiquidCrystal_I2C lcd(0x3F, 16, 2);

// GSM Module Configuration
SoftwareSerial SIM900A(2, 3); // RX, TX

// Sensor and Buzzer Pins
const int BOTTOM_SENSOR_PIN = 5;
const int TOP_SENSOR_PIN = 6;
const int BUZZER_PIN = 9;

// Passenger Tracking
int passengerCount = 0;
const int maxCapacity = 3;

// Sensor State Management
bool bottomSensorTriggered = false;
bool topSensorTriggered = false;
unsigned long lastSensorTriggerTime = 0;
const unsigned long sensorTimeout = 1500; // 1.5 seconds

// SMS Sent Flags
bool smsSent = false;
bool serviceStartSmsSent = false;

// Function to Print Text to LCD using For-Loop
void printToLCD(String text, int row) {
  lcd.setCursor(0, row); // Set cursor to start of row
  for (int i = 0; i < text.length(); i++) {
    lcd.print(text[i]); // Print each character one by one
  }
}

// Function to Send SMS
void sendSMS(String message) {
  Serial.println("Sending Message please wait...");
  SIM900A.println("AT+CMGF=1");    // Set SMS to text mode
  delay(1000);
  Serial.println("Set SMS Number");
  SIM900A.println("AT+CMGS=\"+919606873157\"\r"); // Receiver's Mobile Number
  delay(1000);
  Serial.println("Set SMS Content");
  SIM900A.println(message);
  delay(100);
  Serial.println("Done");
  SIM900A.println((char)26); // Send SMS
  delay(1000);
  Serial.println("Message sent successfully");
}

void setup() {
  Serial.begin(9600);
  SIM900A.begin(9600);

  // Initialize sensors and buzzer
  pinMode(BOTTOM_SENSOR_PIN, INPUT);
  pinMode(TOP_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Display Initial Message
  printToLCD("KSRTC KA21 9876", 0);
  printToLCD("Service Started", 1);
  delay(2000); // Display for 2 seconds

  // Send Service Start SMS (Only Once)
  if (!serviceStartSmsSent) {
    sendSMS("KSRTC KA21 9876 HAS STARTED HIS SERVICE");
    serviceStartSmsSent = true; // Prevent multiple SMS
  }

  // Clear LCD and Show Passenger Count
  lcd.clear();
  String initialText = "Passengers: " + String(passengerCount);
  printToLCD(initialText, 0); // Print on first row using for-loop
}

void loop() {
  // Turn off buzzer if under capacity
  if (passengerCount <= maxCapacity) {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // --- Entry Logic (Bottom -> Top Sensor) ---
  if (digitalRead(BOTTOM_SENSOR_PIN) == LOW && !topSensorTriggered) {
    bottomSensorTriggered = true;
    lastSensorTriggerTime = millis();
    delay(100); // Debounce
  }

  if (bottomSensorTriggered && digitalRead(TOP_SENSOR_PIN) == LOW && millis() - lastSensorTriggerTime <= sensorTimeout) {
    delay(100); // Debounce
    passengerCount++;
    Serial.print("Passenger count: ");
    Serial.println(passengerCount);

    // Update LCD
    String passengerText = "Passengers: " + String(passengerCount);
    printToLCD(passengerText, 0); // Print on first row using for-loop

    if (passengerCount > maxCapacity) {
      digitalWrite(BUZZER_PIN, HIGH);
      Serial.println("Bus overloaded!");
      printToLCD("Bus overloaded!", 1); // Print on second row using for-loop

      // Send SMS only if not already sent
      if (!smsSent) {
        sendSMS("Bus is overloaded!! KA21 9623 DURGAMBA MOTORS HAS EXCEEDED ITS LIMITED CAPACITY!!");
        smsSent = true; // Prevent multiple SMS
      }
    }

    // Reset sensors
    bottomSensorTriggered = false;
    lastSensorTriggerTime = 0;
  }

  // --- Exit Logic (Top -> Bottom Sensor) ---
  if (digitalRead(TOP_SENSOR_PIN) == LOW && !bottomSensorTriggered) {
    topSensorTriggered = true;
    lastSensorTriggerTime = millis();
    delay(100); // Debounce
  }

  if (topSensorTriggered && digitalRead(BOTTOM_SENSOR_PIN) == LOW && millis() - lastSensorTriggerTime <= sensorTimeout) {
    delay(100); // Debounce
    passengerCount--;
    if (passengerCount < 0) passengerCount = 0; // Prevent negative count
    Serial.print("Passenger count: ");
    Serial.println(passengerCount);

    // Update LCD
    String passengerText = "Passengers: " + String(passengerCount);
    printToLCD(passengerText, 0); // Print on first row using for-loop

    if (passengerCount <= maxCapacity) {
      printToLCD("                ", 1); // Clear second row
      smsSent = false; // Reset SMS flag when capacity is back to normal
    }

    // Reset sensors
    topSensorTriggered = false;
    lastSensorTriggerTime = 0;
  }

  // Reset sensor sequence if timeout occurs
  if (millis() - lastSensorTriggerTime > sensorTimeout) {
    bottomSensorTriggered = false;
    topSensorTriggered = false;
    lastSensorTriggerTime = 0;
  }
}