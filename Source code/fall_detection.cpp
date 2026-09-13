#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ------------------- WIFI + THINGSPEAK -------------------
const char* ssid = "kashif";
const char* password = "kashif1234";
String apiKey = "KUYKSUM66EIZW3YO";
const char* server = "http://api.thingspeak.com/update";

// ----------------------------------------------------------
const int MPU_addr = 0x68;
const int buzzerPin = 23;

// Raw sensor values
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;
float ax, ay, az, gx, gy, gz;

// Offsets
float ACCEL_OFFSET_X = 10.61;
float ACCEL_OFFSET_Y = -0.02;
float ACCEL_OFFSET_Z = -7.23;
float GYRO_OFFSET_X  = -0.06;
float GYRO_OFFSET_Y  = 0.03;
float GYRO_OFFSET_Z  = -0.07;

// Thresholds
#define FREEFALL_THRESHOLD 6.0
#define IMPACT_THRESHOLD   14.0
#define STILL_THRESHOLD    7.0
#define GYRO_MOVEMENT_MIN  10
#define GYRO_MOVEMENT_MAX  300

bool freeFall = false;
bool impact = false;
bool finalCheck = false;

int fallStatus = 0;
bool fallPendingUpload = false;   

unsigned long lastTriggerTime = 0;
unsigned long lastUpload = 0;

// ----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(buzzerPin, OUTPUT);

  // Initialize MPU6050
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
}

// ----------------------------------------------------------
void loop() {

  //  If fall happened, STOP reading MPU and upload immediately
  if (fallPendingUpload) {
    uploadFallEvent();
    return;    // wait until upload completes
  }

  mpu_read();

  // Apply offsets
  ax = (AcX / 16384.0) - (ACCEL_OFFSET_X / 16384.0);
  ay = (AcY / 16384.0) - (ACCEL_OFFSET_Y / 16384.0);
  az = (AcZ / 16384.0) - (ACCEL_OFFSET_Z / 16384.0);

  gx = (GyX / 131.0) - GYRO_OFFSET_X;
  gy = (GyY / 131.0) - GYRO_OFFSET_Y;
  gz = (GyZ / 131.0) - GYRO_OFFSET_Z;

  float accMag = sqrt(ax * ax + ay * ay + az * az) * 10;
  float gyroMag = sqrt(gx * gx + gy * gy + gz * gz);

  Serial.print("Acc = "); Serial.print(accMag);
  Serial.print(" | Gyro = "); Serial.println(gyroMag);

  unsigned long now = millis();
  fallStatus = 0;   // default

  // -------------------- FALL STAGES --------------------
  if (accMag < FREEFALL_THRESHOLD && !freeFall) {
    freeFall = true;
    lastTriggerTime = now;
    Serial.println("FREE FALL DETECTED");
  }

  if (freeFall && !impact && now - lastTriggerTime < 1200) {
    if (accMag > IMPACT_THRESHOLD) {
      impact = true;
      lastTriggerTime = now;
      Serial.println("IMPACT DETECTED");
    }
  }

  if (freeFall && !impact && now - lastTriggerTime >= 1200) {
    freeFall = false;
  }

  if (impact && !finalCheck && now - lastTriggerTime < 1000) {
    if (gyroMag > GYRO_MOVEMENT_MIN && gyroMag < GYRO_MOVEMENT_MAX) {
      finalCheck = true;
      lastTriggerTime = now;
      Serial.println("ORIENTATION CHANGE DETECTED");
    }
  }

  if (impact && !finalCheck && now - lastTriggerTime >= 1000) {
    impact = false;
    freeFall = false;
  }

  // ------------------ FINAL FALL CONFIRM ------------------
  if (finalCheck && accMag < STILL_THRESHOLD) {

    Serial.println(" FALL DETECTED & CONFIRMED!");

    tone(buzzerPin, 2000);
    delay(2000);
    noTone(buzzerPin);

    fallStatus = 1;

    //  STOP system – request immediate upload
    fallPendingUpload = true;
  }

  // ------------------- NORMAL SENDING EVERY 15 SEC -------------------
  if (!fallPendingUpload && (millis() - lastUpload >= 15000)) {
    sendToThingSpeak(accMag, gyroMag, fallStatus);
    lastUpload = millis();
  }

  delay(20);
}

// ----------------------------------------------------------
// UPLOAD REGULAR DATA
void sendToThingSpeak(float accM, float gyroM, int fallFlag) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String(server) +
               "?api_key=" + apiKey +
               "&field1=" + String(accM, 2) +
               "&field2=" + String(gyroM, 2) +
               "&field3=" + String(fallFlag);

  http.begin(url);
  int code = http.GET();
  Serial.print("Normal Upload Response: ");
  Serial.println(code);
  http.end();
}

// ----------------------------------------------------------
//  SPECIAL UPLOAD FOR FALL EVENT
void uploadFallEvent() {
  Serial.println("Uploading FALL EVENT...");

  HTTPClient http;
  String url = String(server) +
               "?api_key=" + apiKey +
               "&field1=0" +       // not needed
               "&field2=0" +       // not needed
               "&field3=1";        // ★ ALWAYS 1

  http.begin(url);
  int code = http.GET();
  Serial.print("Fall Upload Response: ");
  Serial.println(code);
  http.end();

  if (code > 0) {
    Serial.println("Fall uploaded successfully!");

    // reset everything
    freeFall = impact = finalCheck = false;
    fallPendingUpload = false;
    fallStatus = 0;

    delay(3000);  // short pause before enabling MPU again
  }
}

// ----------------------------------------------------------
void mpu_read() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();
  Wire.read(); Wire.read();
  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();
}