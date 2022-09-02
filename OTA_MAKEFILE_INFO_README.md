# OTA Makefile Settings for a User Application

## 1. Introduction

This guide shows Makefile variables and build DEFINES used when building an application with ota-update library support.


## 2. Standard Application Makefile Defines


| Makefile Variable | Example | Description |
| ----------------- | ------- | ----------- |
| TARGET?=\<target_name\> | TARGET?=CY8CPROTO-062-4343W | Target Kit for build |
| CORE?=\<core_type\> | CORE?=CM4 | Core CPU for Application<br>(CM33, CM4) |
| APPNAME?=\<application_name\> | APPNAME=ota-test | Name of Application |
| TOOLCHAIN?=\<toolchain\> | TOOLCHAIN?=GCC_ARM | GCC_ARM (default)<br>ARM<br> IAR |
| CONFIG?=\<build_level\> | CONFIG?=Debug | Build level<br>- Debug<br>- Release |

## 3. OTA Specific Makefile Defines

| Makefile addition | Required | Default if not defined | Description |
| ----------------- | -------- | ---------------------- | ----------- |
| OTA_FLASH_MAP=<flash_map.json> | Yes | Error | JSON file passed to flashmap.py that generates flashmap.mk. The JSON file defines:<br>- Internal / external flash usage<br>- Flash area location and sizes<br>- Number of images / slots<br>- XIP (from external flash) if defined |
| OTA_HTTP_SUPPORT=<0,1> | No | Depends on Target Support | Include OTA support for update transfers using HTTP directly to a server.<br>App can use Job or Direct update flow Pull Model. |
| OTA_MQTT_SUPPORT=<0,1> | No | Depends on Target Support | Include OTA support for MQTT Broker transfers over HTTP. Implies HTTP library support<br>App can use Job or Direct update flow Pull Model. |
| OTA_BT_SUPPORT=<0,1> | No | Depends on Target Support | Include BT Support for transfers over BT/BLE/BLESS<br>Push Model. |
| OTA_BT_SECURE=<0,1> | No | Requires BT Support | Include BT TLS secure connection support |
| OTA_APP_VERSION_MAJOR=\<application_version\> | No | 0 | Application Major version number  X.y.z|
| OTA_APP_VERSION_MINOR=\<application_version\> | No | 9 | Application Minor version number  x.Y.z|
| OTA_APP_VERSION_BUILD=\<application_version\> | No | 0 | Application Build version number x.y.Z |
| OTA_TEST_APP_VERSION_IN_TAR=\<0,1\> | No | 0 | Set to 1 to enable checking application version in TAR file in OTA library when updating using a TAR file. |
| OTA_HTTP_USER_AGENT_VALUE=\<string\> | No | "\"ota-http-client\"" | Passed to HTTP library as:<br> DEFINES += HTTP_USER_AGENT_VALUE=$(OTA_TEST_APP_VERSION_IN_TAR)<br>Used in HTTP connection. |
| OTA_HTTP_MAX_HDR_SIZE=\<size\> | No | 2048 | Passed to HTTP library as:<br>DEFINES += HTTP_MAX_RESPONSE_HEADERS_SIZE_BYTES=$(OTA_HTTP_MAX_HDR_SIZE) |
| OTA_HTTP_USE_CUSTOM_CONFIG=\<0,1\> | No | 0 | Passed to HTTP library as:<br>   ifneq ($(OTA_HTTP_USE_CUSTOM_CONFIG),0)<br>DEFINES += HTTP_DO_NOT_USE_CUSTOM_CONFIG<br>endif |
| OTA_MQTT_COMPANY_TOPIC_PREPEND=\<string\> | No | "\"OTAUpdate\"" | Configurable part of MQTT topic name. Must be enclosed in <slash-quote> to compile correctly. |
| OTA_PLATFORM=<platform_type> | Yes | Error | <platform_type> must be one of:<br>CYW20829 - ex: CYW920829M2EVB-01<br>PSOC_062_512K - ex: CY8CPROTO-062S3-4343W<br>PSOC_062_2M    - ex: CY8CPROTO-062-4343W<br>PSOC_064_2M    - ex: CY8CKIT-064B0S2-4343W |
| CY_XIP_SMIF_MODE_CHANGE=\<0,1\> | No | 0 | For PSoC™ 62 devices, this is required to be 1.<br>For CYW20829 devices this is required to be 0. |
| CYHAL_DISABLE_WEAK_FUNC_IMPL=<0,1> | No | 0 | Setting to 1 allows for RAM functions necessary to use SMIF functionality while executing from XIP (external flash) to be used properly on PSoCtm targets. <br>For PSoC™ 62 devices, this is required to be 1.<br>For CYW20829 devices this is required to be 0.|



#### Notes:
- Application Version:
    - Used to verify that the new version is higher than the previous version in the Job Flow.
    - Used in messaging with MQTT Broker, and testing MQTT chunks.




## 4. Document History

| Document Version | Description of Change |
| ---------------- | ---------------------------------------------------------- |
| 1.0.0            | Initial Document                      |

------

© 2022, Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
This software, associated documentation and materials ("Software") is owned by Cypress Semiconductor Corporation or one of its affiliates ("Cypress") and is protected by and subject to worldwide patent protection (United States and foreign), United States copyright laws and international treaty provisions. Therefore, you may use this Software only as provided in the license agreement accompanying the software package from which you obtained this Software ("EULA"). If no EULA applies, then any reproduction, modification, translation, compilation, or representation of this Software is prohibited without the express written permission of Cypress.
Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress reserves the right to make changes to the Software without notice. Cypress does not assume any liability arising out of the application or use of the Software or any product or circuit described in the Software. Cypress does not authorize its products for use in any products where a malfunction or failure of the Cypress product may reasonably be expected to result in significant property damage, injury or death ("High Risk Product"). By including Cypress's product in a High Risk Product, the manufacturer of such system or application assumes all risk of such use and in doing so agrees to indemnify Cypress against all liability.
