# Over-the-Air (OTA) Update Middleware Library

The OTA library provides support for downloading Over-The-Air update of the application code running on an Infineon device.
Supported devices:
- PSoC™ 6 MCU with
  - AIROC™ CYW4343W
  - CYW43012 Wi-Fi & Bluetooth® combo chip
- CYW920829M2EVK-02 evaluation board.
- CY8CKIT-062-BLE
- CY8CPROTO-063-BLE
- CYBLE-416045-EVAL
- KIT_XMC72_EVK
- CYW989829M2EVB-01
- CYW955913EVK-01
- CY8CEVAL-062S2-CYW955513SDM2WLIPA

For devices with both Wi-Fi and Bluetooth® Interfaces, the device can use Wi-Fi or Bluetooth® Interfaces.

For <b>WiFi</b> and <b>Ethernet</b>, the OTA library utilizes MQTT or HTTP and TLS to securely connect to an MQTT Broker/HTTP server to download the user application.

For <b>Bluetooth®</b>, the OTA library works with an intermediate peer application on a laptop (or phone) to push the image to the device.

To handle the downloaded upgrade image, the user must implement storage API callbacks based on the chosen bootloader.

<b>NOTE:</b> The library 'ota-bootloader-abstraction' includes storage API callbacks implementation that handles downloaded MCUBootloader/H1-CP based upgrade images. To use these storage API callbacks implementation, user must register these callbacks during OTA agent initialization.

## Library Versions

| Library Version    | Supported MTB version    | Remarks                    |
|--------------------| -------------------------|----------------------------|
| ota-update v4.3.0  | ModusToolbox 3.2         | cysecuretools v5.1 or greater is required |
| ota-update v4.2.0  | ModusToolbox 3.2         | cysecuretools v5.1 or greater is required |
| ota-update v4.1.0  | ModusToolbox 3.2         | cysecuretools v5.1 or greater is required |
| ota-update v4.0.0  | ModusToolbox 3.1         | cysecuretools v5.0 or greater is required |
| ota-update v3.0.1  | ModusToolbox 3.1         | cysecuretools v4.2 or greater is required |
| ota-update v3.0.0  | ModusToolbox 3.0         | cysecuretools v4.2 or greater is required |
| ota-update v2.0.0  | ModusToolbox 3.0         | <b>NOT</b> backwards compatible with ModusToolbox 2.4 |
| ota-update v1.0.0  | ModusToolbox 2.4         | Replaces anycloud-ota (all versions) |
| anycloud-ota       | NA                       | Depricated                 |

## 1. Enabling OTA in an Application

- Before enabling OTA in an application, the user must choose a suitable bootloader and program it on the chosen platform(s).

- The required build environment for creating BOOT and UPGRADE images should be set up.

- Storage callback APIs for handling upgrade images defined in *ota-update* library should be implemented.

- OTA Enabled application is built using ModusToolbox, Incorporating the OTA library and making initialization calls to start the OTA process.

- It runs on CM4 CPU in case of multicore Psoc6 devices and on CM7 CPU in XMC7200 devices.

- In case of 20829 and 89829 devices, it runs on CM33 CPU along with the bootloader application.

- The CYW955913EVK-01 platform features a built-in bootloader. OTA Application runs on CM33 CPU along with the bootloader.

On a virgin device, main application(BOOT image) is programmed / flashed to the device first time. Subsequent versions of the application (so called updates) are downloaded using OTA.

On some devices such as PSoC™ 64 kits, there will be multiple images such as main application and the TFM images that are OTA updatable. This OTA solution allows using TAR format package for updating a single, or multiple images.

The ModusToolbox OTA code examples import the ota-update library automatically.


## 2. Push Model Updates

<b>Bluetooth®</b> updates use a "Push" model. On reset / boot, the device advertises that it can handle an update, and a Host (laptop, phone, etc.) is used to connect and "Push" the update to the device.


## 3. Pull Model Updates

<b>WiFi</b> and <b>Ethernet</b> based protocols(MQTT/HTTP) use a "Pull" model. There are two "Update Flows" that a user can designate. HTTP and MQTT transports can be used for this type of update.<br>

<b>4.1. Direct Flow</b><br>
With "Direct" flow approach, device downloads a known file from the server/broker.

1. HTTP: The device connects to the known HTTP Server and downloads the OTA update image using HTTP `GET` requests, asking for a range of data sequentially until all data is transferred.

2. MQTT: The device connects to the known MQTT Broker topic and publishes a "Request Download" message with a "Unique Topic Name" to the Publisher. The Publisher python application then splits the OTA update image into chunks and publishes them on the unique topic.<br>

