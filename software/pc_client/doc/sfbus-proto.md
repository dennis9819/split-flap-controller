# SplitflapBus protocol

The SF-Bus is an RS485 based protocol. It has a singular master node and up to 32 client nodes per interface.  It can address up to 65536 devices.

The communication is always initiated by the master node. The start of a new communication is signaled by sending the start byte `0x2B` to the bus, followed by the protocol version and frame length. Each connected node now captures the entire packet. This prevents the node from falsely identifyng payload as a start byte if it contains the value `0x2B`.

Starting after the transmission of the payload length, the receiever counts the remaining packages and terminates the transmission at 0 bytes remaining. 

In version 1.0 it expects the last byte to be `0x24`. If this is not the case, the transmission is marked as invalid and will not be processed further. The client will not send any response.

In version 2.0, the checksum is verified. If it does not match the payload, the transmission is marked as invalid and will not be processed further. The client will not send any response.
No stop-byte is expected.

## Packet format (1.0)
```
+------------+-----------+--------+----------+----------------+-----------+
| Start-Byte | Protocol- | Frame- | Address  | Payload        | Stop-Byte |
|            | Version   | Length |          |                |           |
| 0x2B       | 0x00      | 1 byte | 2 bytes  | framelegth - 3 | 0x24      |
+------------+-----------+--------+----------+----------------+-----------+
 The frame length includes the address and stop-byte
 Therefor, the payload is framelegth - 3.
```

## Packet format (2.0)
*This i not yet implemented!!*
```
+------------++----------+--------+----------+----------------+-----------+
| Start-Byte | Protocol- | Frame- | Address  | Payload        | Checksum  |
|            | Version   | Length |          |                |           |
| 0x2B       | 0x00      | 1 byte | 2 bytes  | framelegth - 4 | 2 bytes   |
+------------+-----------+--------+----------+----------------+-----------+
 The frame length includes the address and stop-byte
 Therefor, the payload is framelegth - 4.
```

## Payload format (Protocol Version 1.0 + 2.0)
The first byte of the payload is the command. This tells the controller
what to do or what data to expect next.

```
+----------+----------------+
| Command  | Payload        |
| Byte     |                |
| 1 Byte   | framelegth - 4 |
+----------+----------------+
```


### Ping
Check if a module is present under this device id
- Payload: `0xFE`
- Expected response: `0xFF`

### Reset
Resets the device controller. Should be done after address or calibration change.

Can also be done to clear error flags
- Paylad `0x30`
- Expects no response.

### Motor power on
- Paylad `0x21`
- Expects no response.

### Motor power off
- Paylad `0x20`
- Expects no response.

### Display flap
- Paylad `0x10 <1 byte: flap id>`
- Expects no response.

### Display flap with full rotation
- Paylad `0x11 <1 byte: flap id>`
- Expects no response.

### Read EEPROM
- Payload `0xF0`
- Response is `0xAA` followed by content of EEPROM (5 bytes). See mapping below.

### Write EEPROM
- Payload `0xF1 <5 bytes of eeprom content>`
- Response is `0xAA` followed by new content of EEPROM. See mapping below.

### Get controller status
- Payload `0xF8`
- Response is 7 bytes long.

```
+--------+------------+------------+
| Byte 0 | Byte 1 - 2 | Byte 3 - 6 |
| 8-Bit  | 16-Bit     | 32-Bit     |
| Status | Voltage    | Rotations  |
+--------+------------+------------+
  |         |            |
  |         |            +-> uint32 counter of total rotations
  |         |
  |         +-> uint16 raw ADC readings.
  |             double __voltage = ((double)_voltage / 1024) * 55;
  |
  +->   Bit 0:  delta too big. Sensed position not equal to expected position
        Bit 1:  no home found. Cannot sense home magnet.
        Bit 2:  fuse blown
        Bit 4:  device powered down. Can be changed by sending: 0x21
        Bit 5:  failsafe active. Device failed cirtically and stopped operation.
                can be recovered with reset: 0x30
        Bit 6:  device busy. Device is currently moving to the next flap position

```

## EEPROM format
```
+------------+------------+--------+
| Byte 0 - 1 | Byte 2 - 5 | Byte 6 |
| 16-Bit     | 32-Bit     | 8-Bit  |
| Address    | Rotations  | Okay   |
+------------+------------+--------+
  |             |           |
  |             |           +-> do not change
  |             |
  |             +-> uint32 counter of total rotations
  |
  +-> uint16 device address
```