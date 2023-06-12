#include <Arduino.h>

#include <Adafruit_NeoPixel.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SparkFun_PCA9536_Arduino_Library.h>
#include <Wire.h>
#include <Adafruit_MAX1704X.h>
#include <SparkFun_VL53L5CX_Library.h>
#include <SPI.h>
#include <SD.h>

#include <audio.h>
#include <bluetooth.h>

//Settings
#define testMode          false
#define audioEnabled      true
#define gyroEnabled       true
#define stabilityEnabled  true
#define strictStability   true

//Pin definitions
const int 
  ledPin = 32, ledLatch = 16, sysLatch = 4, chipSelect = 5, mosi = 23, 
  miso = 19, sck = 18, chargeStat = 13, usbDet = 33, button = 17;

//Constant variables
const int 
  resolution = 4 * 4, rangingFrequency = 60, sensorAddress[] = {0x45, 0x46, 0x47, 0x29}, stabilityRange = 50,
  numLeds = 223, brightness = 50, minBrightness = 0, maxBrightness = 150, 
  debounceTime = 50,
  incrementNum = 60, instantaneousWidth = 600, nominalWidth = 200, randomIncrement = 40;

const float 
  ledSpacing = 6.9444, heightOffset = 0.0,
  minVoltage = 3.3, maxVoltage = 4.2,
  maxVelocity = 3.0, 
  humNominalGain = 0.3, humMaxGain = 0.6;

#if !strictStability
  const unsigned int priorityMask[4][resolution] = 
  {
    {0, 0, 0, 0, 3, 2, 2, 3, 3, 1, 1, 3, 0, 0, 0, 0},
    {0, 0, 0, 0, 3, 2, 2, 3, 3, 1, 1, 3, 0, 0, 0, 0},
    {0, 0, 0, 0, 3, 2, 2, 3, 3, 1, 1, 3, 0, 0, 0, 0},
    {0, 0, 0, 0, 3, 1, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0}
  }; //0 = don't use, 1 = primary, 2 = secondary, 3 = reserved (helper)
#else
  const unsigned int priorityMask[4][resolution] = 
  {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  }; //0 = don't use, 1 = primary
#endif

//Dynamic variables
unsigned long lastDebounceTime = 0;
int clashWidth[4], previousMeasurement[4], lastMeasurement[4];
float ledHeights[(numLeds-7)/2], minColVal[4];
int ledAnimationBrightness[(numLeds-7)/2];
float animationDisplacement = 0.0, animationVelocity = 0.1, animationAcceleration = 0.0;
bool audioOn = false, bluetoothOn = false, gyroscopeOn = false, vl53l5cxOn = false, isInContact = false, wasInContact = false;
int buttonState, lastButtonState = HIGH;
int previousPowerSetting;
String previousHumFile;

Adafruit_MPU6050 mpu;
Adafruit_MAX17048 lipo;
PCA9536 io;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numLeds, ledPin, NEO_GRB + NEO_KHZ800);

SparkFun_VL53L5CX sensor[4];
VL53L5CX_ResultsData MeasurementData[4];


void usbConnect() {

  digitalWrite(ledLatch, LOW);
  saveData(getColor(), getIRFile(), getIdleFile(), getShutdownOnDrop(), getPowerSetting());
  memShutdown();
  digitalWrite(sysLatch, LOW); 

  if(!bluetoothOn) {
    bluetoothInit();
    bluetoothOn = true;
  }

  while(!digitalRead(chargeStat)) {
    for (int i = 0; i < 255; i++) {
      strip.setPixelColor(0, map(i, 0, 255, 50, 128), 0, map(i, 0, 255, 50, 128));
      strip.show();
    }
    for (int i = 255; i > 0; i--) {
      strip.setPixelColor(0, map(i, 0, 255, 50, 128), 0, map(i, 0, 255, 50, 128));
      strip.show();
    }
    
    if (isDeviceConnected()) sendData(getIRFile() + "," + getIdleFile() + "," + getShutdownOnDrop() + "," + getPowerSetting() + "," + 0 + "," + "Charging");
  }

  strip.setPixelColor(0, 0, 255, 0);
  strip.show();
  while(true) {
    if(!isPlaying(0)) playWav("/ChargeDone.wav", 0, 0.1);
    audioLoop();
  }

}

