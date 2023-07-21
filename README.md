
# Over-the-Air (OTA) Update Middleware Library

The OTA library provides support for Over-The-Air update of the application code running on an Infineon device.
Supported devices:
- PSoC™ 6 MCU with
  - AIROC™ CYW4343W
  - CYW43012 Wi-Fi & Bluetooth® combo chip
- CYW920829M2EVK-02 evaluation board.
- CY8CKIT-062-BLE
- CY8CPROTO-063-BLE
- CYBLE-416045-EVAL

For devices with both Wi-Fi and Bluetooth® Interfaces, the device can use Wi-Fi or Bluetooth® Interfaces.

For <b>WiFi</b>, the OTA library utilizes MQTT or HTTP and TLS to securely connect to an MQTT Broker/HTTP server to download and update the user application.

For <b>Bluetooth®</b>, the OTA library works with an intermediate peer application on a laptop (or phone) to push the image to the device.

## Library Versions
ota-update v3.0.0 works with ModusToolbox 3.0.

ota-update v2.0.0 works with ModusToolbox 3.0 and is <b>not</b> backwards compatible with ModusToolbox 2.4.

ota-update v1.0.0 works with ModusToolbox 2.4 and replaces anycloud-ota (all versions).

The anycloud-ota vx.x.x library will be depricated soon.

## 1. OTA Update with MCUBootloader Application

The OTA library works in concert with MCUBootApp to provide a no-fail solution to updating device software in the field.

MCUBootApp is a secure bootloader application for 32-bit microcontrollers and users should build it outside of the user OTA application. It is programmed separately to the device before flashing the user OTA application and is not updated for the life of the device.

User OTA application will include the ota-update library, which will set the appropriate flags so that MCUBootApp knows when to perform an update of your application. The OTA library will download a new (updated) application, store it in flash, and set flags so that on the next reset, MCUBootApp will see there is an update available.

<b>The basic device boot sequence is as follows:</b>

1. ROM boot will start MCUBootApp
2. If no update is downloaded, MCUBootApp starts the current application.
3. MCUBootApp determines if there is an update in the Secondary Slot
4. If there is an update available:<br>
   a. MCUBootApp verifies the update.<br>
   b. MCUBootApp SWAPs (or OVERWRITEs) the current with the update applications.<br>
   c. MCUBootApp starts the new (updated) application.<br>
   d. The updated application must call cy_ota_storage_validated() to validate the update.<br>
   e. For SWAP, if the new application does not call cy_ota_storage_validated(), MCUBootApp will REVERT on the next reset.<br>

