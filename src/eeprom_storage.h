#include <Arduino.h>
#include <EEPROM.h>

#ifndef EEPROM_STORAGE_H
#define EEPROM_STORAGE_H

class EEPROM_Storage {
  public:
    String ssid;
    String pass;

    EEPROM_Storage() {
      EEPROM.begin(512); //Initialasing EEPROM
    }

    void clear() {
        Serial.println("EEPROM Clearing...");
        for (int i = 0; i < 96; ++i)
          EEPROM.write(i, 0);
        EEPROM.commit();
    }

    bool write(String _ssid, String _pass) {
        ssid = _ssid;
        pass = _pass;

        if (ssid.length() > 0 && pass.length() > 0) {
          Serial.printf("EEPROM Write SSID: %s; Pass: %s\n", ssid.c_str(), pass.c_str());
  
          for (uint8_t i = 0; i < ssid.length(); ++i) {
            char symbol = ssid[i];
            EEPROM.write(i, symbol);
            Serial.printf("  Wrote SSID[%d]: %c\n", i, symbol);
          }

          for (uint8_t i = 0; i < pass.length(); ++i) {
            char symbol = pass[i];
            EEPROM.write(32 + i, symbol);
            Serial.printf("  Wrote Pass[%d]: %c\n", i, symbol);
          }
          EEPROM.commit();
          Serial.println("");
          return true;
        }
        return false;
    }

    bool read() {
      ssid = "";
      for (int i = 0; i < 32; ++i) {
        uint8_t byte = EEPROM.read(i);
        if(byte == 0)
          break;
        ssid += char(byte);
      }
      Serial.printf("EEPROM Read SSID: '%s'\n", ssid.c_str());

      pass = "";
      for (int i = 32; i < 96; ++i) {
        uint8_t byte = EEPROM.read(i);
        if(byte == 0)
          break;
        // Serial.printf("  Read Pass[%d]: %d\n", i, byte);
        pass += char(byte);
      }
      Serial.printf("EEPROM Read Pass: '%s'\n", pass.c_str());
//      Serial.printf("  Pass len = %d. isEmpty: %s\n", pass.length(), pass.isEmpty() ? "true" : "false");


      return ssid.length() > 0 && pass.length() > 0;
    }
};

EEPROM_Storage eepromStorage;

#endif /* EEPROM_STORAGE_H */