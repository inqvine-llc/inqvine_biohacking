#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// BLE Server details
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Servo configuration
Servo myservo;
int servoPin = 15;

// BLE Client variables
static BLEUUID serviceUUID(SERVICE_UUID);
static BLEUUID charUUID(CHARACTERISTIC_UUID);
static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// Callback function for BLE notifications
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  
  Serial.print("Received data (");
  Serial.print(length);
  Serial.print(" bytes): ");
  
  String receivedData = "";
  for (int i = 0; i < length; i++) {
    receivedData += (char)pData[i];
  }
  Serial.println(receivedData);

  // Process the complete JSON data
  processJSONData(receivedData);
}

void processJSONData(String jsonString) {
  Serial.println("Processing JSON: " + jsonString);
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  unsigned long timestamp = doc["time"];
  unsigned long packetNumber = doc["packet"];

  Serial.print("Timestamp: ");
  Serial.print(timestamp);
  Serial.print(", Packet: ");
  Serial.println(packetNumber);

  if (doc.containsKey("IMU")) {
    JsonObject imu = doc["IMU"];
    float roll = imu["roll"];
    float pitch = imu["pitch"];
    float yaw = imu["yaw"];
    
    // Use absolute yaw value to control the servo
    float absYaw = abs(yaw);
    int servoPosition = map(absYaw, 0, 180, 0, 180);
    
    // Ensure the servo position is within bounds
    servoPosition = constrain(servoPosition, 0, 180);
    
    myservo.write(servoPosition);
    
    Serial.print("Roll: ");
    Serial.print(roll);
    Serial.print(", Pitch: ");
    Serial.print(pitch);
    Serial.print(", Yaw: ");
    Serial.print(yaw);
    Serial.print(", Abs Yaw: ");
    Serial.print(absYaw);
    Serial.print(", Servo position: ");
    Serial.println(servoPosition);
  } else {
    Serial.println("No IMU data in this packet");
  }

  if (doc.containsKey("sensors")) {
    JsonArray sensors = doc["sensors"];
    for (JsonObject sensor : sensors) {
      int channel = sensor["channel"];
      const char* name = sensor["name"];
      int value = sensor["value"];
      
      Serial.print("Sensor: ");
      Serial.print(name);
      Serial.print(" (Ch ");
      Serial.print(channel);
      Serial.print(") = ");
      Serial.println(value);
    }
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
    Serial.println("Connected to BLE Server");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected from BLE Server");
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the BLE Server.
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");

  // Increase MTU size
  pClient->setMTU(517);

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if(pRemoteCharacteristic->canRead()) {
    String value = pRemoteCharacteristic->readValue().c_str();
    Serial.print("The characteristic value was: ");
    Serial.println(value);
  }

  if(pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
  return true;
}

// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

  // Initialize servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);
  myservo.attach(servoPin, 500, 2500);

  // Initialize servo to middle position
  myservo.write(90);
  Serial.println("Servo initialized to 90 degrees");

  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    // The characteristic is handled in the callback
  } else if(doConnect == false) {
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  // Add a small delay to prevent overwhelming the serial output
  delay(10);
}