void voltageTooLow() {
  digitalWrite(ledLatch, LOW);
  strip.setPixelColor(0, 255, 0, 0);
  strip.show();
  saveData(getColor(), getIRFile(), getIdleFile(), getShutdownOnDrop(), getPowerSetting());
  memShutdown();
  digitalWrite(sysLatch, LOW);
  while (true) {if(digitalRead(usbDet)) usbConnect();}
}

void ignition() {

  if (gyroEnabled) {
    if(!mpu.begin(0x69)) while(true);

    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    gyroscopeOn = true;
  }

  playWav(("/" + getIRFile() + "Ignition.wav").c_str(), 0, 0.7);
  playWav(("/" + getIdleFile() + ".wav").c_str(), 1, humNominalGain);

  int lengthMs;
  if (getIRFile().equals("Alternative")) lengthMs = 1700;
  if (getIRFile().equals("Deep")) lengthMs = 1057;
  if (getIRFile().equals("Default")) lengthMs = 1495;
  if (getIRFile().equals("Quick")) lengthMs = 1583;
  if (getIRFile().equals("SemiDeep")) lengthMs = 1051;
  int delayTime;
  if(lengthMs > 1100) {
    delayTime = lengthMs/((numLeds-7)/4);
  } else {
    delayTime = lengthMs/((numLeds-7)/4);
  }

  //for(int i = 1; i < 7; i++) strip.setPixelColor(i, getColor());
  //strip.show();

  uint32_t virtualLedStrip[(numLeds-7)/2];

  for(int i = 0; i < (numLeds-7)/2; i++) {

    long currTime = millis();

    animationVelocity += animationAcceleration;
    animationDisplacement += animationVelocity;
    animationDisplacement = fmod(animationDisplacement, 2.0);

    for(int j = 0; j < (numLeds-7)/2; j++) virtualLedStrip[j] = 0x00;
    i++;
    if(lengthMs < 1100) i++;
    for(int j = 0; j < i; j++) virtualLedStrip[j] = getColor();
    for(int j = 0; j < (numLeds-7)/2; j++) {
      ledAnimationBrightness[j] = getBumpBrightness(ledHeights[j], 1.0, 255.0, animationDisplacement, 0.0005, -0.3, 255.0, -1.0);
      virtualLedStrip[j] = bumpPixelBrightness(virtualLedStrip[j], ledAnimationBrightness[j]);
    }

    for(int j = 1; j < 7; j++) strip.setPixelColor(j, virtualLedStrip[0]);

    for(int j = 0; j < (numLeds-7)/2; j++) {
      strip.setPixelColor(j+7, virtualLedStrip[j]);
      strip.setPixelColor(numLeds-j-1, virtualLedStrip[j]);
    }

    strip.show();

    bluetoothLoop();

    if(digitalRead(usbDet) && !testMode) usbConnect();
    if (lipo.cellVoltage() < minVoltage) voltageTooLow();
    float batteryPercent = mapFloat(lipo.cellVoltage(), minVoltage, maxVoltage, 0.0, 100.0);
    strip.setPixelColor(0, map(batteryPercent, 0.0, 100.0, 255, 0), map(batteryPercent, 0.0, 100.0, 0, 255), 0);

    if (isDeviceConnected()) sendData(getIRFile() + "," + getIdleFile() + "," + getShutdownOnDrop() + "," + getPowerSetting() + "," + batteryPercent + "," + "Running");

    int targetDelay = delayTime-millis()+currTime-1;
    long currentTimeStamp = millis();
    while(millis()-currentTimeStamp < targetDelay) audioLoop();

  }

}