<b>4.2. Job Flow</b><br>
With a "Job" flow approach, the device downloads a JSON formatted job document that provides information about where the update file is, its version, and other information concerning the update.

1. The device connects to the server/broker to obtain the Job document.
- HTTP: The device connects to the known HTTP server and uses an HTTP `GET` request to obtain the Job document.
- MQTT: The device subscribes to the known MQTT Broker topic and publishes a request for the Job document.

2. The device uses the Job document to determine if the OTA update image is an appropriate update.

3. The device connects to the data server/broker and downloads the OTA image.
- HTTP: The image is downloaded using HTTP `GET` requests, asking for a range of data sequentially until all data is transferred.
- MQTT: The device sends a "Request Download" message with the "Unique Topic Name" on the known topic. The puthon Publisher application then splits the OTA image into chunks and publishes them on the unique topic.


## 4. Quick start

- To create an OTA application, the user needs to include the ota-update library by creating an *ota-update.mtb* file and placing it in the application's *deps* folder. The contents of the *ota-update.mtb* file should be as follows:
    ```
    https://github.com/Infineon/ota-update#latest-v4.X#$$ASSET_REPO$$/ota-update/latest-v4.X
    ```
    **NOTE** : H1-CP platform support added in *ota-update* version *release-v4.2.0*.

- For MCUBootloader-based OTA code examples on platforms like PSoC,20829, 89829 and XMC7200, Users need to create an *ota-bootloader-abstraction.mtb* file to pull *ota-bootloader-abstraction* library which has storage APIs to handle the MCUBootloader based OTA upgrade files and place it in the application *deps* folder. The contents of *ota-bootloader-abstraction.mtb* should be as follows:
    ```
    https://github.com/Infineon/ota-bootloader-abstraction#latest-v1.X#$$ASSET_REPO$$/ota-bootloader-abstraction/latest-v1.X
    ```
    **NOTE** : H1-CP platform support added in *ota-bootloader-abstraction* version *release-v1.2.0*.

- For WiFi-based OTA code examples on platforms like PSoC6, Users need to create an *wifi-core-freertos-lwip-mbedtls.mtb* file for the WiFi bundle and place it in the application *deps* folder. The contents of *wifi-core-freertos-lwip-mbedtls.mtb* should be as follows:
    ```
    https://github.com/Infineon/wifi-core-freertos-lwip-mbedtls#latest-v1.X#$$ASSET_REPO$$/wifi-core-freertos-lwip-mbedtls/latest-v1.X
    ```
    **NOTE**: ota-update library currently supports *wifi-core-freertos-lwip-mbedtls/latest-v1.X* only.

- For WiFi-based OTA code examples on platforms like CYW955913EVK-01, Users need to create an *wifi-core-threadx-cat5.mtb* file for the WiFi bundle and place it in the application *deps* folder. The contents of *wifi-core-threadx-cat5.mtb* should be as follows:
    ```
    https://github.com/Infineon/wifi-core-threadx-cat5#latest-v1.X#$$ASSET_REPO$$/wifi-core-threadx-cat5/latest-v1.X
    ```

- For Ethernet-based OTA code examples on platforms like XMC7200, Users need to create an *ethernet-core-freertos-lwip-mbedtls.mtb* file for the Ethernet bundle and place it in the application *deps* folder. The contents of *ethernet-core-freertos-lwip-mbedtls.mtb* should be as follows:
    ```
    https://github.com/Infineon/ethernet-core-freertos-lwip-mbedtls#latest-v1.X#$$ASSET_REPO$$/ethernet-core-freertos-lwip-mbedtls/latest-v1.X
    ```
    **NOTE**: ota-update library currently supports *ethernet-core-freertos-lwip-mbedtls/latest-v1.X* only.

- For BLE-based OTA code examples on platforms like PSoC6, 20829 and 89829, Users need to create an *btstack-integration.mtb* file for the btstack and place it in the application *deps* folder. The contents of *btstack-integration.mtb* should be as follows:
    ```
    https://github.com/Infineon/btstack-integration#latest-v4.X#$$ASSET_REPO$$/btstack-integration/latest-v4.X
    ```
    **NOTE**: For BLE-based OTA code examples on platforms like CYW955913EVK-01, required BT libraries are available in ROM. So no need to explicitly pull these libraries.

