
#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#if __has_include("secret_real.h")
  #include "secret_real.h"
#else
  #include "secret_dummy.h"
#endif

#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#include "consts.h"


  static bool needToSendPong = false;
  FirebaseData stream;

  void streamCallback(FirebaseStream data) {
    Serial.printf("*** sream path: %s ; event path: %s ; data type: %s ; event type, %s\n",
                  data.streamPath().c_str(),
                  data.dataPath().c_str(),
                  data.dataType().c_str(),
                  data.eventType().c_str());
    printResult(data); // see addons/RTDBHelper.h
    Serial.println();

    // This is the size of stream payload received (current and max value)
    // Max payload size is the payload size under the stream path since the stream connected
    // and read once and will not update until stream reconnection takes place.
    // This max value will be zero as no payload received in case of ESP8266 which
    // BearSSL reserved Rx buffer size is less than the actual stream payload.
    Serial.printf("*** PING Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());

    needToSendPong = true;
  }

  void streamTimeoutCallback(bool timeout) {
    if (timeout)
      Serial.println("*** stream timed out, resuming...\n");

    if (!stream.httpConnected())
      Serial.printf("*** error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
  }


class FirebaseManager {
  //Define Firebase Data object
  String deviceMAC;
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;

  

  void initPingStream() {
      Firebase.setDoubleDigits(5);
      Serial.println("  Initialization 'Ping' stream... ");

#if defined(ESP8266)
      stream.setBSSLBufferSize(2048 /* Rx in bytes, 512 - 16384 */, 512 /* Tx in bytes, 512 - 16384 */);
#endif
      String path = DB_DEVICES_IDS_TYPES + deviceMAC + "/ping_last_ts"; 
      if (!Firebase.RTDB.beginStream(&stream, path))
        Serial.printf("    sream begin error, %s\n\n", stream.errorReason().c_str());

      Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);

      /** Timeout options, below is default config.
      //WiFi reconnect timeout (interval) in ms (10 sec - 5 min) when WiFi disconnected.
      config.timeout.wifiReconnect = 10 * 1000;
      //Socket begin connection timeout (ESP32) or data transfer timeout (ESP8266) in ms (1 sec - 1 min).
      config.timeout.socketConnection = 30 * 1000;
      //ESP32 SSL handshake in ms (1 sec - 2 min). This option doesn't allow in ESP8266 core library.
      config.timeout.sslHandshake = 2 * 60 * 1000;
      //Server response read timeout in ms (1 sec - 1 min).
      config.timeout.serverResponse = 10 * 1000;
      //RTDB Stream keep-alive timeout in ms (20 sec - 2 min) when no server's keep-alive event data received.
      config.timeout.rtdbKeepAlive = 45 * 1000;
      //RTDB Stream reconnect timeout (interval) in ms (1 sec - 1 min) when RTDB Stream closed and want to resume.
      config.timeout.rtdbStreamReconnect = 1 * 1000;
      //RTDB Stream error notification timeout (interval) in ms (3 sec - 30 sec). It determines how often the readStream
      //will return false (error) when it called repeatedly in loop.
      config.timeout.rtdbStreamError = 3 * 1000;
      */
  }

  public:
    FirebaseManager(String mac) {
      Serial.printf("\nInitialization Firebase Client v%s.\n", FIREBASE_CLIENT_VERSION);
      deviceMAC = mac;

      /* Assign the api key (required) */
      config.api_key = API_KEY;
      /* Assign the RTDB URL (required) */
      config.database_url = DATABASE_URL;

      // For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino
      /* Assign the user sign in credentials */
      auth.user.email = USER_EMAIL;
      auth.user.password = USER_PASSWORD;  

      fbdo.setResponseSize(4096);

      /* Assign the callback function for the long running token generation task */
      config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

      /* Initialize the library with the Firebase authen and config */
      Firebase.begin(&config, &auth);
      Firebase.reconnectWiFi(true);

      initPingStream();
      Serial.println("Firebase Client initialized");
    };


    void sendDeviceInfo() {
      //String path = DB_DEVICES_IDS_TYPES + deviceMAC;
      String path = DB_DEVICES_IDS_TYPES + deviceMAC + "/type";
      Serial.printf("sendIOTDeviceInfo: Path = %s\n", path.c_str());
      
      bool result = Firebase.RTDB.setInt(&fbdo, path, IOT_TYPE);
      Serial.printf("sendIOTDeviceInfo: result = %s\n", result ? "ok" : fbdo.errorReason().c_str());
    }


    void sendSensorsList(String * arrStrSensorsAddr, uint8_t sensorsNumber) {
        Serial.printf("\nFinding new sensors and sending their arddesses to the Firebase DB.\n");
        Serial.printf("  Total actial sensors %d.\n", sensorsNumber);
        String path = DB_DEVICES_IDS_TYPES + deviceMAC + "/" + DB_SENSOR_NAMES;

        // Create new JSON Array and add to it all actual sensors
        FirebaseJson jsonMapSensorsAddr;

        for(uint8_t i = 0; i < sensorsNumber; i++) {
          String sensorName = arrStrSensorsAddr[i];
          jsonMapSensorsAddr.set(arrStrSensorsAddr[i], arrStrSensorsAddr[i]);
        }

        // Read from DB sensors list which were stored them there
        bool getJSONRes = Firebase.RTDB.getJSON(&fbdo, path);
        int resCode = fbdo.httpCode();

        if (getJSONRes && resCode == FIREBASE_ERROR_HTTP_CODE_OK) {
          FirebaseJson *jsonMapDB = fbdo.to<FirebaseJson *>();
          size_t jsonArrDBLen = jsonMapDB->iteratorBegin();
          Serial.printf("  Initial number of sensors in DB = %d\n", jsonArrDBLen);

          for (size_t iDB = 0; iDB < jsonArrDBLen; iDB++) {
            String sSensorAddrDB = jsonMapDB->valueAt(iDB).key;
            String sSensorNameDB = jsonMapDB->valueAt(iDB).value;
            sSensorNameDB = sSensorNameDB.substring(1, sSensorNameDB.length() - 1); // Trim leading and ending quotes
            
            bool isNotActualSensor = true;
            for(uint8_t iReal = 0; iReal < sensorsNumber; iReal++) {
              String sSensorNameReal = arrStrSensorsAddr[iReal];
              if(sSensorNameDB.equalsIgnoreCase(sSensorNameReal)) {
                isNotActualSensor = false;
                break;
              }
            }
            // Add sensor from DB to total sensors list
            if(isNotActualSensor) {
              jsonMapSensorsAddr.set(sSensorAddrDB, sSensorNameDB);
            }
          }
          jsonMapDB->iteratorEnd();
        }

        // The total sensors list send to DB
        bool jsonSetResult = Firebase.RTDB.set(&fbdo, path, &jsonMapSensorsAddr);
        Serial.printf("Set sensors names list to RTDB... %s\n", jsonSetResult ? "ok" : fbdo.errorReason().c_str());
        Serial.println();
    }

    // /** Not used */ 
    // void checkIfDataPathIsExists() {
    //     String path = DB_DEVICES_DATA + deviceMAC + "/";
    //     Serial.printf("checkIfDataPathIsExists: Path: %s.\n", path.c_str());

    //     QueryFilter query;
    //     query.orderBy("time");
    //     query.limitToFirst(1);

    //     bool getJSONRes = Firebase.RTDB.getJSON(&fbdo, path, &query);
    //     int resCode = fbdo.httpCode();
    //     Serial.printf("checkIfDataPathIsExists: getJSONRes= %s: resCode= %d.\n", getJSONRes?"True":"False", resCode);

    //     if (Firebase.RTDB.getJSON(&fbdo, path, &query) && fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK) {
    //       // FirebaseJson *fbJsonMain = fbdo.to<FirebaseJson *>();

    //       // size_t jsonMainLen = fbJsonMain->iteratorBegin();
    //       // FirebaseJson::IteratorValue jsonIterValue;

    //       // for (size_t i = 0; i < jsonMainLen; i++) {
    //       //   jsonIterValue = fbJsonMain->valueAt(i);
    //       //   String sKey = jsonIterValue.key;
    //       //   String sValue = jsonIterValue.value;
    //       //   Serial.printf("checkIfDataPathIsExists: %s = %s\n", sKey.c_str(), sValue.c_str());
    //       // }
    //     } else {
    //       // Такого пути нет. Сделаем
    //       FirebaseJson jsonEmpty;
    //       bool result = Firebase.RTDB.setJSON(&fbdo, path, &jsonEmpty);
    //       Serial.printf("checkIfDataPathIsExists. Device data path creation has result: %s\n", result ? "ok" : fbdo.errorReason().c_str());
    //     }
    // }

    void sendSensorsDataToDB(String *arrSensorsAddr, float* arrTemperatures, uint8_t sensorsNumber)  {
        Serial.println(" Push sensors new data to DB.");
        String path = DB_DEVICES_DATA + deviceMAC + DB_SENSOR_TEMPERATURES;
        FirebaseJson json;
        FirebaseJson jsonTime;
        jsonTime.set(".sv", "timestamp");
        json.set("time", jsonTime);

        for(uint8_t i = 0; i < sensorsNumber; i++) {
          json.set(arrSensorsAddr[i], arrTemperatures[i]);
        }
        bool result = Firebase.RTDB.pushAsync(&fbdo, path, &json);
        Serial.printf("  Push data to RTDB ... %s\n\n", result ? "ok" : fbdo.errorReason().c_str());
    }

    void checkIfNeedSendPong() {
      if(!needToSendPong)
        return;
      needToSendPong = false;

      String path = DB_DEVICES_IDS_TYPES + deviceMAC + "/pong_last_ts"; 
      FirebaseJson json;
      FirebaseJson jsonTime;

      jsonTime.set(".sv", "timestamp");
      json.set("time", jsonTime);

      bool result = Firebase.RTDB.setJSON(&fbdo, path, &jsonTime);
      Serial.printf("*** Send PONG to RTDB ... %s\n\n", result ? "ok" : fbdo.errorReason().c_str());
    }
};

#endif /* FIREBASE_MANAGER_H */