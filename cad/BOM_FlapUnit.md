# Bill of Material for 1U/2U Flap Unit

## 3D Printed Parts 
| part-number | part | file | amount | notes |
| ----------- | ---- | ---- | ------ | ----- |
| FU-001 | Unit Frame Base     | STL Unit/splitflap-unit-base.stl | 1 | common for all units |
| FU-002 | Unit Frame Cover 1U | STL Unit/splitflap-cover-1u.stl  | 1 | only for 1U wide unit|
| FU-003 | Unit Frame Cover 2U | STL Unit/splitflap-cover-2u.stl  | 1 | only for 2U wide unit|
| FU-004 | Unit Drum Core      | STL Unit/splitflap-drum-core.stl | 1 | common for all units |
| FU-005 | Unit Drum Core 1U   | STL Unit/splitflap-drum-outer-1u.stl | 1 | only for 1U wide unit|
| FU-006 | Unit Drum Core 2U   | STL Unit/splitflap-drum-outer-2u.stl | 1 | only for 2U wide unit|
| FU-007 | Flaps   | ... | 45 | |
- 49 Individual Parts per module

## Hardware

| part-number | part | amount | notes |
| ----------- | ---- | ------ | ----- |
| FU-011      | Ball Bearing 684ZZ (9 x 4 x 4mm) | 1 | |
| FU-012      | Stepper 28BYJ-48 DC 12V | 1 |  **!! Use 12V Variant. NOT 5V !!**|
| FU-013      | Heat insert M3 x 3mm | 3 | |
| FU-014      | Heat insert M3 x 5mm | 4 | |
| FU-015      | M3x5 Screw DIN 912 | 3 | |
| FU-016      | M3x12 Screw DIN 912 | 2 | |
| FU-017      | M3x8 Screw DIN EN ISO 10642 | 2 | |
| FU-018      | Magnet round d=2mm | 1 | |
- 17 Individual Parts per module

## Electronics
| part-number | part | amount | notes |
| ----------- | ---- | ------ | ----- |
| FU-031      | Module Controller PCB | 1 | |
| FU-032      | ATmega8A-A | 1 | |
| FU-033      | SN75176AP | 1 | |
| FU-034      | ULN2003 | 1 | Can be salvaged from the driver board that usually comes with the stepper motor |
| FU-035      | A1101xLH | 1 | SOT-23W variant! |
| FU-036      | 16,0000-HC49U-S Crystal | 1 |  |
| FU-037      | Capacitor 10u 35V (Radial_D6.3mm_P2.50mm) | 1 |  |
| FU-038      | Capacitor 10u 16V (1206) | 1 |  |
| FU-039      | Capacitor 22p (0805) | 2 |  |
| FU-040      | Capacitor 100n (0805) | 4 |  |
| FU-041      | Fuse 200mA (0805) | 1 |  |
| FU-042      | JST B5B-XH-A | 1 |  |
| FU-043      | Resistor 1k | 1 |  |
| FU-044      | Resistor 10k | 3 |  |
- 20 Individual Parts per module


## Summary:
Parts per module: 76 (mostly 3d printed)

# Bill of Material for 4Unit Backplane

## 3D Printed Parts 
| part-number | part | file | amount | notes |
| ----------- | ---- | ---- | ------ | ----- |
| FP-001      | Upper Slot | STL Slot/splitflap-slot-upper.stl | 1 |  |
| FP-002      | Lower Slot | STL Slot/splitflap-slot-lower.stl | 1 |  |
- 2 Individual Parts per module

## Sheet metal
| part-number | part | file | amount | notes |
| ----------- | ---- | ---- | ------ | ----- |
| FP-003      | Backplane | Sheet Metal/sheet-metal-backplate.dxf | 1 |  |
- 1 Individual Parts per module

## Hardware
| part-number | part | amount | notes |
| ----------- | ---- | ------ | ----- |
| FP-011      | Heat insert M4 x 5mm | 4 | |
| FP-012      | Heat insert M3 x 5mm | 4 | |
| FP-013      | M3x5 Screw DIN 912 | 8 | |
| FP-014      | M4x10 Screw DIN 912 | 4 | |
| FP-015      | M3x5 Standoff | 8 | |
| FP-016      | M3 Nut | 4 | |
- 32 Individual Parts per module

## Electronics
| part-number | part | amount | notes |
| ----------- | ---- | ------ | ----- |
| FU-031      | Backplane PCB | 1 | |
| FU-032      | PhoenixContact_MSTBA_2,5_2-G-5,08 | 2 | |
| FU-033      | Phoenix_MSTB:PhoenixContact_MSTBA_2,5_3-G-5,08 | 4 | |
| FU-034      | L7805 | 1 |  |
| FU-035      | 74AHCT1G125 | 1 | |
| FU-036      | 2x08 Edge Connector | 4 |  |
- 13 Individual Parts per module

## Summary:
Parts per assembly: 48 

# Bill of Material for Backplane connection bracket
## Sheet metal
| part-number | part | file | amount | notes |
| ----------- | ---- | ---- | ------ | ----- |
| FC-001      | Bracket | sheet-metal-connector-middle.dxf | 1 |  |
- 1 Individual Parts per module

## Hardware
| part-number | part | amount | notes |
| ----------- | ---- | ------ | ----- |
| FC-011      | M4x10 | 8 | |
| FC-012      | M4 Nut | 8 | |
- 16 Individual Parts per module

## Summary:
Parts per assembly: 17 

# Parts for full display (12x3)
- Display Units: 36 (2736 Parts)
- Backplane Units: 9 (432 Parts)
- Backplane Connectors: 12 (204 Parts)
- Sum: 3372 Individual Parts