void retraction() { 

  playWav(("/" + getIRFile() + "Retraction.wav").c_str(), 0, 0.7);

  int lengthMs;
  if (getIRFile().equals("Alternative")) lengthMs = 1700;
  if (getIRFile().equals("Deep")) lengthMs = 1057;
  if (getIRFile().equals("Default")) lengthMs = 1495;
  if (getIRFile().equals("Quick")) lengthMs = 1583;
  if (getIRFile().equals("SemiDeep")) lengthMs = 1051;
  int delayTime;
  if(lengthMs > 1100) {
    delayTime = lengthMs/((numLeds-7)/4);
  } else {
    delayTime = lengthMs/((numLeds-7)/4);
  }

  stopWav(1);

  uint32_t virtualLedStrip[(numLeds-7)/2];

  for(int i = ((numLeds-7)/2)-1; i >= 0; i--) {

    long currTime = millis();

    animationVelocity += animationAcceleration;
    animationDisplacement += animationVelocity;
    animationDisplacement = fmod(animationDisplacement, 2.0);

    for(int j = 0; j < (numLeds-7)/2; j++) virtualLedStrip[j] = getColor();
    i--;
    if(lengthMs < 1100) i--;
    for(int j = i; j < (numLeds-7)/2; j++) virtualLedStrip[j] = 0x00;
    for(int j = 0; j < (numLeds-7)/2; j++) {
      ledAnimationBrightness[j] = getBumpBrightness(ledHeights[j], 1.0, 255.0, animationDisplacement, 0.0005, -0.3, 255.0, -1.0);
      virtualLedStrip[j] = bumpPixelBrightness(virtualLedStrip[j], ledAnimationBrightness[j]);
    }

    for(int j = 1; j < 7; j++) strip.setPixelColor(j, virtualLedStrip[0]);

    for(int j = 0; j < (numLeds-7)/2; j++) {
      strip.setPixelColor(j+7, virtualLedStrip[j]);
      strip.setPixelColor(numLeds-j-1, virtualLedStrip[j]);
    }

    strip.show();

    bluetoothLoop();

    if(digitalRead(usbDet) && !testMode) usbConnect();
    if (lipo.cellVoltage() < minVoltage) voltageTooLow();
    float batteryPercent = mapFloat(lipo.cellVoltage(), minVoltage, maxVoltage, 0.0, 100.0);
    strip.setPixelColor(0, map(batteryPercent, 0.0, 100.0, 255, 0), map(batteryPercent, 0.0, 100.0, 0, 255), 0);

    if (isDeviceConnected()) sendData(getIRFile() + "," + getIdleFile() + "," + getShutdownOnDrop() + "," + getPowerSetting() + "," + batteryPercent + "," + "Running");

    int targetDelay = delayTime-millis()+currTime-1;
    long currentTimeStamp = millis();
    while(millis()-currentTimeStamp < targetDelay) audioLoop();

  }

  for(int i = 6; i >= 1; i--) strip.setPixelColor(i, 0, 0, 0);
  strip.show();

}

void sensorSetup() {

  for (int i = 0; i < 4; i++) io.pinMode(i, OUTPUT);
  for (int i = 0; i < 4; i++) io.digitalWrite(i, LOW);

  for(int i = 1; i < numLeds; i++) strip.setPixelColor(i, 0, 0, 0);
  strip.setPixelColor(1, getColor());
  strip.show();

  Serial.println("Starting sensors ");
  Serial.println("[        ] 0%");

  bool individualVL53L5CXOn[4] = {false, false, false, false};
  vl53l5cxOn = false;
  for(int i = 0; i < 4; i++) {
    if (!vl53l5cxOn) {
      io.digitalWrite(i, HIGH);
      if(!sensor[i].begin()) {
        Serial.print("Sensor ");
        Serial.print(i);
        Serial.println(" failed");
      } else {
        sensor[i].setAddress(sensorAddress[i]);
        sensor[i].setResolution(resolution);
        sensor[i].setRangingMode(SF_VL53L5CX_RANGING_MODE::CONTINUOUS);
        sensor[i].setRangingFrequency(rangingFrequency);
        sensor[i].startRanging();
        individualVL53L5CXOn[i] = true;
      }
    }
    if (i == 0) Serial.println("[==      ] 25%  ");
    if (i == 1) Serial.println("[====    ] 50%  ");
    if (i == 2) Serial.println("[======  ] 75%  ");
    if (i == 3) Serial.println("[========] 100% ");
  }
  vl53l5cxOn = true;
  for(int i = 0; i < 4; i++) if(!individualVL53L5CXOn[i]) vl53l5cxOn = false;
  Serial.println("VL53L5CX sensors started");

}



