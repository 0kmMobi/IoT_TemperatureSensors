#ifndef CONSTS_H
#define CONSTS_H

  ///
  #define DS18B20_AND_AM2320_SENSORS_CONTROLLER 10
  #define DOOR_ALARM_CONTROLLER 11
  #define DS18B20_SENSORS_CONTROLLER 12
  ///
  #define IOT_TYPE DS18B20_SENSORS_CONTROLLER



  // Pins
  #define PIN_BTN_FLASH D3            // to WebServer mode for get WiFi info
  #define PIN_LED_BUILTIN LED_BUILTIN // D4



  // Sensors DS18B20
  #define PIN_ONE_WIRE_BUS D4 //Pin to which is attached a temperature sensor
  #define ONE_WIRE_MAX_DEV 3 //The maximum number of devices













  // Incoming IoT parameters from FireBase stream
  #define PARAM_NAME_DURATION "duration"
  #define PARAM_NAME_PING_TS "ping_ts"
  #define PARAM_NAME_PONG_TS "pong_ts"
  
  uint32_t FREQ_MEASUREMENT_TIME_MSEC = 30*1000; // The frequency receiving data from sensors
  
  void setDataDuration(int seconds) {
    if(seconds < 30)
      seconds = 30;
    else if(seconds > 24*60*60)
      seconds = 24*60*60;
    FREQ_MEASUREMENT_TIME_MSEC = seconds * 1000;
    Serial.printf("Set the DURATION between data to %d seconds.\n", seconds);
  }


  // Wifi AP and Web-server
  #define WIFI_AP_SSID_BASE "iot_"
  #define WIFI_AP_PASSWORD "12345678"

  #define WIFI_AP_LOCALIP "192.168.4.1"
  #define WIFI_AP_GATEWAY "192.168.4.1"
  #define WIFI_AP_NETMASK "255.255.255.0"


  #define WEB_SERVER_PORT 80
  #define WEBSERVER_WAITING_TIMER_MSEC_MAX (5*60*1000)

  // Firebase
  #define DB_DEVICES_IDS_TYPES "/devices_list/"
  #define DB_DEVICES_DATA "/devices_data/"
  #define DB_SENSOR_NAMES "sensor_names/"
  #define DB_DS18B20_TEMPERATURES "/temperatures/"



#endif /* CONSTS_H */