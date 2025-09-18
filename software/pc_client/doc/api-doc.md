# Websockets Interface documentation
All requests and responses are sent as json objects.

## Request
Every request must conatin at least one value: `command`
```
{
   "command": "<command>"
}	
```
### Device manager commands

#### Save config `dm_save`
Saves config to ./flapconfig.json.

Request:
```
{
   "command": "dm_load"
}	
```
Response:
```
{
   "ack": true
}	
```
#### Load config `dm_load`
Loads config from ./flapconfig.json.

Request:
```
{
   "command": "dm_load"
}	
```
Response:
```
{
   "ack": true
}	
```
#### Dump config `dm_dump`
Dumps current config to socket.

Request:
```
{
   "command": "dm_dump"
}	
```
Response:
```
{
   "devices_all": <registered device count>
   "devices_online": <online device count>,
   "devices": <array of all devices>,
   "map": <2D array of device locations>
}	
```

#### Describe single device `dm_describe`
Gets all information for specified device id.

Request:
```
{
   "command": "dm_describe",
   "id": <device id>
}	
```
Response:
```
{
   ...
}	
```
#### Register new device `dm_register`
Register device, assign new id and assign a location.
```
{
   "command": "dm_describe",
   "address": <device nus address>,
   "x": <x position on screen>,
   "y": <y position on screen>,
}	
```
Response:
```
{
   "id": <assigned device id>
}	
```

#### Remove device from config `dm_remove`
Removes device from config and frees location in screen

Request:
```
{
   "command": "dm_remove",
   "id": <device id>
}	
```
Response:
```
{
   "ack": true
}	
```

#### Remove device from config `dm_refresh`
Refresh device config

Request:
```
{
   "command": "dm_refresh"
}	
```
Response:
```
{
   "devices_online": <online device count>
}	
```

### Device raw commands

#### Ping module `dr_ping`
Checks if a module reponds on the given address.

Request:
```
{
   "command": "dr_ping",
   "address": <address>
}	
```
Response:
```
{
   "success": <boolean: if device reponded 'true', else 'false'>
}	
```

#### Set module address `dr_setaddress`
Changes the hardware address of an module.

Request:
```
{
   "command": "dr_setaddress",
   "address": <address>,
   "newaddress": <new address>
}	
```
Response:
```
{
   "success": <boolean: if success 'true', else 'false'>
}	
```

#### Set module offset calibration `dr_setcalibration`
Sets the offset calibration for an module.

Request:
```
{
   "command": "dr_setcalibration",
   "address": <address>,
   "calibration": <calibration value (default: 1800)>
}	
```
Response:
```
{
   "success": <boolean: if success 'true', else 'false'>
}	
```

#### Reset module `dr_reset`
Resets the controller of an module

Request:
```
{
   "command": "dr_reset",
   "address": <address>
}	
```
Response:
```
{
   "ack": true
}	
```

#### Display flap `dr_display`
Sets the module to the specified flap directly or with recalibration.

Request:
```
{
   "command": "dr_display",
   "address": <address>,
   "flap": <flap-number>,
   ("full": <boolean: full rotation of drum/recalibration>)

}	
```
Response:
```
{
   "ack": true
}	
```

#### Power module on/off `dr_power`
Sets the power-state for the motor of the given module.

Request:
```
{
   "command": "dr_power",
   "address": <address>,
   "power": <boolean: power on>

}	
```
Response:
```
{
   "ack": true
}	
```

## Responses
### Error:
```
{
   "error": "<error name>",
   "detail": "<more detailed error message>"
}	
```
The following responses are valid:
- `parsing error`: the json request is malformated and cannot be parsed
- `format error`: the json object is missing required fields. Check details for more information.
- `internal error`: an internal error occured. This should not happen. Chaeck server logs.

