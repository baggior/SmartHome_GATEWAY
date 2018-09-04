# SmartHome_GATEWAY

Gateway Module.

1. Bridge between general RS485 serial protocol to WiFi REST
1. Bridge between MODBUS RTU / ASCII to to WiFi REST

---

## Modbus REST API

A service that exposes the Modbus protocol via a RESTful API.
The API is as follows:

- GET: /Read/{deviceType}:
  - Reads a number of Modbus registers.
  - The {devicetype} could be either:
    - "coils"
    - "holdingregisters"
    - "inputregisters"
    - "inputs"
  - The body will follow the format below
    ```json
    {
        "destination": "ip:portnumber or serial address",
        "connectiontype": "0 for serial RTU, 1 for serial ASCII",
        "slaveid": "slave address as a number",
        "address": "offset number where we want to start the reads",
        "count": "number of registers to read"
    }  
    ```
- POST: /Write/{devicetype}:
  - Perform a write to a Modbus register. The {devicetype} could be either coils or holdingregisters
  - The body will follow the format below
    ```json
    {
       "destination": "ip:portnumber or serial address",
        "connectiontype": "0 for serial RTU, 1 for serial ASCII",
        "slaveid": "slave address as a number",
        "address": "register offset number where we want to start the reads",
        "data": "the data to write to the device. This will either be an array of bools or ushorts"
    }  
    ```

### Examples

#### GET: read

the slave id is 1, and we want to read starting from register address 1414 for five registers.
Here is how the request will the body look like:

```json
{
    "destination": "COM3",
    "connectiontype": 2,
    "slaveid": 1,
    "address": 1414,
    "count": 5
}
```  

#### POST: write

the slave id is 1, we want to write to holding registers starting from register address 1414 values [14,21,12]. The request will go to /Write/holdingregisters

```json
{
    "destination": "COM3",
    "connectiontype": 2,
    "slaveid": 1,
    "address": 1414,
    "data": [14,21,12]
}
```  
