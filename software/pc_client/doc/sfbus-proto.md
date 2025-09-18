# SplitflapBus protocol
## Terminology
* *main controller interface*: RS485 interface, connected to the mangment pc. Serves as bus master.
* *node*: Participant on bus. Can be a controller interface or an flap controller.
* *flap controller*: Microcontroller located in each flap module that processes requests and drives motor.

## Protocol implementation
The SF-Bus is an RS485 based protocol. It has a singular master node and up to 32 client *nodes* per interface.  It can address up to 65536 *devices*.

The communication is always initiated by the *main controller interface*.
The start of a new communication is signaled by sending the start byte `0x2B` to the bus, followed by the protocol version (8-bit) and frame length (8-Bit). Each connected *node* now captures the entire packet, even if the adress does not match its own. This reduces the risk of the *node* falsely identifying payload data as a start byte if it contains the value `0x2B`.

After the transmission of the payload length, the receiver *node* counts the remaining packages and terminates the transmission at 0 bytes remaining. The data is written to a buffer for further processing.

In version 1.0, the receiving *node* expects the last byte to be `0x24`. If this is not the case, the transmission is marked as invalid and will not be processed further. The receiving *node* will not send any response.

In version 2.0, the checksum is verified. If it does not match the payload, the transmission is marked as invalid and will not be processed further. The receiving *node* will not send any response.
No stop-byte is expected. Version 2.0 also requires a timeout-check in the receiving function. This allows the bus to support hot-plugging, to cancel incomplete or invalid packages and reliably detect the start of a new package.

The communication is typically unidirectional. Lost transmissions are not detectable. The *flap controller* NEVER initiates a communication to the *main controller interface*. *node* to *master* communication only occures as a response to specific commands. This is specified in the command / payload documentation.

## Packet format (1.0)
```
+---------------------------------+----------------------------------------+
| Header                          | Frame                                  |
+------------+-----------+--------+----------+-----------------+-----------+
| Start-Byte | Protocol- | Frame- | Address  | Payload         | Stop-Byte |
|            | Version   | Length |          |                 |           |
| 0x2B       | 0x00      | 1 byte | 2 bytes  | framelemgth - 3 | 0x24      |
+------------+-----------+--------+----------+-----------------+-----------+
 The frame length includes the address and stop-byte
 Therefor, the payload is framelegth - 3.
```

## Packet format (2.0)
*This is not yet implemented!!*
```
+---------------------------------+----------------------------------------+
| Header                          | Frame                                  |
+------------+-----------+--------+----------+-----------------+-----------+
| Start-Byte | Protocol- | Frame- | Address  | Payload         | Checksum  |
|            | Version   | Length |          |                 |           |
| 0x2B       | 0x01      | 1 byte | 2 bytes  | framelength - 4 | 2 bytes   |
+------------+-----------+--------+----------+-----------------+-----------+
 The frame length includes the 16-bit address and 16 bit checksum.
 Therefor, the payload is framelegth - 4.

 Checksum is based on MODBUS CRC algorithm. Check implementation in sfbus.c
```

More infromation regarding the crc algorithm: https://ctlsys.com/support/how_to_compute_the_modbus_rtu_message_crc/

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