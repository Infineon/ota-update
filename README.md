# Over-the-Air (OTA) Update Middleware Library - Secure firmware updates for Infineon connectivity devices over Wi-Fi, Ethernet, and Bluetooth&reg;

# Overview

The Over-the-Air (OTA) Update middleware library provides support for downloading and applying over-the-air updates to the application code running on an Infineon device. It lets connected products receive new firmware in the field over Wi-Fi, Ethernet, or Bluetooth&reg;, without a physical connection.

For **Wi-Fi** and **Ethernet**, the OTA library uses MQTT or HTTP with TLS to securely connect to an MQTT Broker or HTTP server and download the user application. For **Bluetooth&reg;**, the OTA library works with an intermediate peer application on a laptop or phone that pushes the image to the device. On devices that support both Wi-Fi and Bluetooth&reg;, either interface can be used.

Starting with v4.0, the OTA library is fully separated from the bootloader. It downloads the upgrade image and hands each chunk to application-implemented storage callbacks (`cy_ota_storage_interface_t`), so it can work with any bootloader. The companion `ota-bootloader-abstraction` library provides ready-made storage callback implementations for MCUboot-based images and for the built-in bootloader on the CYW955913SDCM2WLIPA platform. To use those implementations, register the callbacks during OTA agent initialization.

The library supports two update models:
- **Push model (Bluetooth&reg;):** on reset, the device advertises that it can accept an update, and a host (laptop or phone) connects and pushes the image.
- **Pull model (MQTT/HTTP over Wi-Fi or Ethernet):** the device pulls the image using either a **Direct flow** (download a known file) or a **Job flow** (download a JSON job document describing the update, then download the image).

On a virgin device, the main application (BOOT image) is flashed once; subsequent versions (updates) are delivered through OTA. On multi-image platforms such as PSOC&trade; 64 and PSOC&trade; Edge E84 (PSE84), the OTA solution supports TAR-format packages to update a single image or multiple images (for example, a main application and its TFM images, or a CM33 Secure image, a CM33 Non-Secure image, and a CM55 image).

The ModusToolbox&trade; OTA code examples import the `ota-update` library automatically.

