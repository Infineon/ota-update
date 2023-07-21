# Flash Layout for OTA

## 1. Introduction

OTA, in concert with MCUBootApp, uses 2 "Slots" in order to update a device. The Primary Slot (also called "boot") is where the currently executing application is located. The Secondary Slot (also called "upgrade") is where we store the new application as it is being downloaded.

| Flash Area Name | Purpose |
|-------------------| ---------------------------- |
| Primary Slot | Location of executing code   |
| Secondary Slot| Location to store the updating code |
| Status Area | MCUBootApp uses this area for keeping track of the SWAP process |
| Scratch Area | MCUBootApp uses this area as a temporary location when swapping Primary and Secondary Slots |

The location of the flash slots is based on the flash available in the system. The layout is defined at application build time and is never changed during the lifetime of the device. The flash layout of the application **must** match the flash layout in MCUBootApp.

In order to get these areas to match, we use a common JSON formatted file to define the flash areas. When building MCUBootApp, use the same JSON file as is used for building the User Application. The python script [flashmap.py](./scripts/flashmap.py) is executed as part of the build. It will check for some common mistakes in JSON files.

All flash erase operations (ex: erasing a slot for a new download) will erase full sectors at a time. PSoC™ 6 MCUs have internal flash that use 512 byte sector sizes. External flash devices will have different sector sizes, from 4k to 256k. If you are going to modify a flash layout, be aware of the sector sizes.

## 2. Internal Flash Only Layout

When the device only has internal flash, all the flash areas must reside in the internal flash. This restricts the size of the application to what can fit in the internal flash space.

Example  :<br>
Target   : CY8CPROTO-062-4343W<br>
Flashmap : [psoc62_2m_int_swap_single.json](./configs/flashmap/psoc62_2m_int_swap_single.json)

**Internal Flash Layout for 2M internal flash**
<br>Note: Internal flash sector size is 512 bytes.
| Address Range | Size | Description |
| --------------|------| -----------|
| 0x10000000-0x10017FFF | 0x00018000 | MCUBootApp code |
| 0x10018000-0x100183FF | 0x00000400 | Primary Slot Required MCUBootHeader (added in signing step of User App) |
| 0x10018400-0x101063FF | 0x000EDC00 | Primary Slot User Application (951k) |
| 0x10106000-0x101063FF | 0x00000400 | Secondary Slot Required MCUBootHeader (added in signing step of User App) |
| 0x10106400-0x101F3FFF | 0x000EDC00 | Secondary Slot User Application (951k)|
| 0x101f4000-0x101F7BFF | 0x00003c00 | MCUBootApp Status Area |
| 0x101f7C00-0x101F8BFF | 0x00001000 | MCUBootApp Scratch Area (4k) |
| 0x101F9C00-0x10200000 | 0x00006400 | Unused (25k) |


## 3. Internal Flash + External Flash Layout

When the device has both internal and external flash, the Secondary Slot and the Swap Area can be moved to external flash, leaving more internal flash for the application code.

Example  :<br>
Target   : CY8CPROTO-062S3-4343W<br>
Flashmap : [psoc62_512k_ext_swap_single.json](./configs/flashmap/psoc62_512k_ext_swap_single.json)

**Internal Flash  + External Flash Layout for 512k Internal Flash**

**Internal Flash**
<br>Note: Internal flash sector size is 512 bytes.
| Address Range | Size | Description |
| --------------|------| -----------|
| 0x10000000-0x10017FFF | 0x00018000 | MCUBootApp code |
| 0x10018000-0x100183FF | 0x00000400 | Primary Slot Required MCUBootHeader (added in signing step of User App) |
| 0x10018400-0x100743FF | 0x0005C000 | Primary Slot User Application (368k) |
| 0x10074400-0x10073E00 | 0x00003c00 | MCUBootApp Status Area |
| 0x10078000-0x10080000 | 0x00008000 | Unused (32k)

**External Flash**
<br>Note: External flash sector size is 256k bytes.
| Address Range | Size | Description |
| --------------|------| -----------|
| 0x18000000-0x180003FF | 0x00000400 | Secondary Slot Required MCUBootHeader (added in signing step of User App) |
| 0x18000400-0x1805C3FF | 0x0005C000 | Secondary Slot User Application (368k)|
| 0x18080000-0x180FFFFF | 0x00080000 | MCUBootApp Scratch Area (512k) |


## 4. Internal Flash + External Flash, XIP from External Flash Layout

When the device has both internal and external flash, and the Primary Slot can reside in external flash, the User Application will run in **XIP** mode (e**X**ecute **I**n **P**lace). The Primary Slot moves into external flash and can have a larger slot size.

Example  :<br>
Target   : CY8CPROTO-062S3-4343W<br>
Flashmap : [psoc62_512k_xip_swap_single.json](./configs/flashmap/psoc62_512k_xip_swap_single.json)

**Internal Flash  + External Flash Layout for 512k Internal Flash using XIP**

**Internal Flash**
<br>Note: Internal flash sector size is 512 bytes.
| Address Range | Size | Description |
| --------------|------| -----------|
| 0x10000000-0x10017FFF | 0x00018000 | MCUBootApp code |
| 0x10018000-0x1001BBFF | 0x00003c00 | MCUBootApp Status Area |
| 0x1001BC00-0x10080000 | 0x00064400 | Unused (401k)

**External Flash**
<br>Note: External flash sector size is 256k bytes.
| Address Range | Size | Description |
| --------------|------| -----------|
| 0x18000000-0x100003FF | 0x00000400 | Primary Slot Required MCUBootHeader (added in signing step of User App) |
| 0x18000400-0x1813FF00 | 0x0013FF00 | Primary Slot User Application (1.25M) |
| 0x18180000-0x181803FF | 0x00000400 | Secondary Slot Required MCUBootHeader (added in signing step of User App) |
| 0x18180400-0x1805C3FF | 0x0013FF00 | Secondary Slot User Application (1.25M)|
| 0x18440000-0x184BFFFF | 0x00080000 | MCUBootApp Scratch Area (512k) |


## 5. External Flash Only Layout

When the device has only external flash, all flash areas must reside in external flash. MCUBootApp and the User Application execute from external flash. The size of the external flash will determine the size of the application.

Example  :<br>
Target   : CYW920829M2EVK-02<br>
Flashmap : [cyw20829_xip_swap_single.json](./configs/flashmap/cyw20829_xip_swap_single.json)

**External Flash Layout for device without Internal Flash**

<br>Note: External flash sector size is 4k bytes.
| Address Range | Size | Description |
| --------------|------| -----------|
| 0x60000000-0x6001FFFF | 0x00020000 | MCUBootApp code |
| 0x60020000-0x600003FF | 0x00000400 | Primary Slot Required MCUBootHeader (added in signing step of User App) |
| 0x60020400-0x6007FFFF | 0x0005FC00 | Primary Slot User Application (368k) |
| 0x60080000-0x608043FF | 0x00000400 | Secondary Slot Required MCUBootHeader (added in signing step of User App) |
| 0x60080400-0x600DFFFF | 0x0005FC00 | Secondary Slot User Application (368k)|
| 0x600E0000-0x600EBFFF | 0x0000C000 | MCUBootApp Status Area |
| 0x600EC000-0x600EDFFF | 0x00002000 | MCUBootApp Scratch Area (512k) |


## 6. Sample Flash Layout(JSON)

Sample flash layout (JSON) available in [OTA Flashmaps](./configs/flashmap/).

For a given target, the same flashmap should be used for [MCUBootApp](./configs/flashmap/MCUBoot_Build_Commands.md) and user ota applications.

