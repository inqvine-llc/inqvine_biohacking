#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include "SparkFun_BNO080_Arduino_Library.h"
#include <ArduinoJson.h> 

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

BNO080 myIMU;  // IMU sensor object
boolean isIMU = false;  // Flag to check if IMU is initialized

int led1 = 4, led2 = 15;  // Pin numbers for LEDs
int but1 = 12, but2 = 13;  // Pin numbers for buttons

int ch[] = { 36, 39, 32, 34, 35, 33 };  // Pin layout
const char* ch_names[] = { "EMG", "ECG", "EEG", "EMG", "Other", "Other" };  // Names for channels
const char* ch_descriptions[] = { 
  "Electromyography (EMG) channel", 
  "Electrocardiography (ECG) channel", 
  "Electroencephalography (EEG) channel", 
  "Electromyography (EMG) channel", 
  "Other sensor", 
  "Other sensor" 
};

const char* ch_scientific[] = {
  "Muscle electrical activity measurement",
  "Heart electrical activity measurement",
  "Brain electrical activity measurement",
  "Muscle electrical activity measurement",
  "Other sensor measurement",
  "Other sensor measurement"
};

int ch_on[] = { 1, 1, 0, 1, 0, 0 };  // Initial configuration with channels enabled
unsigned int val[] = { 0, 0, 0, 0, 0, 0 };  // Values read from the channels
unsigned long t = 0;  // Time variable for sampling
int freq = 10, tsample = 10;  // Frequency and sample time variables
int acc = 0;  // Acceleration flag (if used)

union udata {
  float f;
  unsigned long ul;
  long l;
  int i;
  byte b[4];
};

udata cmd;

// Function declaration
void set_datarate(int freq);

class E3K3ConnectionCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Client connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client disconnected");
    }
};

class E3K3CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue().c_str();

      if (rxValue.length() > 0) {
        cmd.l = strtol(rxValue.c_str(), NULL, 10);
        Serial.print("Received Command: ");
        Serial.println(cmd.l);

        for (int i = 0; i < 6; i++) {
          ch_on[i] = cmd.b[0] & (0b1 << i);
          if (ch_on[i]) {
            Serial.print("Channel ");
            Serial.print(ch[i]);
            Serial.println(" enabled");
          } else {
            Serial.print("Channel ");
            Serial.print(ch[i]);
            Serial.println(" disabled");
          }
        }

        int freq = int(cmd.b[1]);
        switch (freq) {
          case 1:
            set_datarate(10);
            break;
          case 2:
            set_datarate(100);
            break;
          case 3:
            set_datarate(500);
            break;
          case 4:
            set_datarate(1000);
            break;
          default:
            set_datarate(10);
            break;
        }
      }
    }
};

void set_datarate(int freq) {
  tsample = 1000.0 / freq;
  Serial.print(" Data Rate: ");
  Serial.print(freq);
  Serial.print("Hz, Delay: ");
  Serial.print(tsample);
  Serial.println("ms");
}

void setup() {
  Serial.begin(115200);
  
  // Create the BLE Device
  BLEDevice::init("E3K_3");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new E3K3ConnectionCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristic->setCallbacks(new E3K3CharacteristicCallbacks());

  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("BLE server started. Waiting for connections...");

  // Print the Bluetooth MAC address
  Serial.print("Bluetooth MAC address: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());

  tsample = 1000.0 / freq;  // Calculate time delay
  Serial.print("Initial sampling rate set to: ");
  Serial.print(freq);
  Serial.println("Hz");

  // Initialize LEDs and buttons
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(but1, INPUT);
  pinMode(but2, INPUT);

  // Initialize IMU
  Wire.begin();
  isIMU = myIMU.begin();
  if (isIMU == false) {
    Serial.println("BNO080 not detected at default I2C address. IMU not connected or check your jumpers and the hookup guide.");
    Serial.println("Continuing....");
  } else {
    Wire.setClock(400000); // Increase I2C data rate to 400kHz
    myIMU.enableRotationVector(50); // Send data update every 50ms
    Serial.println(F("IMU enabled"));
    Serial.println(F("Rotation vector enabled"));
    Serial.println(F("Output in form i, j, k, real, accuracy"));
  }
}

void loop() {
  static unsigned long lastPrintTime = 0;
  static unsigned long packetCount = 0;

  if (deviceConnected && (millis() - t) > tsample) {
    t = millis();
    packetCount++;

    // Create a JSON document
    StaticJsonDocument<512> jsonDoc;
    JsonObject jsonData = jsonDoc.to<JsonObject>();
    jsonData["time"] = t;
    jsonData["packet"] = packetCount;

    // Add IMU data if available
    if (isIMU == true && myIMU.dataAvailable() == true) {
      float roll = (myIMU.getRoll()) * 180.0 / PI; // Convert roll to degrees
      float pitch = (myIMU.getPitch()) * 180.0 / PI; // Convert pitch to degrees
      float yaw = (myIMU.getYaw()) * 180.0 / PI; // Convert yaw / heading to degrees

      JsonObject imuData = jsonData.createNestedObject("IMU");
      imuData["roll"] = roll;
      imuData["pitch"] = pitch;
      imuData["yaw"] = yaw;
    }

    // Add analog sensor data
    JsonArray sensorData = jsonData.createNestedArray("sensors");
    for (int i = 0; i < 6; i++) {
      if (ch_on[i]) {
        val[i] = analogRead(ch[i]);
        
        JsonObject sensor = sensorData.createNestedObject();
        sensor["channel"] = ch[i];
        sensor["name"] = ch_names[i];
        sensor["value"] = val[i];
      }
    }

    // Serialize JSON to string
    String jsonString;
    serializeJson(jsonDoc, jsonString);

    // Send JSON via BLE
    pCharacteristic->setValue(jsonString.c_str());
    pCharacteristic->notify();
    
    // Print JSON to Serial for debugging (every second)
    if (millis() - lastPrintTime > 1000) {
      Serial.println("Sending BLE data:");
      Serial.println(jsonString);
      Serial.print("Packets sent in the last second: ");
      Serial.println(packetCount);
      lastPrintTime = millis();
      packetCount = 0;
    }
  }

  // Check connection status every second
  static unsigned long lastConnectionCheck = 0;
  if (millis() - lastConnectionCheck > 1000) {
    Serial.print("BLE Connection status: ");
    Serial.println(deviceConnected ? "Connected" : "Disconnected");
    lastConnectionCheck = millis();
  }

  ///////////-----Button and LEDs
  digitalWrite(led1, digitalRead(but1));   // turn the LED on based on button
  digitalWrite(led2, digitalRead(but2));

  delay(10);
}