For the complete API description, see the [OTA Update Reference Manual](https://infineon.github.io/ota-update/api_reference_manual/html/index.html).

# Features

- Secure download over Wi-Fi or Ethernet using MQTT or HTTP with TLS.
- Bluetooth&reg; push updates using an intermediate host/peer application.
- Job flow and Direct flow for pull-model (MQTT/HTTP) updates.
- Bootloader-agnostic design: upgrade-image handling is offloaded to application storage callbacks (`cy_ota_storage_interface_t`), with ready-made callbacks available in `ota-bootloader-abstraction`.
- OTA agent runs in a separate background thread and provides callbacks for connection stage, download progress, and error reporting.
- TAR-format packages for single-image or multi-image (for example, TFM) updates.
- Configurable timing and connection parameters through `cy_ota_config.h`.
- FreeRTOS and ThreadX support.

# When to Use

Use the OTA Update library when you need to update the firmware of a deployed Infineon connectivity device in the field, including:
- Remote firmware updates for PSOC&trade; 6, PSOC&trade; 64, CYW20829, CYW89829, XMC7200, CYW955913, and PSOC&trade; Edge E84 (PSE84) applications.
- Updates delivered over Wi-Fi or Ethernet from an MQTT Broker or HTTP server (pull model).
- Updates pushed over Bluetooth&reg; from a laptop or phone (push model).
- Single-image or multi-image (for example, main application plus TFM images) updates using TAR packages.

OTA runs on the CM4 CPU on multi-core PSOC&trade; 6 devices, the CM7 CPU on XMC7200 devices, the CM33 CPU (alongside the bootloader) on CYW20829, CYW89829, and CYW955913 devices, and either the CM33 Non-Secure CPU or the CM55 CPU on PSOC&trade; Edge E84 (PSE84) devices.


# Prerequisites

## Hardware Requirements

The OTA Update library supports the following Infineon device families and their connectivity capabilities:
- PSOC&trade; 6 MCU with AIROC&trade; CYW4343W, CYW43012, or CYW43439 Wi-Fi & Bluetooth&reg; combo connectivity
- PSOC&trade; 6 Bluetooth&reg; LE devices (for example, CY8CKIT-062-BLE, CY8CPROTO-063-BLE, CYBLE-416045-EVAL)
- PSOC&trade; 64 Secure Boot MCU
- AIROC&trade; CYW20829 and CYW89829 Bluetooth&reg; LE SoCs
- XMC7200 (with optional CYW43439 Wi-Fi connectivity)
- CYW955913 (with built-in bootloader)
- PSOC&trade; Edge E84 (PSE84)

For the specific kits validated with this release, see [RELEASE.md](./RELEASE.md).

**Note:** Before using the library, make sure the board is upgraded to KitProg3. The tool and instructions are available in the [Firmware Loader](https://github.com/Infineon/Firmware-loader) GitHub repository. Otherwise you may see an error such as "unable to find CMSIS-DAP device" or "KitProg firmware is out of date". This example uses the board's default configuration; see the kit user guide to ensure the board is configured correctly.

## Software Requirements

This version of the library was validated for compatibility with the following software and tools:

| Software and Tools                                        | Version |
| :---                                                      | :---    |
| ModusToolbox&trade; Software Environment                  | 3.9     |
| ModusToolbox&trade; Device Configurator                   | 5.90    |
| ModusToolbox&trade; Bluetooth Configurator                | 3.60    |
| GCC Compiler                                              | 14.2.1  |
| IAR Compiler                                              | 9.50.2  |
| Arm Compiler 6                                            | 6.22    |
| LLVM Compiler                                             | 22.1.0  |

Supported toolchains: GCC, IAR, and Arm C. Supported operating systems: FreeRTOS and ThreadX. For the authoritative, up-to-date version list, see [RELEASE.md](./RELEASE.md).

The supplied Publisher and Subscriber Python scripts require Python (tested with Python 3.8.1) and the `paho-mqtt` module version 1.6.1:
```
pip install paho-mqtt==1.6.1
```
**Note:** The Publisher and Subscriber scripts are compatible only with paho-mqtt version 1.6.1.

A terminal emulator (for example, Tera Term) is recommended for viewing log output.

## Read Documentation First

- [OTA Update Reference Manual](https://infineon.github.io/ota-update/api_reference_manual/html/index.html) - Complete OTA API reference
- [OTA Makefile settings for a user application](./OTA_MAKEFILE_INFO_README.md) - Makefile variables and build DEFINES
- [OTA Bootloader Abstraction Reference Manual](https://infineon.github.io/ota-bootloader-abstraction/api_reference_manual/html/index.html) - Ready-made storage callbacks and bootloader support
- [ModusToolbox&trade; Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/modustoolbox)

## Choose Starting Template

Start from a ModusToolbox&trade; OTA code example (recommended), which imports the `ota-update` library and its dependencies automatically. Alternatively, start from an Empty application and add the required libraries as described below.

## Add Required Libraries

Add the required middleware using the ModusToolbox&trade; **Library Manager** (available in the ModusToolbox&trade; for VS Code extension and the Eclipse IDE), or by creating the corresponding `.mtb` files in the application's `deps` folder and running `make getlibs`.

Add the OTA library itself by creating `ota-update.mtb`:
```
https://github.com/Infineon/ota-update#latest-v4.X#$$ASSET_REPO$$/ota-update/latest-v4.X
```

Depending on your bootloader, transport, and platform, add the following libraries:

- **Bootloader (MCUboot-based platforms: PSOC&trade; 6, CYW20829, CYW89829, XMC7200, PSE84)** - `ota-bootloader-abstraction.mtb`:
  ```
  https://github.com/Infineon/ota-bootloader-abstraction#latest-v1.X#$$ASSET_REPO$$/ota-bootloader-abstraction/latest-v1.X
  ```
- **Wi-Fi transport (PSOC&trade; 6)** - `wifi-core-freertos-lwip-mbedtls.mtb`:
  ```
  https://github.com/Infineon/wifi-core-freertos-lwip-mbedtls#latest-v4.X#$$ASSET_REPO$$/wifi-core-freertos-lwip-mbedtls/latest-v4.X
  ```
  **Note:** For the PSOC&trade; 64 platform, the ota-update library is currently only supported with `wifi-core-freertos-lwip-mbedtls/latest-v1.X`. For the PSOC&trade; 6 and XMC7200 platforms, it is currently only supported with `wifi-core-freertos-lwip-mbedtls/latest-v2.X`.
- **Wi-Fi transport (CYW955913EVK-01)** - `wifi-core-threadx-cat5.mtb`:
  ```
  https://github.com/Infineon/wifi-core-threadx-cat5#latest-v1.X#$$ASSET_REPO$$/wifi-core-threadx-cat5/latest-v1.X
  ```
- **Ethernet transport (XMC7200)** - `ethernet-core-freertos-lwip-mbedtls.mtb` and `ethernet-phy-driver.mtb`:
  ```
  https://github.com/Infineon/ethernet-core-freertos-lwip-mbedtls#latest-v2.X#$$ASSET_REPO$$/ethernet-core-freertos-lwip-mbedtls/latest-v2.X
  ```
  ```
  https://github.com/Infineon/ethernet-phy-driver#latest-v1.X#$$ASSET_REPO$$/ethernet-phy-driver/latest-v1.X
  ```
  **Note:** Applications using `ethernet-core-freertos-lwip-mbedtls v1.X` for OTA updates do not need to pull `ethernet-phy-driver`.
- **Bluetooth&reg; transport (PSOC&trade; 6, CYW20829, CYW89829, PSE84)** - `btstack-integration.mtb`:
  ```
  https://github.com/Infineon/btstack-integration#latest-v7.X#$$ASSET_REPO$$/btstack-integration/latest-v7.X
  ```
  **Note:** For Bluetooth&reg;-based OTA on CYW955913EVK-01, the required Bluetooth&reg; libraries are available in ROM and do not need to be pulled explicitly.
- **MQTT transport** - `mqtt.mtb`:
  ```
  https://github.com/Infineon/mqtt#latest-v4.X#$$ASSET_REPO$$/mqtt/latest-v4.X
  ```
- **HTTP transport** - `http-client.mtb`:
  ```
  https://github.com/Infineon/http-client#latest-v1.X#$$ASSET_REPO$$/http-client/latest-v1.X
  ```

After adding the `.mtb` files, run:
```
make getlibs
```

## Configure Makefile

Create the OTA application Makefile as described in [OTA Makefile settings for a user application](./OTA_MAKEFILE_INFO_README.md). At minimum, enable OTA and select the transports and application version:
```
OTA_SUPPORT=1
OTA_HTTP_SUPPORT=1
OTA_MQTT_SUPPORT=1
OTA_BT_SUPPORT=1
APP_VERSION_MAJOR=1
APP_VERSION_MINOR=0
APP_VERSION_BUILD=0
```
For MCUboot support, update the Makefile as described in the `ota-bootloader-abstraction` [MCUBootloader Support Makefile Readme](https://github.com/Infineon/ota-bootloader-abstraction/blob/master/source/COMPONENT_MCUBOOT/MCUBOOT_OTA_MAKEFILE_INFO_README.md). For the built-in bootloader on CYW955913EVK-01, see the [CYW955913SDCM2WLIPA Bootloader Support Makefile Readme](https://github.com/Infineon/ota-bootloader-abstraction/blob/master/source/COMPONENT_CYW955913SDCM2WLIPA/CYW955913SDCM2WLIPA_OTA_MAKEFILE_INFO_README.md).

To enable debug logging, add `ENABLE_OTA_LOGS` to the Makefile DEFINES and call `cy_log_init()` (provided by the `cy-log` module of the `connectivity-utilities` library):
```
DEFINES+=ENABLE_OTA_LOGS
```
See the [Connectivity Utilities Reference Manual](https://infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html) for `cy-log` details.

## Add Headers

Include the OTA API header in your application:
```c
#include "cy_ota_api.h"
```
Copy the configuration file `configs/cy_ota_config.h` from the library to your application's top-level directory and adjust it as needed:
```
cp <ota-update-library>/configs/cy_ota_config.h <application dir>
```
For more details on the storage callback APIs, see `include/cy_ota_api.h` and the [OTA Update Reference Manual](https://infineon.github.io/ota-update/api_reference_manual/html/index.html).

# Quick Start

This quick start outlines the steps to add OTA to an application. It assumes you have chosen and programmed a bootloader for your platform.

**Step 1:** Add the `ota-update` library and the libraries required for your bootloader, transport, and platform (see [Add Required Libraries](#add-required-libraries)), then run `make getlibs`.

**Step 2:** Create the OTA application Makefile (see [Configure Makefile](#configure-makefile)) and add the required bootloader support.

**Step 3:** Copy `configs/cy_ota_config.h` into your application directory and adjust the timing and connection parameters as needed.

**Step 4:** Implement the storage callbacks defined in `cy_ota_storage_interface_t` (or use the implementations from `ota-bootloader-abstraction`), then register them and start the OTA agent (see [Sample Code](#sample-code)).

**Expected Outcome:** The OTA agent runs in a background thread, contacts the server or Broker at the configured interval, downloads any available update, and (if `reset_after_complete` is set) resets the device so the bootloader can apply the update.

## Sample Code

The application registers a set of storage callbacks when starting the OTA agent. These callbacks handle the downloaded upgrade image based on the chosen bootloader:

```c
typedef struct cy_ota_storage_interface_s
{
    cy_ota_file_open           ota_file_open;             /**< Creates and open new receive file for the data chunks as they come in. */
    cy_ota_file_read           ota_file_read;             /**< Reads the flash data starting from the given offset.                   */
    cy_ota_file_write          ota_file_write;            /**< Write a data to the flash at the given offset.                         */
    cy_ota_file_close          ota_file_close;            /**< Close the underlying receive file in the specified OTA context.        */
    cy_ota_file_verify         ota_file_verify;           /**< Authenticate received OTA update file specified in the OTA context.    */
    cy_ota_file_set_pending    ota_file_set_boot_pending; /**< To Mark the image in the inactive slot as pending.                     */
    cy_ota_file_validate       ota_file_validate;         /**< The application has validated the new OTA image.                       */
    cy_ota_file_get_app_info   ota_file_get_app_info;     /**< Get Application info like Application version and Application ID.      */
} cy_ota_storage_interface_t;
```

Pass the MQTT Broker/HTTP server parameters, credentials, and the storage-callback interface (`cy_ota_storage_interface_t`) into `cy_ota_agent_start()`. The OTA agent then runs in its own background thread and, for Bluetooth&reg; operation, starts the Bluetooth&reg; module and begins advertising. The agent provides a callback mechanism that reports connection stages, download percentage, and errors; the application can override the default behavior for each step or stop the current download. For the full API and callback signatures, see `include/cy_ota_api.h` and the [OTA Update Reference Manual](https://infineon.github.io/ota-update/api_reference_manual/html/index.html).

## Expected Outcome

When the OTA agent is running and an update is available, you should observe:

Visual/Hardware Behavior:
- The device connects to the configured MQTT Broker/HTTP server (or advertises over Bluetooth&reg;), downloads the upgrade image, and stores it in the upgrade slot using the registered storage callbacks.
- If `reset_after_complete` is set, the device resets after the download completes, and the bootloader boots the new application on the next reset.

Terminal Output (with `ENABLE_OTA_LOGS` defined):
```
Starting OTA agent...
Connecting to server...
OTA download progress: 10%
OTA download progress: 50%
OTA download progress: 100%
OTA download complete. Resetting device...
```

Performance Metrics:
- Download time depends on the transport, image size, and network conditions. For HTTP updates, Smart HTTP response buffering (v4.5.1 and later) reduces the overall upgrade time; for large MQTT updates, set `CY_OTA_MAX_PACKETS` (v4.5.2 and later) appropriately.

# Troubleshooting

- **Missed MQTT update (no Job document):** If you are not using a Job document with MQTT and the device is not subscribed to the topic when the Publisher sends the update, it will miss the update. Use a Job document, which contains the location of the OTA image.
- **Unreliable network:** Ensure a reliable network connection before starting an OTA update; poor connectivity can cause lost packets or a lost connection and a failed update.
- **Low battery:** Ensure the device is adequately powered before starting an update; a low battery can cause OTA to fail.
- **Arm compiler link error on CYW20829/Bluetooth&reg; platforms:** Ensure `MBEDTLS_ENTROPY_HARDWARE_ALT` is not defined in `mbedtls_user_config.h`. When it is defined, you may encounter an unresolved-symbol error during the link phase with the Arm compiler.
- **CYW20829B0 / CYW89829B0 silicon:** The default BSP (3.X) targets the B1 silicon. For B0 silicon, use BSP Assistant to configure MPN `CYW20829B0LKML` (CYW920829M2EVK-02) or `CYW89829B01MKSBG` (CYW989820M2EVB-01), run `make getlibs`, replace `fw-cyw20829b1`/`fw-cyw89829b1` with `fw-cyw20829b0`/`fw-cyw89829b0` in `libs/mtb.mk`, then clean-build and program.
- **Windows 10 Python setup:** Install the pip requirements into the ModusToolbox&trade; Python: `<ModusToolbox>/tools_3.*/python/python -m pip install -r <ota-update-library>scripts/requirements.txt`.
- **No log output:** The library disables all debug logs by default. Add `DEFINES+=ENABLE_OTA_LOGS` to the Makefile and call `cy_log_init()`.
- **KitProg3:** If programming fails with "unable to find CMSIS-DAP device", upgrade the board to KitProg3 using the [Firmware Loader](https://github.com/Infineon/Firmware-loader).

# Related Code Examples

- [ModusToolbox&trade; OTA code examples](https://github.com/Infineon?q=mtb-example-ota&type=all) - Infineon `mtb-example-ota-*` reference applications for MQTT, HTTP, and Bluetooth&reg; OTA updates

# Configuration Reference

OTA behavior is configured through the application Makefile and the `cy_ota_config.h` file:
- **Makefile DEFINES** - Enable OTA and select transports and options: `OTA_SUPPORT`, `OTA_HTTP_SUPPORT`, `OTA_MQTT_SUPPORT`, `OTA_BT_SUPPORT`, `APP_VERSION_MAJOR`/`APP_VERSION_MINOR`/`APP_VERSION_BUILD`, and debug switches. For the complete list, see [OTA Makefile settings for a user application](./OTA_MAKEFILE_INFO_README.md).
- **`cy_ota_config.h`** - Adjust timing values (how often to check for updates), buffer sizes, and MQTT Broker/HTTP server connection parameters (for example, `CY_OTA_MAX_PACKETS`). Copy this file from `configs/cy_ota_config.h` into your application and edit as needed. For the parameter descriptions, see the [OTA Update Reference Manual](https://infineon.github.io/ota-update/api_reference_manual/html/index.html).

## Update Flows

For pull-model (MQTT/HTTP) updates, choose one of two flows:

- **Direct flow:** The device downloads a known file from the server/Broker. Over HTTP, the device issues HTTP `GET` requests for sequential ranges of data. Over MQTT, the device publishes a "Request Download" message with a unique topic name, and the Publisher splits the image into chunks and publishes them on that topic. Select Direct flow by setting the `use_get_job_flow` field in `cy_ota_network_params_t` to `CY_OTA_DIRECT_FLOW` when calling `cy_ota_agent_start()`.
- **Job flow:** The device first downloads a JSON job document (default name `ota_update.json`) describing where the update is, its version, and other details. It then determines whether the update is appropriate (for example, a newer version for the same board) and downloads the image from the specified server/Broker. Over HTTP, the device uses an HTTP `GET` request to obtain the Job document; over MQTT, it subscribes to a known topic and publishes a request, and the Publisher responds with the Job document on the unique topic.

## Job Document

The Job document is a JSON-formatted file that contains fields used to communicate between the Publisher script and the OTA agent running on the device:

```
Manufacturer:   Manufacturer Name.
ManufacturerId: Manufacturer Identifier
Product:        Product Name.
SerialNumber:   The Devices unique Serial Number
Version:        Application version Major.Minor.Build (ex: "12.15.0")
Board:          Name of board used for the product. (ex: "CY8CPROTO_062_4343W")
Connection:     The type of Connection to access the OTA Image. ("MQTT", "HTTP", "HTTPS")
Port:           Port number for accessing the OTA Image. (ex: MQTT:1883, HTTP:80)
```

Additional fields for the HTTP transport type:

```
Server:         Server URL (ex: "http://ota_images.my_server.com")
File:           Name of the OTA Image file (ex: "<product>_<board>_v5.6.0.bin")
Offset:         Offset in bytes for the start of data to transfer.
Size:           The size of the chunk of data to send, in bytes.
```

Example Job document for an OTA update image available on an HTTP server:

```
{
  "Manufacturer":"Express Widgits Corporation",
  "ManufacturerId":"EWCO",
  "Product":"Easy Widgit",
  "SerialNumber":"ABC213450001",
  "Version":"1.76.0",
  "Board":"CY8CPROTO_062_4343W",
  "Connection":"HTTP",
  "Server":"<server_URL>",
  "Port":"80",
  "File":"/<OTA_image_file_name>"
}
```

Additional fields for the MQTT transport type:

```
Broker:          MQTT Broker to access the OTA update image (for example: mqtt.my_broker.com)
UniqueTopicName: The "replace" value is just a placeholder. It is replaced with the Unique Topic Name for the OTA data transfer.
```

Example Job document for an OTA update image available on an MQTT Broker:

```
{
  "Manufacturer":"Express Widgits Corporation",
  "ManufacturerId":"EWCO",
  "Product":"Easy Widgit",
  "SerialNumber":"ABC213450001",
  "Version":"1.2.28",
  "Board":"CY8CPROTO_062_4343W",
  "Connection":"MQTT",
  "Broker":"mqtt.widgitcorp.com",
  "Port":"1883",
  "UniqueTopicName":"replace"
}
```

## MQTT Publisher Script

The *ota-update/WiFi_Ethernet/scripts/publisher.py* script contains several configurable values. Ensure that the parameters in the script match the application values. In addition, check the example application you are using for any special needs.

The Publisher and Subscriber scripts need the 'paho-mqtt' Python module, version 1.6.1.

```
pip install paho-mqtt==1.6.1
```
**NOTE**:The Publisher and Subscriber scripts are only compatible with paho-mqtt version 1.6.1.

Using the publisher script to test MQTT updates:

```
cd mtb_shared/ota-update/scripts/WiFi_Ethernet
python publisher.py [tls] [-l] [-f <filepath>] [-b <broker>] [-k <kit>] [-c <company_topic>]
```

Usage:

- `tls` - Add this argument to run the Publisher with TLS (secure sockets). You must also supply the appropriate certificates and keys.

- `-l` - Output more logging information to debug connection issues.

- `-f <filepath>` -Use this argument to designate the name of the downloaded file.

- `-b <broker>` - Use this argument to override the default broker.  Note that you will need to add the certificates for TLS usage. The files must reside in the same directory as the *subscriber.py* script.

  - `amazon`

    - Change the `AMAZON_BROKER_ADDRESS` endpoint in the *publisher.py* script.

         ```
         AMAZON_BROKER_ADDRESS = "<endpoint>.iot.us-west-1.amazonaws.com"
         ```

    - Add the following certificate files:

      ```
      ca_certs = "amazon_ca.crt"
      certfile = "amazon_client.crt"
      keyfile  = "amazon_private_key.pem"
      ```

  - `eclipse` ( broker: `mqtt.eclipseprojects.io`)

    - Add the following certificate files:

      ```
      ca_certs = "eclipse_ca.crt"
      certfile = "eclipse_client.crt"
      keyfile  = "eclipse_client.pem"
      ```
  - `mosquitto` (broker: `test.mosquitto.org`)

    - Add the following certificate files:

      ```
      ca_certs = "mosquitto.org.crt"
      certfile = "mosquitto_client.crt"
      keyfile  = "mosquitto_client.key"
      ```

- `-k <kit>` - Use this argument to override the default kit (CY8CPROTO_062_4343W).

  The kit name is used as part of the topic name; it must match the kit you are using, be sure to replace '-' (dash) with '_' (underscore).

- `-c <company_topic>` - Add this to the first part of the topic path names.

  Company topic default is "OTAUpdate".

  This must also be mirrored in the application for the topic name. This allows for multiple devices being tested to simultaneously connect to different instances of the  Publisher running on different systems so that they do not interfere with each other.

## MQTT Subscriber Script

The *subscriber.py* script is provided as a verification script that acts the same as a device. It can be used to verify that the Publisher is working as expected. Ensure that the `BROKER_ADDRESS` matches the Broker used in *publisher.py*.

Using the publisher script for verifying the Broker connection:

```
cd ota-update/scripts/WiFi_Ethernet
python subscriber.py [tls] [-l] [-b broker] [-k kit] [-f filepath] [-c <chunk_size>] [-e <company_topic>]
```

Usage:

- `tls` - Add this argument to run the Publisher with TLS (secure sockets). You must also supply appropriate certificates and keys.

- `-l` - Output more logging information to debug connection issues.

- `-b <broker>` - Use this argument to override the default Broker. Note that you will need to add certificates for TLS usage. The files need to reside in the same directory as *publisher.py*.

  - `amazon`

    - Change the `AMAZON_BROKER_ADDRESS` endpoint in the *subscriber.py* script.

      ```
      AMAZON_BROKER_ADDRESS = "<endpoint>.iot.us-west-1.amazonaws.com"
      ```

    - Add the following certificate files:

      ```
      ca_certs = "amazon_ca.crt"
      certfile = "amazon_client.crt"
      keyfile  = "amazon_private_key.pem"
      ```

  - `eclipse`  (Broker: `mqtt.eclipseprojects.io`)

    - Add the following certificate files:
      ```
      ca_certs = "eclipse_ca.crt"
      certfile = "eclipse_client.crt"
      keyfile  = "eclipse_client.pem"
      ```

  - `mosquitto`  (Broker: `test.mosquitto.org`)

    - Add the following certificate files:

      ```
      ca_certs = "mosquitto.org.crt"
      certfile = "mosquitto_client.crt"
      keyfile  = "mosquitto_client.key"
      ```

- `-k kit` - Use this argument to override the default kit (CY8CPROTO_062_4343W).

  The kit name is used as part of the topic name; it must match the kit you are using, be sure to replace '-' (dash) with '_' (underscore).

- `-f filepath` - Use this argument to designate the output file for download.

- `-c <chunk_size>` - The size in bytes of the chunk to request (used for testing).

- `-e <company_topic>` - Add this to the first part of the topic path names.

  Company topic default is "OTAUpdate".

  This must also be mirrored in the application for the topic name. This allows for multiple devices being tested to simultaneously connect to different instances of the Publisher running on different systems so that they do not interfere with each other.

## Bootloader Support

Starting with v4.0, the `ota-update` library is fully separated from MCUboot: it can work with any bootloader as long as the required storage callbacks are implemented and registered with the OTA agent.

**MCUboot support** - MCUboot is a secure bootloader for 32-bit microcontrollers; build it outside the OTA application and program it to the device before flashing the OTA application. It is not updated for the life of the device. You can implement your own storage callbacks or use the `ota-bootloader-abstraction` library, which provides:
- Template flashmaps for PSOC&trade; 6, CYW20829, CYW89829, XMC7200, and PSE84 platforms.
- Template linker files for the GCC_ARM, ARM, and IAR toolchains.
- Storage operation callback APIs to handle MCUboot-based upgrade images.
- Prebuild and postbuild scripts for generating and signing MCUboot-based BOOT and UPGRADE images.

See the `ota-bootloader-abstraction` [README](https://github.com/Infineon/ota-bootloader-abstraction/blob/master/README.md) for enabling MCUboot support.

**CYW955913EVK-01 built-in bootloader** - The CYW955913EVK-01 has a bootloader programmed in ROM. You can implement your own storage callbacks or use the storage callback APIs from `ota-bootloader-abstraction`. See the `ota-bootloader-abstraction` [README](https://github.com/Infineon/ota-bootloader-abstraction/blob/master/README.md) for enabling built-in bootloader support.

**Other bootloaders** - Implement the storage API callbacks defined in `cy_ota_storage_interface_t` (see `include/cy_ota_api.h`) and register them with the OTA agent. The agent downloads the upgrade image and invokes the registered callbacks to handle it. Sample scripts for hosting the UPGRADE image are available in the library `scripts/` folder. Once bootloader support is added, choose the Push or Pull model to deliver the image to the device.

## Bluetooth&reg; Push Updates

For Bluetooth&reg; (push model), the host downloads the upgrade image and uses a peer application to push it to the advertising device:

1. The host (laptop, phone, etc.) downloads the upgrade OTA image from the server over LAN/WLAN/Bluetooth&reg;.
2. On reset, the Bluetooth&reg; LE device running the OTA application advertises that it can handle OTA updates.
3. Use the peer app in the library `scripts/Bluetooth/` folder, or download it from the [OTA Peer Application page](https://github.com/Infineon/btsdk-peer-apps-ota) (Windows/iOS/Android):
   ```
   ./WsOtaUpgrade.exe /file <OTA App Upgrade.bin>
   ```
4. Select the appropriate Bluetooth&reg; LE OTA-supported device from the scanned list and start the transfer.
5. The host pushes the update to the device sequentially; the device stores the received chunks in its upgrade slot using the registered storage callbacks.
6. After the transfer completes, the bootloader boots the upgrade image on the next reboot.

# Release Notes and Changelog

- [RELEASE.md](./RELEASE.md) - Detailed release notes and changelog for all versions
- [version.xml](./version.xml) - Current library version

For device-support details and additional design guidance, see [How to Design with PSOC&trade; 6 MCU - KBA223067](https://community.infineon.com/t5/Knowledge-Base-Articles/How-to-Design-with-PSoC-6-MCU-KBA223067/ta-p/248857) in the Infineon community.

# License

This library is provided under Infineon (Cypress) software license terms. It depends on the `http-client`, `mqtt`, and `ota-bootloader-abstraction` libraries (pulled by default from the application's `deps` folder), and, depending on the selected transport and platform, on the `wifi-core-freertos-lwip-mbedtls`, `wifi-core-threadx-cat5`, `ethernet-core-freertos-lwip-mbedtls`, `ethernet-phy-driver`, or `btstack-integration` libraries. Each of these dependencies is governed by its own license terms. You are responsible for reviewing and accepting all applicable third-party license terms.

- [LICENSE](./LICENSE.txt)

# Copyright

(c) 2026, Infineon Technologies AG, or an affiliate of Infineon
Technologies AG.  All rights reserved.
