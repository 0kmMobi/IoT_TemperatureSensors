#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "eeprom_storage.h"

enum wifiConnectionResult {
    WIFI_CONNECTION_IN_PROGRESS  = 0,
    WIFI_CONNECTION_SUCCESS = 1,
    WIFI_CONNECTION_FAILURE = -1
};

class WifiStation {
  public:
    WifiStation() {}

    ~WifiStation() {
      if(WiFi.isConnected())
        WiFi.disconnect(false);
    }

    String getCompressedMAC() {
      uint8_t mac[6];
      WiFi.macAddress(mac);

      char macStr[18] = { 0 };
      sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      return String(macStr);
    }

    /**
     * Режим подключения к wifi-сети. Инициируем подключение, а затем в течении некоторого времени (30 * 500 мсек = 15 сек) проверяем статус
     */
    uint32_t wifiConnectionCheckTimer;
    uint8_t wifiConnectionCheckAttempts;
    const uint32_t WIFI_CONNECTION_CHECK_TIMER_MAX = 500; // После 500 мсек проверяем статус
    const uint8_t WIFI_CONNECTION_CHECK_ATTEMPTS_MAX = 20; // Таких проверок 30 штук

    uint8_t wifiConnectionLastStatus = WL_IDLE_STATUS; // Тут будет храниться причина неудачного подключения. Вероятно, это WL_NO_SSID_AVAIL или WL_WRONG_PASSWORD.

    void initWiFiConnection() {
      String ssid = eepromStorage.ssid;
      String pass = eepromStorage.pass;

      Serial.printf("\nConnecting to Wifi '%s': ", ssid.c_str());

      WiFi.begin(ssid.c_str(), pass.c_str());

      wifiConnectionCheckTimer = 0;
      wifiConnectionCheckAttempts = 0;
    }

    int8_t updateWifiConnection(uint32_t dt) {
      wifiConnectionCheckTimer += dt;

      if(wifiConnectionCheckTimer > WIFI_CONNECTION_CHECK_TIMER_MAX) {
        wifiConnectionLastStatus = WiFi.status();
        if(wifiConnectionLastStatus == WL_CONNECTED) {
          Serial.println("\nConnection is done");
          return WIFI_CONNECTION_SUCCESS;
        } else {
          wifiConnectionCheckTimer -= WIFI_CONNECTION_CHECK_TIMER_MAX;
          wifiConnectionCheckAttempts ++;
          Serial.print(".");
          if(wifiConnectionCheckAttempts > WIFI_CONNECTION_CHECK_ATTEMPTS_MAX) {
            Serial.printf("\nNo connection. Status= %d\n", WiFi.status());
            return WIFI_CONNECTION_FAILURE;
          }
        }
      }
      return WIFI_CONNECTION_IN_PROGRESS;
    }

    /**
     * Периодическая проверка статуса подключения в процессе основной работы
     */
    uint32_t wifiDisconectTimeStart = 0;
    const uint32_t WIFI_DISCONECT_TIME_MAX_MSEC = 10 * 1000;

    bool wifiCheckConnected(uint32_t dt) {
      if(WiFi.status() == WL_CONNECTED) {
        wifiDisconectTimeStart = 0;
        return true;
      }

      if(wifiDisconectTimeStart == 0)
        WiFi.reconnect();

      wifiDisconectTimeStart += dt;
      if(wifiDisconectTimeStart >= WIFI_DISCONECT_TIME_MAX_MSEC) {
        Serial.printf("Wifi was lost. The device will reboot now.\n");
        ESP.restart();
      }
      return false;
    }
};