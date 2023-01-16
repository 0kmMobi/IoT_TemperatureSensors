#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "eeprom_storage.h"
#include "consts.h"
#if __has_include("secret_real.h")
  #include "secret_real.h"
#else
  #include "secret_dummy.h"
#endif

ESP8266WebServer server(WEB_SERVER_PORT);

int8_t networksFound = 0;

String printNetworksArround() {
  String sRes = "";
  if(networksFound == 0)
    return sRes;

  sRes += networksFound;
  sRes += " network(s) found\n";
  for (int i = 0; i < networksFound; i++) {
    sRes += WiFi.SSID(i) + "\t";
    sRes += WiFi.channel(i);
    sRes += "\t";
    sRes += WiFi.RSSI(i);
    sRes += "\t";
    sRes += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "open" : "";
    sRes += "\t";
    sRes += WiFi.isHidden(i) ? "hidden":"";
    sRes += "\n";
  }
  Serial.println(sRes);
  return sRes;
}

void handle_info() {
  String sRes = printNetworksArround();
  server.send(200, "text/plain", sRes);
}

void handle_set() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  String sAnswer = "";
  bool hasError = true;
  if(ssid.isEmpty() || pass.isEmpty()) {
    sAnswer = "ERROR: One or more parameters was not defined";
  } else if(pass.length() < 8) {
    sAnswer = "ERROR: The passkey length is less than 8";
  } else if(pass.length() > 32) {
    sAnswer = "ERROR: The passkey length is more than 32";
  } else {
    // Check SSID contains in founded networks
    bool ssidFound = false;
    for (int i = 0; i < networksFound; i++) {
        if(ssid.equals(WiFi.SSID(i))) {
          ssidFound = true;
          break;
        }
    }
    if(ssidFound)
      hasError = false;
    else
      sAnswer = "ERROR: The SSID is not contains in found networks list";
  }

  if(hasError)
    server.send(200, "text/plain", sAnswer);
  else {
    server.send(200, "text/plain", "Ok");

    // Сохраняем данные
    eepromStorage.clear();
    eepromStorage.write(ssid, pass);
    ESP.restart();
  }
}


void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}


class WifiWebServer {
  private:
    String getCompressedMAC() {
      uint8_t mac[6];
      WiFi.macAddress(mac);

      char macStr[18] = { 0 };
      sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      return String(macStr);
    }


  public:
    uint32_t webServerWaitingTimer;
    

    WifiWebServer() {
      IPAddress local_ip;
      IPAddress gateway;
      IPAddress subnet;

      local_ip.fromString(WIFI_AP_LOCALIP);
      gateway.fromString(WIFI_AP_GATEWAY);
      subnet.fromString(WIFI_AP_NETMASK);

      networksFound = WiFi.scanNetworks(false, true);
      Serial.printf("Found %d Wifi-networks.\n", networksFound);
      printNetworksArround();

      Serial.print("MAC address = ");
      String macAddr = WiFi.softAPmacAddress();
      Serial.println(macAddr);


      Serial.print("Setting soft-AP configuration ... ");
      Serial.println(WiFi.softAPConfig(local_ip, gateway, subnet) ? "Ready" : "Failed!");

      String ssid = WIFI_AP_SSID_BASE + getCompressedMAC();
      Serial.printf("Soft-AP SSID: %s; Pass: %s\n", ssid.c_str(), WIFI_AP_PASSWORD);
      Serial.print("Setting soft-AP ... ");
      Serial.println(WiFi.softAP(ssid, WIFI_AP_PASSWORD) ? "Ready" : "Failed!");

      Serial.print("Soft-AP IP address = ");
      Serial.println(WiFi.softAPIP());

      delay(100);

      server.on("/info", handle_info);
      server.on("/set", handle_set);
      server.onNotFound(handle_NotFound);
      
      server.begin();
      Serial.println("HTTP server started");

      webServerWaitingTimer = 0;
    }

    ~WifiWebServer() {
      server.stop();
    }

    bool updateAndGetTimeIsOut( uint32_t dt) {
      server.handleClient();
      webServerWaitingTimer += dt;
      if(webServerWaitingTimer > WEBSERVER_WAITING_TIMER_MSEC_MAX) {
          return true;
      }
      return false;
    }

    //
    // void showConnectedDevices() {
    //   uint8_t numClientsNew = WiFi.softAPgetStationNum();
    //
    //   auto client_count = wifi_softap_get_station_num();
    //   Serial.printf("Total devices connected = %d\n", client_count);
    //
    //   auto i = 1;
    //   struct station_info *station_list = wifi_softap_get_station_info();
    //   while (station_list != NULL) {
    //       auto station_ip = IPAddress((&station_list->ip)->addr).toString().c_str();
    //       char station_mac[18] = {0};
    //       sprintf(station_mac, "%02X:%02X:%02X:%02X:%02X:%02X", MAC2STR(station_list->bssid));
    //       Serial.printf("%d. %s %s", i++, station_ip, station_mac);
    //       station_list = STAILQ_NEXT(station_list, next);
    //   }
    //   wifi_softap_free_station_info();
    // }
};