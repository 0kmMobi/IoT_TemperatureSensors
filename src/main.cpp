#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "led_blink.h"
#include "wifi_web_server.h"
#include "eeprom_storage.h"
#include "wifi_station.h"
#include "firebase_manager.h"
#include "sensors_manager.h"


const uint8_t STATE_START = 0;
const uint8_t STATE_WIFI_CONNECT = 1;
const uint8_t STATE_NEW_DEVICE_SERVER = 2;
const uint8_t STATE_MAIN_WORK = 4;

uint8_t curState;



LedBlink *led;


WifiStation *wifiStation;
WifiWebServer *webServer;

FirebaseManager *firebaseManager;
SensorsManager *sensorsManager;

uint32_t mainLoopTimer;

uint32_t sensorsDataTimer;

uint32_t updateFrameTime() {
  uint32_t delta = millis() - mainLoopTimer;
  mainLoopTimer = millis();
  return delta;
}


void changeState(uint8_t newState) {
  if(curState == newState)
    return;

  // Clearing after old state
  switch(curState) {
    case STATE_WIFI_CONNECT:
      delete led;
    break;
    case STATE_NEW_DEVICE_SERVER:
      delete led;
      delete webServer;
    break;
    case STATE_MAIN_WORK:
      delete led;
    break;
  }

  // Initializing for new state
  switch(newState) {
    case STATE_WIFI_CONNECT: {
      wifiStation = new WifiStation();
      wifiStation->initWiFiConnection();
      Phase tasks[] = {Phase(100, LOW), Phase(100, HIGH), Phase(100, LOW), Phase(100, HIGH), Phase(2000, LOW)};
      led = new LedBlink(PIN_LED_BUILTIN, tasks, 5, true);
      led->start();
      break;
    }
    case STATE_NEW_DEVICE_SERVER: {
      delete wifiStation;

      Serial.println("Init wifi web server.");
      Phase tasks[] = {Phase(50, LOW), Phase(50, HIGH)};
      led = new LedBlink(PIN_LED_BUILTIN, tasks, 2, true);
      led->start();
      webServer = new WifiWebServer();
      break;
    }
    case STATE_MAIN_WORK: {
      Serial.println("Init main mode.");
      pinMode(PIN_BTN_FLASH, INPUT_PULLUP);

      Phase tasks[] = {Phase(2000, LOW), Phase(200, HIGH)};
      led = new LedBlink(PIN_LED_BUILTIN, tasks, 2, true);
      led->start();

      String mac = wifiStation->getCompressedMAC();
      Serial.printf("  MAC Address: %s\n", mac.c_str() );
      Serial.printf("  ChipId as a 32-bit integer: %08X\n", ESP.getChipId() );
      Serial.printf("  Flash chipId as a 32-bit integer: %08X\n", ESP.getFlashChipId() );
      Serial.printf("  Flash chip frequency: %d Hz\n", ESP.getFlashChipSpeed() );
      Serial.printf("  Flash chip size: %d bytes\n", ESP.getFlashChipSize() );
      Serial.printf("  Free heap size: %d bytes\n", ESP.getFreeHeap() );

      firebaseManager = new FirebaseManager(mac);
      firebaseManager->sendDeviceInfo();

      sensorsManager = new SensorsManager();
      sensorsManager->initOnWireDS18B20();
      String *arrStrSensorsAddr = sensorsManager->getSensorsAddresses();
      uint8_t sensorsNumber = sensorsManager->getSensorsNumber();
      firebaseManager->sendSensorsList(arrStrSensorsAddr, sensorsNumber);
      Serial.println("After init Sensors and firebase");

      sensorsDataTimer = FREQ_MEASUREMENT_TIME_MSEC;
      break;
    }
  }
  curState = newState;
}


void setup() {
  Serial.begin(9600);
  Serial.println("");


  bool hasWiFiData = eepromStorage.read();
  Serial.printf("EEPROMStorage Read result: %s\n", hasWiFiData ? "true" : "false");

  if(hasWiFiData)
    changeState(STATE_WIFI_CONNECT);
  else
    changeState(STATE_NEW_DEVICE_SERVER);

  mainLoopTimer = millis();
}


void updateSensorsDataAndSendToDB(uint32_t dt, bool hasWiFiCon) {
  sensorsDataTimer += dt;
  if(hasWiFiCon && sensorsDataTimer >= FREQ_MEASUREMENT_TIME_MSEC) {
    if (Firebase.ready()) {
      String *arrStrSensorsAddr = sensorsManager->getSensorsAddresses();
      float* arrTemperatures = sensorsManager->getTemperatures();
      uint8_t sensorsNumber = sensorsManager->getSensorsNumber();

      firebaseManager->sendSensorsDataToDB(arrStrSensorsAddr, arrTemperatures, sensorsNumber);
      sensorsDataTimer -= FREQ_MEASUREMENT_TIME_MSEC;
    }
  }
}


void loop() {
  delay(10);
  uint32_t dt = updateFrameTime();

  switch(curState) {
    case STATE_WIFI_CONNECT: {
      led->update( dt );
      int8_t wifiConnectRes = wifiStation->updateWifiConnection( dt );
      switch(wifiConnectRes) {
        case WIFI_CONNECTION_SUCCESS:
          changeState(STATE_MAIN_WORK);
          break;
        case WIFI_CONNECTION_FAILURE: // Так никуда и не подключились. Теперь делаем web-server.
          changeState(STATE_NEW_DEVICE_SERVER);
          break;
      }

      if(digitalRead(PIN_BTN_FLASH) == LOW) {
        Serial.println("Flash button was pressed in 'Wifi connection' state.");
        changeState(STATE_NEW_DEVICE_SERVER);
      }
      break;
    }
    case STATE_NEW_DEVICE_SERVER: {
      led->update( dt );
      bool timeIsOut = webServer->updateAndGetTimeIsOut( dt );
      if(timeIsOut) {
        Serial.println("The web server is waiting too long for any action.");
        Serial.println("Restart now...\n");
        ESP.reset();
      }
      break;
    }
    case STATE_MAIN_WORK: {
      delay(50);
      led->update( dt );
      bool hasWiFiCon = wifiStation->wifiCheckConnected( dt );

      firebaseManager->checkIfNeedSendPong();

      updateSensorsDataAndSendToDB( dt, hasWiFiCon);

      if(digitalRead(PIN_BTN_FLASH) == LOW) {
        Serial.println("Flash button was pressed in 'Main work' state.");
        eepromStorage.clear();
        ESP.reset();
      }

      break;
    }
  }
}