<b>NOTE:</b> On secure MCUs such as PSoC™ 64, MCUBootApp is flashed as part of the provisioning step performed by the provisioning tools, and does not need to be built separately. For more information see [MCUBootApp Information](./OTA_MCUBOOTAPP_README.md#5-cysecuretools-for-psoc-64-and-cyw20829-devices).<br><br>

<b>1.1 Flash layout requirements</b><br>

MCUBootApp and the OTA enabled application must use the same flash layout.

The flash is separated into sections also called slots. The current application occupies the Primary (boot) Slot. The update application (new version) will be downloaded into the Secondary (upgrade) Slot.

Besides primary and secondary slots, MCUBootApp has a code section, a section for a "Swap Buffer" and a section for "Status Info".

| Section         | Size                 | Description                |
|-----------------| ---------------------|----------------------------|
| MCUBootApp      | 0x00018000           | MCUBootApp code            |
| Primary Slot    | Varies by flash size | User Application (current) |
| Secondary Slot  | Same size as Primary | Update Application (new)   |
| Swap Buffer     | Varies by flash size | Used by MCUBootApp for SWAP operation |
| Status          | Varies by size of Slots | Used by MCUBootApp for update status |

For more information, please see [OTA Flash Layout Information](./OTA_FLASH_LAYOUT_README.md).<br><br>

<b>1.2 Target and Flashmap Use</b><br>

These flashmaps are defaults for the supported targets as of v3.0.0 release.

| Target | Internal<br>Flash size | Flashmaps |
|-------------------|---------------------|----------|
| CY8CKIT-062S2-43012<br>CY8CKIT-064B0S2-4343W<br>CY8CPROTO-062-4343W<br>CY8CEVAL-062S2-LAI-4373M2<br>CY8CEVAL-062S2-MUR-43439M2| 2M | Default - psoc62_2m_ext_swap_single.json<br>psoc62_2m_ext_overwrite_single.json<br>psoc62_2m_int_overwrite_single.json<br>psoc62_2m_int_swap_single.json|
| CY8CPROTO-062-4343W | 2M | Default - psoc62_2m_int_swap_single.json |
| CY8CKIT-062-BLE | 1M | Default - psoc62_1m_cm0_int_swap_single.json |
| CY8CPROTO-063-BLE<br>CYBLE-416045-EVAL | 1M | Default - psoc63_1m_cm0_int_swap_single.json |
| CY8CPROTO-062S3-4343W  | 512K | Default - psoc62_512k_xip_swap_single.json<br> psoc62_512k_ext_overwrite_single.json<br>psoc62_512k_ext_swap_single.json |
| CYW920829M2EVK-02 | 0K | Default - cyw20829_xip_swap_single.json<br> cyw20829_xip_overwrite_single.json |

<br>
<br>

| 2M Internal Flash Maps | Memory Usage |
|--------------------------------|--------------|
| psoc62_2m_ext_overwrite_single.json   | Internal primary, external secondary  |
| psoc62_2m_ext_swap_single.json        | Internal primary, external secondary  |
| psoc62_2m_int_overwrite_single.json   | Internal only (primary and secondary) |
| psoc62_2m_int_swap_single.json        | Internal only (primary and secondary) |

| 1M Internal Flash Maps | Memory Usage |
|--------------------------------|--------------|
| psoc62_1m_cm0_int_swap_single.json    | Internal only (primary and secondary) |
| psoc63_1m_cm0_int_swap_single.json    | Internal only (primary and secondary) |


| 512K Internal Flash Maps | Memory Usage |
|--------------------------------|--------------|
| psoc62_512k_ext_overwrite_single.json | Internal primary, external secondary |
| psoc62_512k_ext_swap_single.json      | Internal primary, external secondary |
| psoc62_512k_xip_swap_single.json      | External only (primary and secondary) |


| 0K Internal Flash Maps | Memory Usage |
|--------------------------------|--------------|
| cyw20829_xip_overwrite_single.json       | External only (primary and secondary) |
| cyw20829_xip_swap_single.json            | External only (primary and secondary) |
| cyw20829_xip_swap_single_psvp.json       | External only (primary and secondary) |

<b>1.3 MCUBootApp Cloning and Building</b><br>

MCUBootApp is a standalone application. It is an open source software taken and customized in terms of Flash map and is built outside of ModusToolbox. MCUBootApp is programmed/flashed on the device one time, at manufacture (or for development).

- MCUBootApp runs on the CM0+ CPU and starts any OTA enabled application on CM4 core in case of multicore Psoc6 devices.
- In case of 20829 devices, MCUBootApp runs on CM33 CPU along with the OTA enabled applications.

MCUBoot itself is not OTA upgradable.

For cloning and building MCUBootApp refer to [MCUBootApp README](./OTA_MCUBOOTAPP_README.md).


## 2. Enabling OTA in an Application

OTA Enabled application is built in ModusToolbox, incorporating the OTA library and making initialization calls to start the OTA process.

- It runs on CM4 CPU in case of multicore Psoc6 devices
- In case of 20829 devices, it runs on CM33 CPU along with the MCUboot application.

On a virgin device, main application is programmed / flashed to the device first time. Subsequent versions of the application (so called updates) are downloaded and upgraded using OTA.

On some devices such as PSoC™ 64 kits, there will be multiple images such as main application and the TFM images that are OTA updatable. This OTA solution allows using TAR format package for updating a single, or multiple images.

The ModusToolbox OTA code examples import the ota-update library automatically.


## 3. Push Model Updates

<b>Bluetooth®</b> updates use a "Push" model. On reset / boot, the device advertises that it can handle an update, and a Host (laptop, phone, etc.) is used to connect and "Push" the update to the device.


## 4. Pull Model Updates

<b>WiFi</b> updates use a "Pull" model. There are two WiFi "Update Flows" that a user can designate. HTTP and MQTT transports can be used for this type of update.<br>

<b>4.1. Direct Flow</b><br>
With "Direct" flow approach, device downloads a known file from the server/broker.

a. HTTP: The device connects to the known HTTP Server and downloads the OTA update image using HTTP `GET` requests, asking for a range of data sequentially until all data is transferred.

b. MQTT: The device connects to the known MQTT Broker topic and publishes a "Request Download" message with a "Unique Topic Name" to the Publisher. The Publisher python application then splits the OTA update image into chunks and publishes them on the unique topic.<br>

<b>4.2. Job Flow</b><br>
With a "Job" flow approach, the device downloads a JSON formatted job document that provides information about where the update file is, its version, and other information concerning the update.

a. The device connects to the server/broker to obtain the Job document.
- HTTP: The device connects to the known HTTP server and uses an HTTP `GET` request to obtain the Job document.
- MQTT: The device subscribes to the known MQTT Broker topic and publishes a request for the Job document.

b. The device uses the Job document to determine if the OTA update image is an appropriate update.

c. The device connects to the data server/broker and downloads the OTA image.
- HTTP: The image is downloaded using HTTP `GET` requests, asking for a range of data sequentially until all data is transferred.
- MQTT: The device sends a "Request Download" message with the "Unique Topic Name" on the known topic. The puthon Publisher application then splits the OTA image into chunks and publishes them on the unique topic.


## 5. Quick start

- Create OTA Application Makefile by referring [OTA Application Makefile Readme](./OTA_MAKEFILE_INFO_README.md).

- Update given template OTA linker file(Or Create new one) and use it in OTA Application Makefile. Refer [OTA Linker file Readme](./OTA_MCUBOOT_LINKER_FILES_README.md) for details.

- The "cy_ota_config.h" file provides configuration options to adjust multiple timing values to customize how often to check for updates, and other parameters for the MQTT Broker/HTTP Server connection.
  Copy the configuration file "\<ota-update library\>configs/cy_ota_config.h" file to the directory where your application Makefile is, and adjust as needed.

- To add the required flash support at the application level itself, the OTA update library implements a callback mechanism and offloads flash operations to application.

- The application must implement these flash operation callbacks and must register callbacks during OTA agent start by giving a parameter of type 'cy_ota_agent_mem_interface_t' in 'cy_ota_agent_start()'.

    ```
    typedef struct cy_ota_agent_mem_interface
    {
        cy_ota_agent_mem_read read;                    /**< Reads a block of data from the specified memory location.                   */
        cy_ota_agent_mem_write write;                  /**< Write a block of data to the specified memory location at the given offset. */
        cy_ota_agent_mem_erase erase;                  /**< Erase a block of memory area.                                               */
        cy_ota_agent_mem_prog_size get_prog_size;      /**< Get programming block size.                                                 */
        cy_ota_agent_mem_erase_size get_erase_size;    /**< Get Erase block size.                                                       */
    } cy_ota_agent_mem_interface_t;
    ```

- For more details like Flash operation callback syntaxes, refer to "\<ota-update library\>include/cy_ota_api.h" .

- Parameters such as MQTT Broker/HTTP server and credentials along with memory operation callbacks are passed into `cy_ota_agent_start()`.

- The OTA Agent runs in a separate background thread, only connecting to MQTT Broker/HTTP server based on the timing configuration.
  For Bluetooth® operation, the OTA library starts the Bluetooth® module and starts advertising.

- The OTA Agent provides a callback mechanism to report stages of connect, download percentage, and errors. The application can override the default OTA Agent behavior for each step, or stop the current download during the callback.

Once the application starts the OTA agent, the OTA agent will contact the MQTT Broker/HTTP server at the defined intervals to check whether an update is available. If available, the update will be downloaded. If the `reset_after_complete` flag was set in the agent parameters, the OTA Agent will automatically reset the device. On the next system reset, MCUBootApp will perform the update.


## 6. Example Update Sequence Using BLE (PUSH Model)

- The Host(laptop, phone, etc) downloads the upgrade OTA Image from the server using LAN/WLAN/BT.

- BLE devices running OTA application on reset advertises that they can handle OTA updates.

- An intermediate application like WsOtaUpgrade.exe can be used for BT connection and update transfer.

    ```
    ./WsOtaUpgrade.exe /file ota-update.signed.bin
    ```

- From the scanned device list, Select the appropriate BLE OTA-supported device.

- Click on the start button to initiate the upgrade image transfer from host to device.

- Host application pushes the update to the device sequentially.

- BLE OTA device stores the received chunks of upgrade image into its secondary Slot.

- After transfer completion, On the next reboot MCUBootloader updates the image in a primary slot with the image in a secondary slot using OVERWRITE/SWAP option.

- Now, the updated application runs from the primary slot.


## 7. Example Job Flow Update Sequence Using HTTP

1. The device connects to the HTTP server and uses the HTTP `GET` request to obtain the Job document (for example: *ota_update.json*).

2. The device determines whether the OTA update image version is newer than the currently executing application, and the board name is the same, etc.

   - If the OTA update image is accessible through an MQTT Broker, the device creates and subscribes to a unique topic name and sends a "Request Download" message with the "Unique Topic Name" to the Publisher on the known topic `OTAUpdate/CY8CPROTO_062_4343W/notify_publisher`. The Publisher then splits the OTA update image into chunks and publishes them on the unique topic.

   - If the OTA update image is accessible on an HTTP Server, the device connects to the HTTP Server and downloads the OTA update image using an HTTP `GET` request, asking for a range of data sequentially until all data is transferred.

For Job document information, please see Job Document section below.


## 8. Example Job Flow Update Sequence Using MQTT

1. The Publisher Python script (*mtb_shared/ota-update/scripts/publisher.py*) subscribes to a known topic (for example: *OTAUpdate/CY8CPROTO_062_4343W/publish_notify*) on the specified MQTT Broker.

2. The device publishes the message "Update Availability" to the known topic. The message includes information about the device (manufacturer, product, serial number, application version, etc.), and a "Unique Topic Name" for the Publisher to send messages to the device.

3. The Publisher receives the "Update Availability" message and publishes the appropriate Job document on the "Unique Topic Name". The Publisher can make a decision if there is an appropriate update available for the device. The Publisher sends an "Update Available" (or "No Update Available") message, which repeats the device-specific information.

4. The device receives the Job document and connects to the Broker/server given in the Job document to obtain the OTA update image.

    - If the OTA update image is located on an MQTT Broker, the device connects and sends a "Request Update" message to the Publisher, which includes the device information again. The Publisher then splits the OTA update image into 4-kB chunks, adds a header to each chunk, and sends it to the device on the "Unique Topic Name".

    - If the OTA update image is located on an HTTP server, the device will connect to the HTTP server and download the OTA update image using an HTTP `GET` request, for a range of data sequentially until all data is transferred.

For Job document information, please see Job Document section below.


## 9. The Pull Model Job Document

The Job document is a JSON-formatted file that contains fields used to communicate between the Publisher script and the OTA Agent running on the device.

```
Manufacturer:   Manufacturer Name.
ManufacturerId: Manufacturer Identifier
Product:        Product Name.
SerialNumber:   The Devices unique Serial Number
Version:        Application version Major.Minor.Build (ex: "12.15.0")
Board:          Name of board used for the product. (ex: "CY8CPROTO_062_4343W")
Connection:     The type of Connection to access the OTA Image. ("MQTT", "HTTP", "HTTPS")
Port:           Port number for accessing the OTA Image. (ex: `MQTT:1883`, HTTP:80`)
```

<b>Additional fields for HTTP Transport Type:</b>

```
Server:         Server URL (ex: "http://ota_images.my_server.com")
File:           Name of the OTA Image file (ex: "<product>_<board>_v5.6.0.bin")
Offset:         Offset in bytes for the start of data to transfer.
Size:           The size of the chunk of data to send, in bytes.
```

The following is an example Job document for an OTA update image that is available on an HTTP server:

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

<b>Additional fields for MQTT Transport Type:</b>

```
Broker:          MQTT Broker to access the OTA update image (for example: `mqtt.my_broker.com`)
UniqueTopicName: The "replace" value is just a placeholder. It is replaced with the Unique Topic Name for the OTA data transfer.
```

The following is an example Job document for an OTA update image that is available on an MQTT Broker:

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

## 10. Setting Up the Supplied Publisher Python Script for MQTT Updates

The *mtb_shared/ota-update/scripts/publisher.py* script contains several configurable values. Ensure that the parameters in the script match the application values. In addition, check the example application you are using for any special needs.

The Publisher and Subscriber scripts require an additional Python module.

```
pip install paho-mqtt
```

Using the publisher script to test MQTT updates:

```
cd mtb_shared/ota-update/scripts
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

