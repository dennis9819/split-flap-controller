# 3D Printable split flap display
This is a 3D printable, expandable split flap display based on the original design of [David Königsmann](https://github.com/davidkingsman/split-flap).
This version uses his drum and flap design but completly redesigns the electrical system.
Notable changes:
* New, more rigid case design.
* Hot-swappable modules
* New Backplane
* New PCB, using mostly smd components and an atmega8 mcu.
* RS-485 bus
* Custom leightweight protocol
* Controller, running an WebSockets-Server.
* Homing-Failure and overcurrent detection
* Address and calibration data is stored in EEPROM

The firmware as well as the Server is completly rewritten wrfrom scratch in plain c and can easily be ported to other mcu's.

__Ongoing Project! This repository is not final.__

## Websockets Server
### Satus request of all devices
Request:
```
{"command":"status"}
```
Response:
```
{
  "devices_all":2,
  "devices":[
    {
      "id":0,
      "address":0,
      "status":{
        "voltage":11.6015625,
        "rotations":4,
        "power":true,
        "raw":0,
        "device":"ONLINE",
        "flags":{
          "errorTooBig":false,
          "noHome":false,
          "fuseBlown":false,
          "homeSense":false,
          "powerDown":false,
          "failSafe":false,
          "busy":false
        }
      }
    },
    {
      "id":1,
      "address":1,
      "status":{
        "voltage":0.0,
        "rotations":0,
        "power":true,
        "raw":0,
        "device":"OFFLINE",
        "flags":{
          "errorTooBig":false,
          "noHome":false,
          "fuseBlown":false,
          "homeSense":false,
          "powerDown":false,
          "failSafe":false,
          "busy":false
        }
      }
    }
  ],
  "devices_online":1
}
```