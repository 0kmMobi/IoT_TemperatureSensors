#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "consts.h"
#if __has_include("secret_real.h")
  #include "secret_real.h"
#else
  #include "secret_dummy.h"
#endif

class SensorsManager {
  OneWire *oneWire;
  DallasTemperature *DS18B20;
  int sensorsNumber; //Number of temperature devices found
  DeviceAddress sensorsAddr[ONE_WIRE_MAX_DEV];  //An array device temperature sensors
  String sensorsAddrStr[ONE_WIRE_MAX_DEV];
  float temperaturesC[ONE_WIRE_MAX_DEV];

public:
    SensorsManager() {
      oneWire = new OneWire(PIN_ONE_WIRE_BUS);
      DS18B20 = new DallasTemperature(oneWire);
    }

    ~SensorsManager() {
      delete oneWire;
      delete DS18B20;
    }

    //Setting the temperature sensor
    void initOnWireDS18B20() {
      Serial.printf("\nInitialization OneWire Sensors.\n");
      DS18B20->begin();

      Serial.printf("Parasite power is: %s.\n", DS18B20->isParasitePowerMode()?"ON":"OFF"); 
      sensorsNumber = DS18B20->getDeviceCount();
      Serial.printf("Number of found sensors: %d.\n", sensorsNumber);

      DS18B20->requestTemperatures();

      // Loop through each device, print out address
      for(int i = 0; i < sensorsNumber; i++) {
        // Search the wire for address
        if( DS18B20->getAddress(sensorsAddr[i], i) ) {
          sensorsAddrStr[i] = GetAddressToString(sensorsAddr[i]);
          Serial.printf("Device at #%d with address: %s.\n", i, sensorsAddrStr[i].c_str());
        } else {
          Serial.printf("!!! Ghost device at %d, but could not detect address. Check power and cabling.\n", i);
        }
        //Get resolution of DS18b20
        Serial.printf("  Resolution: %d bits.\n", DS18B20->getResolution( sensorsAddr[i] ));
        //Read temperature from DS18b20
        float tempC = DS18B20->getTempC( sensorsAddr[i] );
        Serial.printf("  Initial temperature: %f.\n\n", tempC);
      }
    }

    uint8_t getSensorsNumber() {
      return sensorsNumber;
    }

    //------------------------------------------
    //Convert device id to String
    String GetAddressToString(DeviceAddress oneSensorAddr) {
      String str = "";
      for(uint8_t i = 0; i < 8; i++) {
        if( oneSensorAddr[i] < 16 ) 
            str += String(0, HEX);
        str += String(oneSensorAddr[i], HEX);
      }
      return str;
    }

    String *getSensorsAddresses() {
      return sensorsAddrStr;
    }

    //Loop measuring the temperature
    float* getTemperatures() {
      Serial.println("\nGetting sensors new data.");
      DS18B20->setWaitForConversion(true); //No waiting for measurement
      DS18B20->requestTemperatures(); //Initiate the temperature measurement

      for(uint8_t i = 0; i < sensorsNumber; i++) {
        float tempC = DS18B20->getTempC(sensorsAddr[i]); //Measuring temperature in Celsius
        // if(tempC <= DEVICE_DISCONNECTED_C) {}

        Serial.printf("Temperature on [%s] = %f.\n", sensorsAddrStr[i].c_str(), tempC);

        temperaturesC[i] = tempC;
      }
      return temperaturesC;
    }
};