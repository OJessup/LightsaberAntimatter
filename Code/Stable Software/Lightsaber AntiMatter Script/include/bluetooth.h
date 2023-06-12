#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <Preferences.h>

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool reconnected = false;

Preferences mem;

#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

uint32_t storeColor; //Check if all this conversion and stuff works
String IRFile, idleFile;
bool shutdownOnDrop;
int powerSetting;

bool powerSettingChanged = false;

void memInitData() {

    mem.begin("memSpace", false);

    storeColor = mem.getUInt("color", 0x00FF0000);
    IRFile = mem.getString("irfile", "Default");
    idleFile = mem.getString("idlefile", "Deep");
    shutdownOnDrop = mem.getBool("shutdownondrop", true);
    powerSetting = mem.getInt("powersetting", 150);

}

void saveData(uint32_t colorInput, String IRFileInput, String idleFileInput, bool shutdownOnDropInput, int powerSettingInput) {

    mem.putUInt("color", colorInput);
    mem.putString("irfile", IRFileInput);
    mem.putString("idlefile", idleFileInput);
    mem.putBool("shutdownondrop", shutdownOnDropInput);
    mem.putInt("powersetting", powerSettingInput);
    
}

void memShutdown() {mem.end();}

bool isDeviceConnected() {return deviceConnected;}

uint32_t getColor() {return storeColor;}
String getIRFile() {return IRFile;}
String getIdleFile() {return idleFile;}
bool getShutdownOnDrop() {return shutdownOnDrop;}
int getPowerSetting() {return powerSetting;}

void handleBLEAdvertisement(void);

class MyServerCallbacks: public BLEServerCallbacks {

    void onConnect(BLEServer* pServer) {
      deviceConnected = true;  
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {

    void onWrite(BLECharacteristic *pCharacteristic) {

      std::string rxValue = pCharacteristic->getValue();
      
      if (rxValue.length() > 0) {
        //RGB math
        if (rxValue[0] == '#') {
          unsigned int hexToDec[6];
          for (int i = 1; i <= 6; i++) {
            if (rxValue[i] == '0') hexToDec[i-1] = 1;
            if (rxValue[i] == '1') hexToDec[i-1] = 2;
            if (rxValue[i] == '2') hexToDec[i-1] = 3;
            if (rxValue[i] == '3') hexToDec[i-1] = 4;
            if (rxValue[i] == '4') hexToDec[i-1] = 5;
            if (rxValue[i] == '5') hexToDec[i-1] = 6;
            if (rxValue[i] == '6') hexToDec[i-1] = 7;
            if (rxValue[i] == '7') hexToDec[i-1] = 8;
            if (rxValue[i] == '8') hexToDec[i-1] = 9;
            if (rxValue[i] == '9') hexToDec[i-1] = 10;
            if (rxValue[i] == 'A') hexToDec[i-1] = 11;
            if (rxValue[i] == 'B') hexToDec[i-1] = 12;
            if (rxValue[i] == 'C') hexToDec[i-1] = 13;
            if (rxValue[i] == 'D') hexToDec[i-1] = 14;
            if (rxValue[i] == 'E') hexToDec[i-1] = 15;
            if (rxValue[i] == 'F') hexToDec[i-1] = 16;
          }
          int rVal = constrain(map((hexToDec[0]*16) + (hexToDec[1]), 25, 230, 0, 255), 0, 255);
          int gVal = constrain(map((hexToDec[2]*16) + (hexToDec[3]), 25, 230, 0, 255), 0, 255);
          int bVal = constrain(map((hexToDec[4]*16) + (hexToDec[5]), 25, 230, 0, 255), 0, 255);
          storeColor = 0x00000000 + bVal + (gVal<<8) + (rVal<<16);
        }

        if (rxValue[0] == '=') {
          String temp = "";
          for(int i = 1; i < rxValue.length(); i++) {
            temp += rxValue[i];
          }
          idleFile = temp;
        }

        if (rxValue[0] == '&') {
          String temp = "";
          for(int i = 1; i < rxValue.length(); i++) {
            temp += rxValue[i];
          }
          IRFile = temp;
        }

        if (rxValue[0] == '+') {
          shutdownOnDrop = !shutdownOnDrop;
        }

        if (rxValue[0] == '_') {
          String temp = "";
          for(int i = 1; i < rxValue.length(); i++) {
            temp += rxValue[i];
          }
          powerSetting = temp.toInt();
        }

      }
    }
};

void handleBLEAdvertisement() {

  if (!deviceConnected && oldDeviceConnected) {
    //delay(500); //CAN'T HAVE THIS DELAY
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    //Send data
  }
}

void bluetoothInit() {

    //This takes ~731ms
    BLEDevice::init("Lightsaber [Anti-matter]");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

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

    pService->start();

    pServer->getAdvertising()->start();
}

void sendData(String buff) {
    pTxCharacteristic->setValue(buff.c_str());
    pTxCharacteristic->notify();
}

void bluetoothLoop() {handleBLEAdvertisement();}