void setup() {

  pinMode(sysLatch, OUTPUT);
  digitalWrite(sysLatch, HIGH);

  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(1000000);

  io.begin();
  pinMode(usbDet, INPUT);
  pinMode(chargeStat, INPUT_PULLUP);
  pinMode(button, INPUT_PULLUP);

  lipo.begin();

  memInitData();

  for(int i = 0; i < (numLeds-7)/2; i++) {
    ledHeights[i] = ((float)i*ledSpacing)+heightOffset;
    ledAnimationBrightness[i] = 0;
  }
  for(int i = 0; i < 4; i++) {
    previousMeasurement[i] = -1 * stabilityRange;
    lastMeasurement[i] = -1 * stabilityRange;
    minColVal[i] = 0.1;
  }
  previousHumFile = getIdleFile();
  previousPowerSetting = getPowerSetting();

  audioInit();

  strip.begin();
  strip.setBrightness(map(getPowerSetting(), 0, 255, minBrightness, maxBrightness));
  strip.show();

  if (digitalRead(usbDet) && !testMode) usbConnect(); 

  while(lipo.cellVoltage() == 0.00);
  if (lipo.cellVoltage() < minVoltage) voltageTooLow();
  float batteryPercent = mapFloat(lipo.cellVoltage(), minVoltage, maxVoltage, 0.0, 100.0);
  strip.setPixelColor(0, map(batteryPercent, 0.0, 100.0, 255, 0), map(batteryPercent, 0.0, 100.0, 0, 255), 0);
  strip.show();

  bluetoothInit();
  bluetoothOn = true;

  pinMode(ledLatch, OUTPUT);
  digitalWrite(ledLatch, HIGH);

  SPI.begin(sck, miso, mosi);
  SD.begin(chipSelect);

  sensorSetup();

  ignition();

}

