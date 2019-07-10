# SmartHome_GATEWAY

Gateway Module.

1. Bridge between MODBUS RTU / ASCII to ModbusTCP via WiFi

---

## SPIFFS explorer
http://espressif.local/spiffs/\<spiffs filename\>

## SPIFFS editor
http://espressif.local/edit

## Modbus REST API
A service that exposes the RESTful API as follows:

Method: GET, Base url: http://espressif.local/api/
-   plain/restart
    -   Restarts the device
-   plain/heap
    -   shows current available free heap in text plain
-   plain/info
    -   shows Device info
-   echo
    -   logs the request
-   heap
    -   shows current available free heap in JSON
-   config
    -   shows current JSON configuration
-   scanWifi
    -   shows scanned Wifi networks available
-   firmwareUpdate
    -   presents a form to update HEX firmware

