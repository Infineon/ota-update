# Infineon Over The Air (OTA) Library

## What's Included?

See the [README.md](./README.md) for a complete description of the OTA library.

## Changelog

### v4.3.1

- Minor Documentation updates.

### v4.3.0

- Added support for CY8CEVAL-062S2-CYW955513SDM2WLIPA kit.

### v4.2.1

- Minor Documentation updates.

### v4.2.0

- Added support for CYW955913EVK-01 kit.

### v4.1.0

- Added support for CYW89829(CYW989829M2EVB-01) kits.
- IAR support added for 20829 platform.

### v4.0.0

- De-coupled MCUBootloader related code from ota-update library.
- Added user callbacks for bootloader specific storage APIs.
- Added Ethernet Interface support.
- Added support for XMC7200(KIT_XMC72_EVK) kits.
- Abstracted GATT related code from OTA BLE APIs.
- Documentation updates.

### v3.0.1

- Template OTA linker file updated for 20829B0 platform.

### v3.0.0

- Flash memory operations(Read/Write/Erase) offloaded to user application.
- Added support for configurable MTU sizes during BLE OTA.
- MCUBootloader-specific makefile entries are handled within the library.
- Integrated MQTT-v4.X library version.
- Added support for 20829B0(CYW920829M2EVK-02) kits.
- Documentation updates.

### v2.0.0

- Support for ModusToolbox 3.0 build infrastructure
  - ota-update v2.0.0 works with ModusToolbox 3.0 and is not backwards compatible with ModusToolbox 2.4.
  - ota-update v1.0.0 works with ModusToolbox 2.4 and replaces anycloud-ota (all versions).
- New support
    - CY8CPROTO-062S3-4343W WiFi and BT updates using XIP for all Toolchains.
- New Support for these devices:
    - CY8CKIT-062-BLE
    - CY8CPROTO-063-BLE
    - CYBLE-416045-EVAL

### v1.0.0

- New ota-update library
- This ota-update v1.0.0 replaces AnyCloud-ota (all versions).
- The anycloud-ota vx.x.x library will be depricated soon.
- ota-update v1.0.0 works with MTB 2.4
- Legacy support for these devices:
    - CY8CKIT-062S2-43012
    - CY8CKIT-064B0S2-4343W
    - CY8CPROTO-062-4343W
    - CY8CPROTO-062S3-4343W
    - CY8CEVAL-062S2-CYW943439M2IPA1
    - CY8CEVAL-062S2-LAI-4373M2
    - CY8CEVAL-062S2-MUR-43439M2
    - CYW920829M2EVB-01

- New support
    - CY8CPROTO-062S3-4343W now supports WiFi updates using XIP
- XIP builds are only supported with GCC and ARM Toolchains

## Supported Software and Tools
This version of the library was validated for compatibility with the following software and tools:

| Software and Tools                                        | Version |
| :---                                                      | :----:  |
| ModusToolbox&trade; Software Environment                  | 3.2     |
| - ModusToolbox&trade; Device Configurator                 | 4.20    |
| - ModusToolbox&trade; Bluetooth Configurator              | 2.90    |
| - ModusToolbox&trade; CapSense Configurator / Tuner tools | 6.20.0  |
| Peripheral Driver Library (PDL CAT1)                      | 3.12.0  |
| GCC Compiler                                              | 11.3.1  |
| IAR Compiler                                              | 9.40.2  |
| Arm Compiler 6                                            | 6.16    |
