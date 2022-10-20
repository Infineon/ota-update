# MCUBoot Build Commands

The JSON flash maps were verified to work with MCUBoot v1.8.1-cypress. The JSON flash map files reside here: **\<out-update\>/configs/flashmap/**

Choose the configuration to use from the information below and copy the flash map file to your **\<mcuboot\>/boot/cypress/** directory.

## 1. PSoC™ 62 2M Internal Flash Platforms
- CY8CKIT-062S2-43012
- CY8CPROTO-062-4343W
- CY8CEVAL-062S2-MUR-43439M2
- CY8CEVAL-062S2-LAI-4373M2
- CY8CEVAL-062S2-CYW943439M2IPA1

UART differences

The UART Tx and Rx pins are common across most TARGET boards. However, there are exceptions. If you see not "printf()" output to the terminal, check the UART settings. The UART defines can be changed for your device. Edit <mcboot>/boot/cypress/MCUBootApp/config/custom_debug_uart_cfg.h and set the defines as appropriate for your device, and include "USE_CUSTOM_DEBUG_UART=1" to the make command.

There are flash maps for the upgrade slot in internal flash and in external flash. There are also versions for SWAP and OVERWRITE upgrade methods.

<b>Building MCUBoot</b>

To build MCUBoot for internal only flash SWAP configuration:

```
make clean app APP_NAME=MCUBootApp PLATFORM=PSOC_062_2M FLASH_MAP=./psoc62_2m_int_swap_single.json
```

To build MCUBoot for internal only flash OVERWRITE configuration:

```
make clean app APP_NAME=MCUBootApp PLATFORM=PSOC_062_2M FLASH_MAP=./psoc62_2m_int_overwrite_single.json
```

To build MCUBoot for internal + external flash SWAP configuration:

```
make clean app APP_NAME=MCUBootApp PLATFORM=PSOC_062_2M FLASH_MAP=./psoc62_2m_ext_swap_single.json
```

To build MCUBoot for internal + external flash OVERWRITE configuration:

```
make clean app APP_NAME=MCUBootApp PLATFORM=PSOC_062_2M FLASH_MAP=./psoc62_2m_ext_overwrite_single.json
```

To build MCUBoot for external flash execute (XIP) SWAP configuration:

```
make clean app APP_NAME=MCUBootApp PLATFORM=PSOC_062_2M FLASH_MAP=./psoc62_2m_xip_swap_single.json
```

To build MCUBoot for external flash execute (XIP) OVERWRITE configuration:

```
make clean app APP_NAME=MCUBootApp PLATFORM=PSOC_062_2M FLASH_MAP=./psoc62_2m_xip_overwrite_single.json
```

## 2. PSoC™ 62 512K Internal Flash Platforms
- CY8CPROTO-062S3-4343W

<b>Building MCUBoot</b>

To build MCUBoot for internal + external flash OVERWRITE configuration:

```
make clean app APP_NAME=MCUBootApp PLATFORM=PSOC_062_512K USE_CUSTOM_DEBUG_UART=1 FLASH_MAP=./psoc62_512k_ext_overwrite_single.json
```

To build MCUBoot for internal + external flash SWAP configuration:

```
make clean app APP_NAME=MCUBootApp PLATFORM=PSOC_062_512K USE_CUSTOM_DEBUG_UART=1 FLASH_MAP=./psoc62_512k_ext_swap_single.json
```

To build MCUBoot for external flash execute (XIP) SWAP configuration:

```
make clean app APP_NAME=MCUBootApp PLATFORM=PSOC_062_512K USE_CUSTOM_DEBUG_UART=1 FLASH_MAP=./psoc62_512k_xip_swap_single.json
```

## 3. PSoC™ 64B0 Platform
- CY8CKIT-064B0S2-4343W

### Building MCUBoot

There are no flash maps for this device, the parameters are in the Makefile. We use cysecuretools to provision CY8CPROTO-064B0S2-4343W targets. Please see OTA_MCUBOOTAPP_README.md for more details.


## 4. CYW20829 Platforms

The CYW920829M2EVB-01 platform only supports an external flash execution  (XIP) configuration.

### Building MCUBoot

To build MCUBoot for external flash execute (XIP) SWAP configuration:

```
make clean app APP_NAME=MCUBootApp PLATFORM=CYW20829 USE_CUSTOM_DEBUG_UART=1 USE_EXTERNAL_FLASH=1 USE_XIP=1 FLASH_MAP=./cyw20829_xip_swap_single.json
```

To build MCUBoot for external flash execute (XIP) OVERWRITE configuration:

```
make clean app APP_NAME=MCUBootApp PLATFORM=CYW20829 USE_CUSTOM_DEBUG_UART=1 USE_EXTERNAL_FLASH=1 USE_XIP=1 FLASH_MAP=./cyw20829_xip_overwrite_single.json
```

## 5. Document History

| Document Version | Description of Change                                      |
| ---------------- | ---------------------------------------------------------- |
| 1.0.1            | Add UART information                                       |
| 1.0.0            | New middleware library                                     |



© 2022, Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
This software, associated documentation and materials ("Software") is owned by Cypress Semiconductor Corporation or one of its affiliates ("Cypress") and is protected by and subject to worldwide patent protection (United States and foreign), United States copyright laws and international treaty provisions. Therefore, you may use this Software only as provided in the license agreement accompanying the software package from which you obtained this Software ("EULA"). If no EULA applies, then any reproduction, modification, translation, compilation, or representation of this Software is prohibited without the express written permission of Cypress.
Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress reserves the right to make changes to the Software without notice. Cypress does not assume any liability arising out of the application or use of the Software or any product or circuit described in the Software. Cypress does not authorize its products for use in any products where a malfunction or failure of the Cypress product may reasonably be expected to result in significant property damage, injury or death ("High Risk Product"). By including Cypress's product in a High Risk Product, the manufacturer of such system or application assumes all risk of such use and in doing so agrees to indemnify Cypress against all liability.
