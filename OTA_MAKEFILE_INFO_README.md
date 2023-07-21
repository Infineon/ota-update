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
| CY_BUILD_LOCATION=\<Application's Build Working Directory path\> | Yes | Error | Required by app_flashmap.py for setting output directory for use in sign_script.bash script. |
| OTA_SUPPORT=<0,1> | Yes | Error | Must be set to 1 to add OTA defines, sources, and libraries. |
| OTA_PLATFORM=<platform_type> | Yes | Depends on Target Support | <platform_type> must be one of:<br>CYW20829 - ex: CYW920829-KEYBOARD and CYW920829-MOUSE<br>PSOC_062_512K - ex: CY8CPROTO-062S3-4343W<br>PSOC_062_1M - ex: CY8CKIT-062-BLE<br>PSOC_062_2M    - ex: CY8CPROTO-062-4343W<br>PSOC_063_1M - ex: CY8CPROTO-063-BLE<br>PSOC_064_2M    - ex: CY8CKIT-064B0S2-4343W <br>Default value is set for officially supported kits. For reference kit value should be set. |
| OTA_FLASH_MAP=<flash_map.json> | No | Depends on Target Support | Default flash_maps are available [here](./configs/flashmap) for supported targets.<br>If this makefile entry is empty then ota-update library uses target default flash map for generating flashmap.mk.<br>JSON file passed to flashmap.py that generates flashmap.mk. The JSON file defines:<br>- Internal / external flash usage<br>- Flash area location and sizes<br>- Number of images / slots<br>- XIP (from external flash) if defined |
| OTA_LINKER_FILE=<ota_linker_file> | Yes | Error | Based on selected target, Create OTA linker file for XIP or Non XIP mode. <br> Refer [OTA Linker file Readme](./OTA_MCUBOOT_LINKER_FILES_README.md) for details. |
| OTA_HTTP_ONLY=<0,1> | No | Depends on Target Support | Include OTA support for update transfers using ONLY HTTP protocol.<br>App can use Job or Direct update flow Pull Model. |
| OTA_HTTP_SUPPORT=<0,1> | No | Depends on Target Support | Include OTA support for update transfers using HTTP directly to a server.<br>App can use Job or Direct update flow Pull Model. |
| OTA_MQTT_ONLY=<0,1> | No | Depends on Target Support | Include OTA support for update transfers using ONLY MQTT protocol.<br>App can use Job or Direct update flow Pull Model. |
| OTA_MQTT_SUPPORT=<0,1> | No | Depends on Target Support | Include OTA support for MQTT Broker transfers over HTTP. Implies HTTP library support<br>App can use Job or Direct update flow Pull Model. |
| OTA_BT_ONLY=<0,1> | No | Depends on Target Support | Include BT Support for transfers over ONLY BT/BLE/BLESS<br>Push Model. |
| OTA_BT_SUPPORT=<0,1> | No | Depends on Target Support | Include BT Support for transfers over BT/BLE/BLESS<br>Push Model. |
| OTA_BT_SECURE=<0,1> | No | Requires BT Support | Include BT TLS secure connection support |
| OTA_APP_VERSION_MAJOR=\<application_version\> | Yes | Error | Application Major version number  X.y.z|
| OTA_APP_VERSION_MINOR=\<application_version\> | Yes | Error | Application Minor version number  x.Y.z|
| OTA_APP_VERSION_BUILD=\<application_version\> | Yes | Error | Application Build version number x.y.Z |
| OTA_TEST_APP_VERSION_IN_TAR=\<0,1\> | No | 0 | Set to 1 to enable checking application version in TAR file in OTA library when updating using a TAR file. |
| OTA_HTTP_USER_AGENT_VALUE=\<string\> | No | "\"ota-http-client\"" | Passed to HTTP library as:<br> DEFINES += HTTP_USER_AGENT_VALUE=$(OTA_TEST_APP_VERSION_IN_TAR)<br>Used in HTTP connection. |
| OTA_HTTP_MAX_HDR_SIZE=\<size\> | No | 2048 | Passed to HTTP library as:<br>DEFINES += HTTP_MAX_RESPONSE_HEADERS_SIZE_BYTES=$(OTA_HTTP_MAX_HDR_SIZE) |
| OTA_HTTP_USE_CUSTOM_CONFIG=\<0,1\> | No | 0 | Passed to HTTP library as:<br>   ifneq ($(OTA_HTTP_USE_CUSTOM_CONFIG),0)<br>DEFINES += HTTP_DO_NOT_USE_CUSTOM_CONFIG<br>endif |
| OTA_MQTT_COMPANY_TOPIC_PREPEND=\<string\> | No | "\"OTAUpdate\"" | Configurable part of MQTT topic name. Must be enclosed in <slash-quote> to compile correctly. |
| OTA_HTTP_DEBUG=<0,1> | No | Depends on Target Support | Enable for debugging http / ssl. |
| OTA_MQTT_DEBUG=<0,1> | No | Depends on Target Support | Enable for debugging mqtt / ssl. |
| OTA_BT_DEBUG=<0,1> | No | Depends on Target Support | Enable for BT Module debugging. |
| OTA_BUILD_VERBOSE=<0,1> | No | 0 | Enable to output info about Defines. |
| OTA_BUILD_FLASH_VERBOSE=<0,1> | No | 0 | Enable to output info about Flash layout. |
| OTA_BUILD_COMPONENTS_VERBOSE=<0,1> | No | 0 | Enable to output info about COMPONENTS, DEFINES, CY_IGNORE. |
| OTA_BUILD_DEFINES_VERBOSE=<0,1> | No | 0 | Enable to output info about DEFINES. |
| OTA_BUILD_IGNORE_VERBOSE=<0,1> | No | 0 | Enable to output info about CY_IGNORE. |
| OTA_BUILD_POST_VERBOSE=<0,1> | No | 0 | Enable to output info about POSTBUILD values. |
| CY_SIGN_KEY_PATH=\<Image Signing Key Path\> | No | Test key 'cypress-test-ec-p256.pem' is used for signing. | MCUBootloader uses the default key for validating images.<br>Users can use this Makefile entry to use their own key for signing BOOT/UPGRADE images. |
| IMG_SIGNING_SCRIPT_TOOL_PATH=\<Image Signing Tool Path\> | No | For PSoC6 - Image Signing Tool provided by MCUBootloader.<br>For 20829 - cysecuretools 4.2 | Users can use this Makefile entry to use a tool of their choice for signing update images.<br>If this makefile entry is empty, OTA library uses the default Image signing tools depending on the Target device. |
| OTA_POLICY_FILE_PATH=\<Application's Policy File Path\> | No | Depends on Target Support | User needs to define this Makefile entry to provide the policy file path for 20829 devices which use cysecuretools for signing update images.<br>Refer to [OTA MCUBoot App Readme](./OTA_MCUBOOTAPP_README.md).<br>This is not required for PSoC6 non-secure devices.  |
| OTA_APP_POSTBUILD=\<Application's POSTBUILD commands\> | No | Post-build commands for generating Signed BOOT and UPGRADE images. | Users can use this Makefile entry to provide their own post-build commands.<br>If this makefile entry is empty, the OTA library uses the default POSTBUILD commands which create signed BOOT and UPGRADE images.|

<b>Notes:</b>
- ota-update repo provides makefiles/target_ota.mk and makefiles/mcuboot_flashmap.mk files for simplifying the OTA integration with the applications.
- Application Makefile needs to include target_ota.mk file from the ota-update repo for the Target dependent OTA and Flashmap Configurations.
- target_ota.mk internally includes mcuboot_flashmap.mk which provides all the flashmap configurations based on the Target.

    ```
    ifneq ($(MAKECMDGOALS),getlibs)
    ifneq ($(MAKECMDGOALS),get_app_info)
    ifneq ($(MAKECMDGOALS),printlibs)
        include ../mtb_shared/ota-update/release-v3.0.0/makefiles/target_ota.mk
    endif
    endif
    endif
    ```

- Application Version:
    - Used to verify that the new version is higher than the previous version in the Job Flow.
    - Used in messaging with MQTT Broker, and testing MQTT chunks.

- Transfer Protocol:
    - OTA Application Makefile must enable atleast one of the supported Protocols.

