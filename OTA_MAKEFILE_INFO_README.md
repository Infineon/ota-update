# OTA Makefile Settings for a User Application

## 1. Introduction

This guide shows Makefile variables and build DEFINES used when building an application with ota-update library support.


## 2. Standard Application Makefile Defines


| Makefile Variable | Example | Description |
| ----------------- | ------- | ----------- |
| TARGET?=\<target_name\> | TARGET?=CY8CPROTO-062-4343W | Target Kit for build |
| CORE?=\<core_type\> | CORE?=CM4 | Core CPU for Application<br>(CM33, CM4, CM7) |
| APPNAME?=\<application_name\> | APPNAME=ota-test | Name of Application |
| TOOLCHAIN?=\<toolchain\> | TOOLCHAIN?=GCC_ARM | GCC_ARM (default)<br>ARM<br> IAR |
| CONFIG?=\<build_level\> | CONFIG?=Debug | Build level<br>- Debug<br>- Release |

## 3. OTA Specific Makefile Defines

| Makefile addition | Required | Default if not defined | Description |
| ----------------- | -------- | ---------------------- | ----------- |
| OTA_SUPPORT=<0,1> | Yes | Error | Must be set to 1 to add OTA defines, sources, and libraries. |
| OTA_HTTP_SUPPORT=<0,1> | No | 0 | Include OTA support for update transfers using HTTP directly to a server.<br>App can use Job or Direct update flow Pull Model. |
| OTA_MQTT_SUPPORT=<0,1> | No | 0 | Include OTA support for MQTT Broker transfers over HTTP. Implies HTTP library support<br>App can use Job or Direct update flow Pull Model. |
| OTA_BT_SUPPORT=<0,1> | No | 0 | Include BT Support for transfers over BT/BLE/BLESS<br>Push Model. |
| APP_VERSION_MAJOR=\<application_version\> | Yes | Error | Application Major version number  X.y.z|
| APP_VERSION_MINOR=\<application_version\> | Yes | Error | Application Minor version number  x.Y.z|
| APP_VERSION_BUILD=\<application_version\> | Yes | Error | Application Build version number x.y.Z |
| CY_TEST_APP_VERSION_IN_TAR=\<0,1\> | No | 0 | Set to 1 to enable checking application version in TAR file in OTA library when updating using a TAR file. |
| OTA_HTTP_USER_AGENT_VALUE=\<string\> | No | "\"ota-http-client\"" | Passed to HTTP library as:<br> DEFINES += HTTP_USER_AGENT_VALUE=$(CY_TEST_APP_VERSION_IN_TAR)<br>Used in HTTP connection. |
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
| CY_OTA_APP_HEAP_SIZE=\<Heap size required for the Application\> | No | 256KB | This Makefile entry is only specific to XMC7200 platform on IAR Toolchain. <br>Users can use this makefile entry to define Application Heap size as required. <br>If this makefile entry is empty, the OTA library uses the default Heap Size value(256KB).|
| APP_\<ID\>_NAME=\<application_name\> | Yes | Error | Specify each Application's Name in case of **Multi-App Projects**. Currently it is only supported on PSE84 platform. |

<b>Notes:</b>
- ota-update library provides makefiles/ota_update.mk file for simplifying the OTA integration with the applications.
- Application Makefile needs to include ota_update.mk file from the ota-update library for default configurations.

    ```
    ifneq ($(MAKECMDGOALS),getlibs)
    ifneq ($(MAKECMDGOALS),get_app_info)
    ifneq ($(MAKECMDGOALS),printlibs)
        include ../mtb_shared/ota-update/<version>/makefiles/ota_update.mk
    endif
    endif
    endif
    ```

- Application Version:
    - Used to verify that the new version is higher than the previous version in the Job Flow.
    - Used in messaging with MQTT Broker, and testing MQTT chunks.

- Transfer Protocol:
    - OTA Application Makefile must enable atleast one of the supported Protocols.
