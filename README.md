# IoT Temperature Controller

Temperature controller is part of smart home infrastructure.

## Description

<img height="480" src="/_readmi-res/elements.png">

The main features of the IoT controller:
1. Based on ESP8266/ESP32. It connects via Wifi to a router for Internet access;
2.One or more DS18B20 sensors can be connected to the controller via 1-Wire;
3. Sensors data is periodically sent to the Firebase RealTime DataBase;
4. To initialization Wifi network parameters (SSID and Passkey), the device switches to Wifi-AP mode and starts Web server. The user, through a mobile application, connects to the WiFi controller and transfers the Wifi network parameters to it to access the Internet;
5. The Ping-Pong scheme allows the user to know if the device is online.

[To manage the IoT devices of this smart home system, there is a special mobile cross-platform application](https://github.com/0kmMobi/iots_manager).

