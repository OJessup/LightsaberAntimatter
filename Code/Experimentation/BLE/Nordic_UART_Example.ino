 /*Created by Muhammad Hammad on 21st October, 2021,
 * Email: muhammad.hammad1201@gmail.com
 * iOs App Source Code tested with this example: https://github.com/hammad1201/BLESerialIOsExample
 */
 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool reconnected = false;

String buff;
bool serialAvailable = false;

#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

void handleBLEAdvertisement(void);

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("Connected!");
      deviceConnected = true;  
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println("Disconnected!");
      deviceConnected = false;
      serialAvailable = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      
      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};  


void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("Lightsaber [Anti-matter]"); //Nordic UART Service

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE
                                          );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {

    if (Serial.available()) {
      buff = Serial.readString();
      serialAvailable = true;
    }

    if (serialAvailable == true) {
      Serial.print(buff);
      pTxCharacteristic->setValue(buff.c_str());
      pTxCharacteristic->notify();
      serialAvailable = false;
      buff = ""; //Clear the string
    }
  }

  handleBLEAdvertisement();
}

void handleBLEAdvertisement() {
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Start Advertising Again!");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}