void loop() {

  exitSleep:

  if (digitalRead(usbDet) && !testMode) usbConnect();

  if (lipo.cellVoltage() < minVoltage) voltageTooLow();
  float batteryPercent = mapFloat(lipo.cellVoltage(), minVoltage, maxVoltage, 0.0, 100.0);
  strip.setPixelColor(0, map(batteryPercent, 0.0, 100.0, 255, 0), map(batteryPercent, 0.0, 100.0, 0, 255), 0);

  for(int i=1; i<numLeds; i++) strip.setPixelColor(i, getColor());

  if(vl53l5cxOn) for(int i = 0; i < 4; i++) if(sensor[i].isDataReady()) sensor[i].getRangingData(&MeasurementData[i]);

  if (gyroEnabled) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float currentVelocity = calculateMagnitudeVelocity(g.gyro.x, g.gyro.y, g.gyro.z);
    float currentHumGain = mapFloat(currentVelocity, 0.0, maxVelocity, humNominalGain, humMaxGain);
    setGain(1, currentHumGain);

    animationVelocity = mapFloat(currentVelocity, 0.0, maxVelocity, 0.1, 0.2);
  }

  //Height processing
  int currentMeasurement[4] = {-1 * stabilityRange, -1 * stabilityRange, -1 * stabilityRange, -1 * stabilityRange};
  for (int i = 0; i < 4; i++ ) {
    int priority = 1;
    while(priority < 3) {
      for(int j = 0; j < resolution; j++) {
        if(priorityMask[i][j] == priority && MeasurementData[i].target_status[j] == 5) {
          currentMeasurement[i] = MeasurementData[i].distance_mm[j];
          goto foundMeasurement;
        }
      }
      priority++;
    }
    foundMeasurement:
    ;
  }

  for(int i = 0; i < 4; i++) {
    if(currentMeasurement[i] > 760) currentMeasurement[i] = -1 * stabilityRange;
  }

  bool currentMeasurementStable[4] = {false, false, false, false};
  for(int i = 0; i < 4; i++) if(abs(currentMeasurement[i] - previousMeasurement[i]) < stabilityRange) currentMeasurementStable[i] = true;
  if (!stabilityEnabled) for (int i = 0; i < 4; i++) currentMeasurementStable[i] = true;

  int sensorMeasurement[4] = {-1, -1, -1, -1};
  for(int i = 0; i < 4; i++) if (currentMeasurement[i] > 0 && currentMeasurementStable[i]) sensorMeasurement[i] = currentMeasurement[i];

  for(int i = 0; i < 4; i++) if((currentMeasurement[i] < 0 || !currentMeasurementStable[i]) && previousMeasurement[i] > 0) lastMeasurement[i] = previousMeasurement[i];

  audioLoop();

  for(int i = 0; i < 4; i++) {
    minColVal[i] = 1.0;
    if(currentMeasurement[i] > 0 && currentMeasurementStable[i] && clashWidth[i] == 0) clashWidth[i] = instantaneousWidth;
    if(currentMeasurement[i] > 0 && currentMeasurementStable[i] && clashWidth[i] > nominalWidth) clashWidth[i]-=incrementNum;
    if(currentMeasurement[i] > 0 && currentMeasurementStable[i] && clashWidth[i] < nominalWidth) clashWidth[i]+=incrementNum;
    if((currentMeasurement[i] < 0 || !currentMeasurementStable[i]) && clashWidth[i] > 0) clashWidth[i]-=incrementNum;
    if(clashWidth[i] < 0) clashWidth[i] = 0;
    if(currentMeasurement[i] > 0 && currentMeasurementStable[i] && clashWidth[i] > 0) {
      bool sign = random(0, 2);
      if(sign) {
        clashWidth[i] += randomIncrement;
      } else {
        clashWidth[i] -= randomIncrement;
      }
      if(clashWidth[i] < 0) clashWidth[i] = 0;
    }
    minColVal[i] = mapFloat(clashWidth[i], 0.0, nominalWidth, 1.0, 0.1);
    if(minColVal[i] > 1.0) minColVal[i] = 1.0;
    if(minColVal[i] < 0.1) minColVal[i] = 0.1;
  }

  //==================================================================================================================================//
  //CHECK THIS
  animationVelocity += animationAcceleration;
  animationDisplacement += animationVelocity;
  animationDisplacement = fmod(animationDisplacement, 2.0);

  for(int i = 0; i < (numLeds-7)/2; i++) ledAnimationBrightness[i] = getBumpBrightness(ledHeights[i], 1.0, 255.0, animationDisplacement, 0.0005, -0.3, 255.0, -1.0); //CHECK THESE VALUES

  uint32_t virtualLedStrip[4][(numLeds-7)/2];
  for(int i = 0; i < 4; i++) for(int j = 0; j < (numLeds-7)/2; j++) virtualLedStrip[i][j] = bumpPixelBrightness(getColor(), ledAnimationBrightness[j]);

  for(int i = 1; i < 7; i++) strip.setPixelColor(i, virtualLedStrip[0][0]);
  //==================================================================================================================================//

  for(int i = 0; i < 4; i++) {
    if(currentMeasurement[i] > 0 && currentMeasurementStable[i]) {
      for(int j = 0; j < (numLeds-7)/2; j++) virtualLedStrip[i][j] = calculateColor(ledHeights[j], 170, currentMeasurement[i], clashWidth[i], virtualLedStrip[i][j], minColVal[i]);
    } else {
      if(lastMeasurement[i] > 0) for(int j = 0; j < (numLeds-7)/2; j++) virtualLedStrip[i][j] = calculateColor(ledHeights[j], 170, lastMeasurement[i], clashWidth[i], virtualLedStrip[i][j], minColVal[i]);
    }
  }

  uint32_t finalVirtualLedStrip[(numLeds-7)/2];
  for(int i = 0; i < (numLeds-7)/2; i++) finalVirtualLedStrip[i] = calculatePixel(virtualLedStrip[0][i], virtualLedStrip[1][i], virtualLedStrip[2][i], virtualLedStrip[3][i]);
  for(int i = 0; i < 4; i++) previousMeasurement[i] = currentMeasurement[i];

  for(int i = 0; i < (numLeds-7)/2; i++) {
    strip.setPixelColor(i+7, finalVirtualLedStrip[i]);
    strip.setPixelColor(numLeds-i-1, finalVirtualLedStrip[i]);
  }

  if (isDeviceConnected()) sendData(getIRFile() + "," + getIdleFile() + "," + getShutdownOnDrop() + "," + getPowerSetting() + "," + batteryPercent + "," + "Running");

  bluetoothLoop();

  audioLoop();
 
  if (!isPlaying(1)) playWav(("/" + getIdleFile() + ".wav").c_str(), 1, humNominalGain);

  String currentHumFile = getIdleFile();
  if(!currentHumFile.equals(previousHumFile)) playWav(("/" + currentHumFile + ".wav").c_str(), 1, humNominalGain);
  previousHumFile = currentHumFile;

  //CHECK THIS
  int currentPowerSetting = getPowerSetting();
  if(currentPowerSetting != previousPowerSetting) {
    strip.setBrightness(map(currentPowerSetting, 0, 255, minBrightness, maxBrightness));
  }
  previousPowerSetting = currentPowerSetting;

  //Sound
  isInContact = false;
  for(int i = 0; i < 4; i++) {
    if(currentMeasurement[i] > 0 && currentMeasurementStable[i])
    isInContact = true;
  }

  const float clashGainSelection = 0.7;
  if (isInContact && !wasInContact) { 
    if (!isPlaying(0)) {
      if (random(0, 2)) {
        playWav("/ClashFinalLong1.wav", 0, clashGainSelection);
      } else {
        playWav("/ClashFinalLong2.wav", 0, clashGainSelection);
      }
    } else {
      stopFade();
      fadeTo(0, 50, clashGainSelection);
    }
  }
  
  if(!isInContact && wasInContact) {
    stopFade();
    fadeTo(0, 200, 0.0);
  }

  audioLoop();

  wasInContact = isInContact;

  //Check button and touch
  bool isReleased = false;
  int reading = digitalRead(button);
  
  if(reading != lastButtonState) lastDebounceTime = millis();
  
  if(millis()-lastDebounceTime > debounceTime) {
    if(reading != buttonState) {
      buttonState = reading;
  
      if(buttonState == LOW) isReleased = true;
    }
  }
  lastButtonState = reading;

  if(isReleased || (touchRead(6) < 100 && getShutdownOnDrop() && false)) {

    retraction();
    digitalWrite(ledLatch, LOW);
    long currentTime = millis();

    while (millis() - currentTime < 300000) {

      if (digitalRead(usbDet) && !testMode) usbConnect();

      if (lipo.cellVoltage() < minVoltage) voltageTooLow();
      float batteryPercent = mapFloat(lipo.cellVoltage(), minVoltage, maxVoltage, 0.0, 100.0);
      strip.setPixelColor(0, map(batteryPercent, 0.0, 100.0, 255, 0), map(batteryPercent, 0.0, 100.0, 0, 255), 0);
      strip.show();

      //Check button
      isReleased = false;
      reading = digitalRead(button);
      
      if(reading != lastButtonState) lastDebounceTime = millis();
      
      if(millis()-lastDebounceTime > debounceTime) {
        if(reading != buttonState) {
          buttonState = reading;
      
          if(buttonState == LOW) isReleased = true;
        }
      }
      lastButtonState = reading;

      if(isReleased || (touchRead(6) > 100 && getShutdownOnDrop() && false)) {
        digitalWrite(ledLatch, HIGH);
        ignition();
        goto exitSleep;
      }

      if (isDeviceConnected()) sendData(getIRFile() + "," + getIdleFile() + "," + getShutdownOnDrop() + "," + getPowerSetting() + "," + 0 + "," + "Resting");

    }

    saveData(getColor(), getIRFile(), getIdleFile(), getShutdownOnDrop(), getPowerSetting());
    memShutdown();
    digitalWrite(sysLatch, LOW);

  }

  strip.show();

  audioLoop();

}