#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <MPU6050.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "Timo iPhone";
const char* password = "justin123";

// Server details - USE YOUR ACTUAL SERVER IP
const char* server_url = "http://172.20.10.2:5005/api/sensor";

// MPU6050 sensor
MPU6050 mpu;

// Timer for sending data
unsigned long previousMillis = 0;
const long interval = 200;  // send data every 200ms (slightly slower to be gentle on the network)

void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Initialize MPU6050
  Wire.begin();
  mpu.initialize();
  
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }
  Serial.println("MPU6050 initialized");
  
  Serial.print("Server URL: ");
  Serial.println(server_url);
}

void loop() {
  // Only proceed if WiFi is still connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    WiFi.begin(ssid, password);
    delay(5000);
    return;
  }
  
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Read sensor data from MPU6050
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    // Create JSON document
    StaticJsonDocument<256> doc;
    
    // Convert to proper units and add to JSON
    // Accelerometer data (convert to g's)
    doc["accel_x"] = ax / 16384.0; // For +/- 2g range
    doc["accel_y"] = ay / 16384.0;
    doc["accel_z"] = az / 16384.0;
    
    // Gyroscope data (convert to degrees/sec)
    doc["gyro_x"] = gx / 131.0; // For +/- 250 degrees/sec range
    doc["gyro_y"] = gy / 131.0;
    doc["gyro_z"] = gz / 131.0;
    
    // Add magnetometer placeholder data
    doc["mag_x"] = 0;
    doc["mag_y"] = 0;
    doc["mag_z"] = 0;
    
    // Add timestamp
    doc["timestamp"] = currentMillis;
    
    // Serialize JSON to string
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Send data via HTTP POST
    HTTPClient http;
    http.begin(server_url);
    http.addHeader("Content-Type", "application/json");
    
    int httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      // Only print response for debugging, could be removed in production
      Serial.print("Response: ");
      Serial.println(response);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
    
    // Debug output
    Serial.print("Sent: ");
    Serial.println(jsonString);
  }
}