## 11. Using the Subscriber Python Script for testing MQTT Updates

The *subscriber.py* script is provided as a verification script that acts the same as a device. It can be used to verify that the Publisher is working as expected. Ensure that the `BROKER_ADDRESS` matches the Broker used in *publisher.py*.

Using the publisher script for verifying the Broker connection:

```
cd mtb_shared/ota-update/scripts
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


## 12. Note on Using Windows 10

When using ModusToolbox, you will need to install the pip requirements to Python in the ModusToolbox installation.

```
\<ModusToolbox\>/tools_3.0/python/python -m pip install -r \<ota-update-library\>source/port_support/mcuboot/imgtool/requirements.txt
```


## 13. Supported Toolchains

- GCC
- IAR
- ARM C

For the toolchain version information, please refer to [OTA Release.md](./RELEASE.md).

## 14. Supported OS

- FreeRTOS


## 15. Supported Kits

- [PSoC™ 6 Wi-Fi BT Prototyping Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062-4343w/) (CY8CPROTO-062-4343W)
- [PSoC™ 62S2 Wi-Fi BT Pioneer Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062s2-43012/) (CY8CKIT-062S2-43012)
- [PSoC™ 62S3 Wi-Fi BT Prototyping Kit ](https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062s3-4343w/)(CY8CPROTO-062S3-4343W)
- [PSoC™ 64 Secure Boot Wi-Fi BT Pioneer Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-064b0s2-4343w/) (CY8CKIT-064B0S2-4343W)
- [CY8CEVAL-062S2 Evaluation Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ceval-062s2/)(CY8CEVAL-062S2-LAI-4373M2 and CY8CEVAL-062S2-MUR-43439M2)
- [PSoC™ 6-BLE Pioneer Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062-ble/) (CY8CKIT-062-BLE)
- [PSoC™ 6 BLE Prototyping Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-063-ble/) (CY8CPROTO-063-BLE)
- [EZ-BLE Arduino Evaluation Board](https://www.infineon.com/cms/en/product/evaluation-boards/cyble-416045-eval/) (CYBLE-416045-EVAL)
- [AIROC™ CYW20829 Bluetooth® LE SoC](https://www.infineon.com/cms/en/product/promopages/airoc20829/) (CYW920829M2EVK-02)


## 16. Hardware Setup

This example uses the board's default configuration. See the kit user guide to ensure the board is configured correctly.

**Note:** Before using this code example, make sure that the board is upgraded to KitProg3. The tool and instructions are available in the [Firmware Loader](https://github.com/Infineon/Firmware-loader) GitHub repository. If you do not upgrade, you will see an error like "unable to find CMSIS-DAP device" or "KitProg firmware is out of date".


## 17. Software Setup

1. Install a terminal emulator if you don't have one. Instructions in this document use [Tera Term](https://ttssh2.osdn.jp/index.html.en).

2. Python Interpreter. The supplied *publisher.py* script is tested with [Python 3.8.1](https://www.python.org/downloads/release/python-381/).


## 18. Enabling Debug Output

The 'ota-update' library disables all debug log messages by default. Do the following to enable log messages:

1. Add the `ENABLE_OTA_LOGS` macro to the *DEFINES* in the code example's Makefile. The Makefile entry should look like as follows:
    ```
    DEFINES+=ENABLE_OTA_LOGS
    ```

2. Call the `cy_log_init()` function provided by the *cy-log* module. cy-log is part of the *connectivity-utilities* library. See [connectivity-utilities library API documentation](https://infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html) for cy-log details.

## 19. Prepare for Building Your OTA Application

Copy [OTA's cy_ota_config.h](./configs/cy_ota_config.h) to your application's top-level directory, and adjust for your application needs.

   ```
   cp \<ota-update-library\>/configs/cy_ota_config.h <application dir>
   ```

Also, a reminder to look at configuration files for other libraries.


## 20. Limitations

1. If not using a Job document with MQTT, and the device is not subscribed to the topic on the MQTT Broker when the Publisher sends the update, it will miss the update.

  As the workaround, use a Job document, which has information about where the OTA update image is located.

2. Internal and external flash is supported. You can use the internal flash only in certain combinations with platforms that have 2M of internal flash. The size of the application is greatly limited if using internal flash only.

3. Ensure that you have a reliable network connection before starting an OTA update. If your network connection is poor, OTA update may fail due to lost packets or a lost connection.

4. Ensure that you have a fully charged device before starting an OTA update. If your device's battery is low, OTA may fail.

5. When using the ARM compiler with the CYW20829 or BLE platforms, ensure that MBEDTLS_ENTROPY_HARDWARE_ALT is not defined in mbedtls_user_config.h. When MBEDTLS_ENTROPY_HARDWARE_ALT is defined it is possible to encounter an unresolved symbol error during the link phase when using the ARM compiler.


## 21. Additional Information

- [OTA Release version](./RELEASE.md)
- [OTA Flash Layout Information](./OTA_FLASH_LAYOUT_README.md)
- [OTA Makefile Information](./OTA_MAKEFILE_INFO_README.md)
- [OTA API reference guide](https://infineon.github.io/ota-update/api_reference_manual/html/index.html)
- ModusToolbox OTA Examples
    - [OTA MQTT Example](https://github.com/infineon/mtb-example-ota-mqtt)
    - [OTA HTTP Example](https://github.com/infineon/mtb-example-ota-https)
- [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/modustoolbox)
- [MCUBootApp Information](./OTA_MCUBOOTAPP_README.md)
- [MCUboot](https://github.com/JuulLabs-OSS/mcuboot/blob/cypress/docs/design.md) documentation

Infineon also provides a wealth of data at www.infineon.com to help you select the right device, and quickly and effectively integrate it into your design.

For PSoC™ 6 MCU devices, see [How to Design with PSoC 6 MCU - KBA223067](https://community.infineon.com/t5/Knowledge-Base-Articles/How-to-Design-with-PSoC-6-MCU-KBA223067/ta-p/248857) in the Infineon community.


## Additional Information

- [ota-update RELEASE.md](./RELEASE.md)

- [ota-update API documentation](https://Infineon.github.io/ota-update/api_reference_manual/html/index.html)

- [ota-update version](./version.xml)

- [Connectivity utilities API documentation - for cy-log details](https://Infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html)

- [ModusToolbox® Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software)

- [ModusToolbox&trade; code examples](https://github.com/infineon?q=mtb-example-anycloud%20NOT%20Deprecated)