- For MQTT protocol based OTA code examples, Users need to create an *mqtt.mtb* file to pull the *mqtt* library and place it in the application *deps* folder. The contents of *mqtt.mtb* should be as follows:
    ```
    https://github.com/Infineon/mqtt#latest-v4.X#$$ASSET_REPO$$/mqtt/latest-v4.X
    ```

- For HTTP protocol based OTA code examples, Users need to create an *http-client.mtb* file to pull the *http-client* library and place it in the application *deps* folder. The contents of *http-client.mtb* should be as follows:
    ```
    https://github.com/Infineon/http-client#latest-v1.X#$$ASSET_REPO$$/http-client/latest-v1.X
    ```

- For BLE-based secure OTA updates, the ota-update library uses *Mbed-TLS* crypto APIs to verify downloaded images in FreeRTOS-based OTA applications. Therefore, users need to pull the *Mbed-TLS* library along with other dependent libraries.

- Once you have added the necessary *.mtb* files to the *deps* folder of your application, run the command 'make getlibs' to fetch these libraries and their dependencies.
    ```
    make getlibs
    ```

- Create OTA Application Makefile by referring [OTA Application Makefile Readme](./OTA_MAKEFILE_INFO_README.md).

- For MCUBootloader support, Update OTA Application Makefile by referring *ota-bootloader-abstraction* library [MCUBootloader Support Makefile Readme](https://github.com/Infineon/ota-bootloader-abstraction/blob/master/source/COMPONENT_MCUBOOT/MCUBOOT_OTA_MAKEFILE_INFO_README.md)

- For platforms like CYW955913EVK-01 which has in-built bootloader(H1-CP Bootloader), Update OTA Application Makefile by referring *ota-bootloader-abstraction* library [H1-CP_Bootloader Support Makefile Readme](https://github.com/Infineon/ota-bootloader-abstraction/blob/master/source/COMPONENT_H1_CP/H1-CP_OTA_MAKEFILE_INFO_README.md)

- The "cy_ota_config.h" file provides configuration options to adjust multiple timing values to customize how often to check for updates, and other parameters for the MQTT Broker/HTTP Server connection.
  Copy the configuration file "\<ota-update library\>configs/cy_ota_config.h" file to the directory where your application Makefile is, and adjust as needed.

- To add the required bootloader support at the application level itself, the OTA update library implements a callback mechanism and offloads handling of upgrade images including flash operations to application.

- The application must implement these storage operation callbacks to handle upgrade image based on bootloader choosen and must register callbacks during OTA agent start by giving a parameter of type 'cy_ota_storage_interface_t' in 'cy_ota_agent_start()'.

    ```
    typedef struct cy_ota_storage_interface_s
    {
        cy_ota_file_open           ota_file_open;         /**< Creates and open new receive file for the data chunks as they come in. */
        cy_ota_file_read           ota_file_read;         /**< Reads the flash data starting from the given offset.                   */
        cy_ota_file_write          ota_file_write;        /**< Write a data to the flash at the given offset.                         */
        cy_ota_file_close          ota_file_close;        /**< Close the underlying receive file in the specified OTA context.        */
        cy_ota_file_verify         ota_file_verify;       /**< Authenticate received OTA update file specified in the OTA context.    */
        cy_ota_file_validate       ota_file_validate;     /**< The application has validated the new OTA image.                       */
        cy_ota_file_get_app_info   ota_file_get_app_info; /**< Get Application info like Application version and Application ID.      */
    } cy_ota_storage_interface_t;
    ```

- For more details like storage operation callbacks syntaxes, refer to "\<ota-update library\>include/cy_ota_api.h" .

- Parameters such as MQTT Broker/HTTP server and credentials along with memory operation callbacks are passed into `cy_ota_agent_start()`.

- The OTA Agent runs in a separate background thread, only connecting to MQTT Broker/HTTP server based on the timing configuration.
  For Bluetooth® operation, the OTA library starts the Bluetooth® module and starts advertising.

- The OTA Agent provides a callback mechanism to report stages of connect, download percentage, and errors. The application can override the default OTA Agent behavior for each step, or stop the current download during the callback.

Once the application starts the OTA agent, the OTA agent will contact the MQTT Broker/HTTP server at the defined intervals to check whether an update is available. If available, the update will be downloaded. If the `reset_after_complete` flag was set in the agent parameters, the OTA Agent will automatically reset the device. On the next system reset, Bootloader will perform the update.

## 5. Bootloader Support
Starting from the 4.0 version of the ota-update library, the library is now fully separated from the MCUBootloader. This means that it can function independently and work with any bootloader, as long as the required OTA update handling storage APIs are implemented and registered with OTA agent by the user.

### 5.1 MCUBootloader Support

MCUBootloader is a secure bootloader for 32-bits microcontrollers and users should build it outside of the user OTA application. It is programmed separately to the device before flashing the user OTA application and is not updated for the life of the device.

To support MCUBootloader based OTA using ota-update library, User can implement his own storage operation callbacks to handle upgrade image or can make use of *ota-bootloader-abstraction* library.

*ota-bootloader-abstraction* library has below support.
- Template flashmaps for PSoC6, 20829, 89829 and XMC7200 platforms.
- Template linker files for GCC_ARM, ARM, and IAR toolchains.
- Storage operation callback APIs to handle MCUBootloader based upgrade image.
- Prebuild and Postbuild scripts for generating and signing MCUBootloader based BOOT and UPGRADE image of an OTA Application.

Refer *ota-bootloader-abstraction* library [Readme](https://github.com/Infineon/ota-bootloader-abstraction/blob/master/README.md) for enabling MCUBootloader support.

### 5.2 H1-CP bootloader Support

H1-CP bootloader is programmed in ROM section of CYW955913EVK-01.

To support H1-CP bootloader based OTA using ota-update library, User can implement his own storage operation callbacks to handle upgrade image or can make use of *ota-bootloader-abstraction* library.

*ota-bootloader-abstraction* library has below support for H1-CP bootloader based platforms like CYW955913EVK-01.
- Storage operation callback APIs to handle MCUBootloader based upgrade image.
- README for H1-CP bootloader based OTA.

Refer *ota-bootloader-abstraction* library [Readme](https://github.com/Infineon/ota-bootloader-abstraction/blob/master/README.md) for enabling H1-CP bootloader support.

### 5.3 Other bootloader Support

- Starting from the 4.0 version of the *ota-update*, library can function independently and work with any bootloader.
- Library user needs to implement OTA update handling storage API callbacks defined in 'cy_ota_storage_interface_t'. Refer to "\<ota-update library\>include/cy_ota_api.h" for syntaxes and details.
- Register these storage API callbacks with OTA agent while starting OTA agent.
- OTA agent of *ota-update* library downloads upgrade image of OTA application and calls registered storage API callbacks for handling downloaded upgrade image.
- User can make use of sample scripts for hosting UPGRADE image. These scripts are available in "\<ota-update library\>scripts/".
- Once required bootloader support is added, User can choose PUSH/PULL model to upgrade image on device.

## 6. Example Update Sequence Using BLE (PUSH Model)

- The Host(laptop, phone, etc) downloads the upgrade OTA Image from the server using LAN/WLAN/BT.

- BLE devices running OTA application on reset advertises that they can handle OTA updates.

- A peer app is used to push an updated image to the device. User can use peer app which is available in *\<ota-update library\>scripts/Bluetooth/* or It can be downloaded from the [OTA Peer Application Page](https://github.com/Infineon/btsdk-peer-apps-ota). User can use Windows/iOS/Android version of app for pushing an OTA update image.
    ```
    ./WsOtaUpgrade.exe /file <OTA App Upgrade.bin>
    ```

- From the scanned device list, Select the appropriate BLE OTA-supported device.

- Click on the start button to initiate the upgrade image transfer from host to device.

- Host application pushes the update to the device sequentially.

- BLE OTA device stores the received chunks of upgrade image into its upgrade slot using registered storage API callbacks.

- After transfer completion, On the next reboot bootloader boots upgrade image of application.

## 7. Example Job Flow Update Sequence Using HTTP

1. The device connects to the HTTP server and uses the HTTP `GET` request to obtain the Job document (for example: *ota_update.json*).

2. The device determines whether the OTA update image version is newer than the currently executing application, and the board name is the same, etc.

   - If the OTA update image is accessible through an MQTT Broker, the device creates and subscribes to a unique topic name and sends a "Request Download" message with the "Unique Topic Name" to the Publisher on the known topic `OTAUpdate/CY8CPROTO_062_4343W/notify_publisher`. The Publisher then splits the OTA update image into chunks and publishes them on the unique topic.

   - If the OTA update image is accessible on an HTTP Server, the device connects to the HTTP Server and downloads the OTA update image using an HTTP `GET` request, asking for a range of data sequentially until all data is transferred.

For Job document information, please see Job Document section below.


## 8. Example Job Flow Update Sequence Using MQTT

1. The Publisher Python script (*ota-update/scripts/WiFi_Ethernet/publisher.py*) subscribes to a known topic (for example: *OTAUpdate/CY8CPROTO_062_4343W/publish_notify*) on the specified MQTT Broker.

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

## 11. Using the Subscriber Python Script for testing MQTT Updates

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


## 12. Note on Using Windows 10

When using ModusToolbox, you will need to install the pip requirements to Python in the ModusToolbox installation.

```
\<ModusToolbox\>/tools_3.*/python/python -m pip install -r \<ota-update-library\>scripts/requirements.txt
```


## 13. Supported Toolchains

- GCC
- IAR
- ARM C

For the toolchain version information, please refer to [OTA Release.md](./RELEASE.md).

## 14. Supported OS

- FreeRTOS
- ThreadX


## 15. Supported Kits

- [PSoC™ 6 Wi-Fi BT Prototyping Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062-4343w/) (CY8CPROTO-062-4343W)
- [PSoC™ 62S2 Wi-Fi BT Pioneer Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062s2-43012/) (CY8CKIT-062S2-43012)
- [PSoC™ 62S3 Wi-Fi BT Prototyping Kit ](https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-062s3-4343w/)(CY8CPROTO-062S3-4343W)
- [PSoC™ 64 Secure Boot Wi-Fi BT Pioneer Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-064b0s2-4343w/) (CY8CKIT-064B0S2-4343W)
- [CY8CEVAL-062S2 Evaluation Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ceval-062s2/)(CY8CEVAL-062S2-LAI-4373M2, CY8CEVAL-062S2-MUR-43439M2 and CY8CEVAL-062S2-CYW955513SDM2WLIPA)
- [PSoC™ 6-BLE Pioneer Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-062-ble/) (CY8CKIT-062-BLE)
- [PSoC™ 6 BLE Prototyping Kit](https://www.infineon.com/cms/en/product/evaluation-boards/cy8cproto-063-ble/) (CY8CPROTO-063-BLE)
- [EZ-BLE Arduino Evaluation Board](https://www.infineon.com/cms/en/product/evaluation-boards/cyble-416045-eval/) (CYBLE-416045-EVAL)
- [AIROC™ CYW20829 Bluetooth® LE SoC](https://www.infineon.com/cms/en/product/promopages/airoc20829/) (CYW920829M2EVK-02)
- [XMC7200 Evaluation Kit](https://www.infineon.com/cms/en/product/evaluation-boards/kit_xmc72_evk/) (KIT_XMC72_EVK)
- [AIROC™ CYW989820M2EVB-01 Evaluation kit](https://www.infineon.com/cms/en/product/wireless-connectivity/airoc-bluetooth-le-bluetooth-multiprotocol/airoc-bluetooth-le/cyw20829/)(CYW989820M2EVB-01)
- [CYW955913EVK-01 Wi-Fi Bluetooth&reg; Prototyping Kit (CYW955913EVK-01)](https://www.infineon.com/CYW955913EVK-01)

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

2. Ensure that you have a reliable network connection before starting an OTA update. If your network connection is poor, OTA update may fail due to lost packets or a lost connection.

3. Ensure that you have a fully charged device before starting an OTA update. If your device's battery is low, OTA may fail.

4. When using the ARM compiler with the CYW20829 or BLE platforms, ensure that MBEDTLS_ENTROPY_HARDWARE_ALT is not defined in mbedtls_user_config.h. When MBEDTLS_ENTROPY_HARDWARE_ALT is defined it is possible to encounter an unresolved symbol error during the link phase when using the ARM compiler.


## 21. Additional Information

- [ota-update RELEASE.md](./RELEASE.md)

- [ota-update version](./version.xml)

- [OTA Makefile Information](./OTA_MAKEFILE_INFO_README.md)

- [OTA API reference guide](https://infineon.github.io/ota-update/api_reference_manual/html/index.html)

- [Connectivity utilities API documentation - for cy-log details](https://Infineon.github.io/connectivity-utilities/api_reference_manual/html/group__logging__utils.html)

- [OTA bootloader Abstraction Library documentation](https://infineon.github.io/ota-bootloader-abstraction/api_reference_manual/html/index.html)

- [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/modustoolbox)

- [ModusToolbox&trade; code examples](https://github.com/Infineon/Code-Examples-for-ModusToolbox-Software)

Infineon also provides a wealth of data at www.infineon.com to help you select the right device, and quickly and effectively integrate it into your design.

For PSoC™ 6 MCU devices, see [How to Design with PSoC 6 MCU - KBA223067](https://community.infineon.com/t5/Knowledge-Base-Articles/How-to-Design-with-PSoC-6-MCU-KBA223067/ta-p/248857) in the Infineon community.
