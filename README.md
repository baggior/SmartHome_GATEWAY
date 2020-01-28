# SmartHome_GATEWAY

Gateway Module.

Bridge between MODBUS RTU / ASCII to ModbusTCP via WiFi

---

## Remote Debug

Control debug messages remotely:

1. access via wbsocket
   - using app: <http://joaolopesf.net/remotedebugapp/>
   - on port **8232** -> <http://espressif.local:8232>
2. access via TELNET on port 23

---

## SPIFFS explorer

Access SPIFFS files

- URL: <http://espressif.local/spiffs/>\<spiffs filename\>

## SPIFFS editor

- URL: <http://espressif.local/edit>
- username: esp
- password: esp

---

## Modbus REST API

A service that exposes the RESTful API as follows:

Method: GET, Base url: http://espressif.local/api/

- plain/restart
  - Restarts the device
- plain/heap
  - shows current available free heap in text plain
- plain/info
  - shows Device info
- echo
  - logs the request
- heap
  - shows current available free heap in JSON
- config
  - shows current JSON configuration
- scanWifi
  - shows scanned Wifi networks available
- firmwareUpdate
  - presents a form to update HEX firmware

---
