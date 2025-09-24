/*
 * Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

/** @file
 *
 * The Cypress OTA API abstracts the underlying network and
 * platform support for Over-The-Air updates.
 */

/**
 * \addtogroup group_cy_ota Infineon Over The Air (OTA) API
 * \{
 * OTA support for downloading and installing firmware updates.
 *
 * \defgroup group_ota_macros OTA Macros
 * Macros used to define the OTA Agent behavior.
 *
 * \defgroup group_ota_typedefs OTA Typedefs
 * Typedefs used by the OTA Agent.
 *
 * \defgroup group_ota_callback OTA Agent Callback
 * An application can get callbacks from the OTA Agent.
 *
 * \defgroup group_ota_structures OTA Structures
 * Structures used for passing information to and from OTA Agent.
 *
 * \defgroup group_ota_functions OTA Functions
 * Functions to start/stop and query the OTA Agent.
 *
 */

/**
 *
 *
 **********************************************************************/

#ifndef CY_OTA_API_H__
#define CY_OTA_API_H__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include "cy_ota_config.h"          /* Customer OTA overrides.   */
#include "cy_ota_defaults.h"        /* Defaults for OTA.         */
#if defined(COMPONENT_MTB_HAL)
#include "mtb_hal.h"
#else
#include "cyhal.h"
#endif
#include "cybsp.h"
#include "cy_result_mw.h"
#include "cy_log.h"

/*
 * To use WiFi-HTTP support, add this to the makefile.
 * COMPONENT+=OTA_HTTP
 */

/*
 * To use WiFi-MQTT support, add this to the makefile.
 * COMPONENT+=OTA_MQTT
 */

/*
 * To use Bluetooth®, add this define to the makefile.
 * Note that application must provide Bluetooth® initialization and callback handling.
 * Please see full documentation for code snippets.
 * COMPONENT+=OTA_BLUETOOTH
 */

#if defined(COMPONENT_OTA_HTTP) || defined(COMPONENT_OTA_MQTT)
#if defined(CYBSP_WIFI_CAPABLE)
#include "cybsp_wifi.h"
#endif

#include "cy_tcpip_port_secure_sockets.h"
#endif

#ifdef COMPONENT_OTA_HTTP
#include "cy_http_client_api.h"
#endif

#ifdef COMPONENT_OTA_MQTT
#include "cy_mqtt_api.h"
#endif

/* We need another include for Bluetooth® support */
#ifdef COMPONENT_OTA_BLUETOOTH
#include "wiced_bt_gatt.h"
#endif

 /**
  * \addtogroup group_ota_macros
  * \{
  */

/***********************************************************************
 * Error Defines
 ***********************************************************************/

/* Cypress middleware APIs return results of type cy_rslt_t and consist of three parts:
* - module base
* - type
* - error code
*
* \par Result Format
*
  \verbatim
             Module base         Type    Library-specific error code
     +-------------------------+------+------------------------------+
     |          0x0200         | 0x2  |           Error Code         |
     +-------------------------+------+------------------------------+
               14 bits          2 bits            16 bits

  See the macro section of this document for library-specific error codes.
  \endverbatim
*
* The data structure cy_rslt_t is part of cy_result.h.
* In MTB, the PSoC 6 MCU target platform is located in <core_lib/include>.
*
* Module base: This base is derived from CY_RSLT_MODULE_MIDDLEWARE_BASE (defined in cy_result.h) and is an offset of CY_RSLT_MODULE_MIDDLEWARE_BASE.
*              Details of the offset and the middleware base are defined in cy_result_mw.h, which is part of [Github connectivity-utilities] (https://github.com/Infineon/connectivity-utilities).
*              For example, OTA uses CY_RSLT_MODULE_MIDDLEWARE_OTA_UPDATE as the module base, which is 0x020D.
*
* Type: This type is defined in cy_result.h and can be one of CY_RSLT_TYPE_FATAL, CY_RSLT_TYPE_ERROR, CY_RSLT_TYPE_WARNING, or CY_RSLT_TYPE_INFO. MQTT library error codes are of type CY_RSLT_TYPE_ERROR which is 0x2.
*
* Library-specific error codes: These error codes are library-specific and defined in the macro section.
*
* Helper macros used for creating library-specific results are provided as part of cy_result.h.
*/

#define CY_RSLT_OTA_ERROR_BASE    CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_MIDDLEWARE_OTA_UPDATE, 0) /**< OTA Update error code base. */

#define CY_RSLT_OTA_ERROR_UNSUPPORTED           ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  1) ) /**< Unsupported feature.                  */
#define CY_RSLT_OTA_ERROR_GENERAL               ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  2) ) /**< Generic error.                        */
#define CY_RSLT_OTA_ERROR_BADARG                ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  3) ) /**< Bad argument.                         */
#define CY_RSLT_OTA_ERROR_OUT_OF_MEMORY         ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  4) ) /**< Out of Memory error.                  */
#define CY_RSLT_OTA_ERROR_ALREADY_STARTED       ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  5) ) /**< OTA update already started.           */
#define CY_RSLT_OTA_ERROR_MQTT_INIT             ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  6) ) /**< MQTT init failed.                     */
#define CY_RSLT_OTA_ERROR_OPEN_STORAGE          ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  7) ) /**< Could not open local storage.         */
#define CY_RSLT_OTA_ERROR_READ_STORAGE          ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  8) ) /**< Could not Read from local storage.    */
#define CY_RSLT_OTA_ERROR_WRITE_STORAGE         ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE +  9) ) /**< Could not Write to local storage.     */
#define CY_RSLT_OTA_ERROR_CLOSE_STORAGE         ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 10) ) /**< Close local storage error.            */
#define CY_RSLT_OTA_ERROR_CONNECT               ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 11) ) /**< Connection to the server failed.      */
#define CY_RSLT_OTA_ERROR_DISCONNECT            ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 12) ) /**< Server disconnect error.              */
#define CY_RSLT_OTA_ERROR_REDIRECT              ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 13) ) /**< Redirection failure.                  */
#define CY_RSLT_OTA_ERROR_SERVER_DROPPED        ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 14) ) /**< Broker/server disconnected.           */
#define CY_RSLT_OTA_ERROR_MQTT_SUBSCRIBE        ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 15) ) /**< Broker/server Subscribe error.        */
#define CY_RSLT_OTA_ERROR_MQTT_PUBLISH          ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 16) ) /**< Broker/Server Publish error.          */
#define CY_RSLT_OTA_ERROR_GET_JOB               ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 17) ) /**< Failed to get the OTA Job document.   */
#define CY_RSLT_OTA_ERROR_GET_DATA              ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 18) ) /**< Failed to get the OTA image.          */
#define CY_RSLT_OTA_ERROR_NOT_A_HEADER          ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 19) ) /**< No header in the payload.             */
#define CY_RSLT_OTA_ERROR_NOT_A_JOB_DOC         ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 20) ) /**< Job document is invalid.              */
#define CY_RSLT_OTA_ERROR_MALFORMED_JOB_DOC     ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 21) ) /**< Malformed Job document.               */
#define CY_RSLT_OTA_ERROR_WRONG_BOARD           ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 22) ) /**< Board in Job document does not match. */
#define CY_RSLT_OTA_ERROR_INVALID_VERSION       ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 23) ) /**< Invalid version in Job or Packet.     */
#define CY_RSLT_OTA_ERROR_VERIFY                ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 24) ) /**< Verify image failure.                 */
#define CY_RSLT_OTA_ERROR_SENDING_RESULT        ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 25) ) /**< Sending result failed.                */
#define CY_RSLT_OTA_ERROR_APP_RETURNED_STOP     ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 26) ) /**< Callback returned Stop OTA download.  */
#define CY_RSLT_OTA_ERROR_APP_EXCEEDED_RETRIES  ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 27) ) /**< Number of retries exceeded.           */
#define CY_RSLT_OTA_ERROR_TRANSPORT_UNSUPPORTED ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 28) ) /**< HTTP, MQTT or Bluetooth® not supported in this build. */
#define CY_RSLT_OTA_ERROR_NO_IMAGE_INFO         ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 29) ) /**< No OTA image information.             */

#if defined(COMPONENT_OTA_BLUETOOTH) || defined(CY_DOXYGEN)
#define CY_RSLT_OTA_ERROR_BLE_GENERAL           ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 32) ) /**< General Bluetooth® error.                     */
#define CY_RSLT_OTA_ERROR_BLE_VERIFY            ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 33) ) /**< Bluetooth® Verification of download error.    */
#define CY_RSLT_OTA_ERROR_BLE_STORAGE           ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 34) ) /**< Bluetooth® BLE Transport Error.               */
#endif

#define CY_RSLT_OTA_ERROR_SMIF_TIMEOUT          ( (cy_rslt_t)(CY_RSLT_OTA_ERROR_BASE + 64) ) /**< SMIF timeout.             */

#define CY_RSLT_OTA_INFO_BASE    CY_RSLT_CREATE(CY_RSLT_TYPE_INFO, CY_RSLT_MODULE_MIDDLEWARE_OTA_UPDATE, 0)   /**< Base for informational results. */
#define CY_RSLT_OTA_EXITING                     ( (cy_rslt_t)(CY_RSLT_OTA_INFO_BASE + 1) ) /**< OTA Agent exiting.                         */
#define CY_RSLT_OTA_ALREADY_CONNECTED           ( (cy_rslt_t)(CY_RSLT_OTA_INFO_BASE + 2) ) /**< OTA already connected.                     */
#define CY_RSLT_OTA_CHANGING_SERVER             ( (cy_rslt_t)(CY_RSLT_OTA_INFO_BASE + 3) ) /**< Data broker/server different from job.     */

#define CY_RSLT_OTA_USE_JOB_FLOW                ( (cy_rslt_t)(CY_RSLT_SUCCESS          ) ) /**< Use Job flow for update.                 */
#define CY_RSLT_OTA_USE_DIRECT_FLOW             ( (cy_rslt_t)(CY_RSLT_OTA_INFO_BASE + 4) ) /**< Use Direct flow for update.              */
#define CY_RSLT_OTA_NO_UPDATE_AVAILABLE         ( (cy_rslt_t)(CY_RSLT_OTA_INFO_BASE + 5) ) /**< No OTA update on the server.             */

/** \} group_ota_macros */

/***********************************************************************
 * Define min / max values
 ***********************************************************************/

/**
  * \addtogroup group_ota_typedefs
  * \{
  */

/**
 * @brief Macro combo to add quotes around a define.
 * Used to define a string value for CY_TARGET_BOARD.
 * */
#define ADD_QUOTES(str) #str
/**
 * @brief Macro combo to add quotes around a define.
 * Used to define a string value for CY_TARGET_BOARD.
 * */
#define EXPAND_AND_ADD_QUOTES(str) ADD_QUOTES(str)
/**
 * @brief Macro combo to add quotes around a define.
 * Used to define a string value for CY_TARGET_BOARD.
 * */
#ifndef CY_TARGET_BOARD_STRING
#define CY_TARGET_BOARD_STRING EXPAND_AND_ADD_QUOTES(CY_TARGET_BOARD)
#endif

/**
 * @brief Size for the HTTP filename to get the OTA image.
 */
#define CY_OTA_MQTT_FILENAME_BUFF_SIZE          (256)

/**
 * @brief Size for the unique MQTT topic to get the OTA image.
 */
#define CY_OTA_MQTT_UNIQUE_TOPIC_BUFF_SIZE      (256)

/**
 * @brief Size of buffer for JSON update document
 */
#define CY_OTA_JSON_DOC_BUFF_SIZE               (1024)

/**
 * @brief Size of data chunks for each transfer.
 *
 * Override in cy_ota_config.h.
 */
#ifndef CY_OTA_CHUNK_SIZE
#define CY_OTA_CHUNK_SIZE                       (4096)
#endif

/**
 * @brief Size of data chunks header for each transfer.
 *
 * Override in cy_ota_config.h.
 */
#ifndef CY_OTA_CHUNK_HEADER_SIZE
#define CY_OTA_CHUNK_HEADER_SIZE                (1024)
#endif

/**
 * @brief Size of the string for the HTTP file name.
 */
#define CY_OTA_HTTP_FILENAME_SIZE               (256)

/**
 * @brief First part of the topic to subscribe / publish.
 *
 * Topic for the device to send messages to the Publisher:
 *  "COMPANY_TOPIC_PREPEND / BOARD_NAME / PUBLISHER_LISTEN_TOPIC"
 *
 * Override in cy_ota_config.h.
 */
#ifndef COMPANY_TOPIC_PREPEND
#define COMPANY_TOPIC_PREPEND                   "OTAUpdate"
#endif

/**
 * @brief Last part of the topic to subscribe.
 *
 * Topic for the device to send messages to the Publisher:
 *  "COMPANY_TOPIC_PREPEND / BOARD_NAME / PUBLISHER_LISTEN_TOPIC"
 *
 * Override in cy_ota_config.h.
 */
#ifndef PUBLISHER_LISTEN_TOPIC
#define PUBLISHER_LISTEN_TOPIC                  "publish_notify"
#endif

/**
 * @brief "Magic" value placed in the MQTT Data Payload header.
 *
 * Override in cy_ota_config.h.
 */
#ifndef CY_OTA_MQTT_MAGIC
#define CY_OTA_MQTT_MAGIC                       "OTAImage"
#endif

/**
 * @brief End of topic to send a message to the Publisher for Direct download.
 *
 * Override in cy_ota_config.h.
 */
#ifndef PUBLISHER_DIRECT_TOPIC
#define PUBLISHER_DIRECT_TOPIC                  "OTAImage"
#endif

/**
 * @brief Publisher response when no update is available.
 */
#define NOTIFICATION_RESPONSE_NO_UPDATES        "No Update Available"

/**
 * @brief Publisher response when an update is available.
 */
#define NOTIFICATION_RESPONSE_UPDATES           "Update Available"

/**
 * @brief Publisher response when receiving results.
 */
#define NOTIFICATION_RESPONSE_RESULT_RECEIVED   "Result Received"

/**
 * @brief Topic to receive messages from the Publisher.
 */
#define CY_OTA_SUBSCRIBE_AVAIL_TOPIC        COMPANY_TOPIC_PREPEND "/" CY_TARGET_BOARD_STRING "/" DEVICE_LISTEN_TOPIC

/**
 * @brief Topic to send messages to the Publisher.
 */
#define SUBSCRIBER_PUBLISH_TOPIC            COMPANY_TOPIC_PREPEND "/" CY_TARGET_BOARD_STRING "/" PUBLISHER_LISTEN_TOPIC

/**
 * @brief Minimum interval check time.
 *
 * Minimum interval time for any timing specification in seconds.
 */
#define CY_OTA_INTERVAL_SECS_MIN        (5)

/**
 * @brief Maximum interval check time.
 *
 * This applies to:
 *  Initial wait after call to cy_ota_agent_start() before contacting server.
 *  Wait value after failing or completing an OTA download.
 */
#define CY_OTA_INTERVAL_SECS_MAX        (60 * 60 * 24 * 365)     /* one year */

/*
 * Check for valid define values.
 */
#if (CY_OTA_INITIAL_CHECK_SECS < CY_OTA_INTERVAL_SECS_MIN)
    #error  "CY_OTA_INITIAL_CHECK_SECS must be greater or equal to CY_OTA_INTERVAL_SECS_MIN."
#endif
#if (CY_OTA_INITIAL_CHECK_SECS > CY_OTA_INTERVAL_SECS_MAX)
    #error  "CY_OTA_INITIAL_CHECK_SECS must be less than CY_OTA_INTERVAL_SECS_MAX."
#endif

#if (CY_OTA_NEXT_CHECK_INTERVAL_SECS < CY_OTA_INTERVAL_SECS_MIN)
    #error  "CY_OTA_NEXT_CHECK_INTERVAL_SECS must be greater or equal to CY_OTA_INTERVAL_SECS_MIN."
#endif
#if (CY_OTA_NEXT_CHECK_INTERVAL_SECS > CY_OTA_INTERVAL_SECS_MAX)
    #error  "CY_OTA_NEXT_CHECK_INTERVAL_SECS must be less than CY_OTA_INTERVAL_SECS_MAX."
#endif

/* Don't check the minimum because this number can be 0. */
#if (CY_OTA_CHECK_TIME_SECS > CY_OTA_INTERVAL_SECS_MAX)
    #error  "CY_OTA_CHECK_TIME_SECS must be less than CY_OTA_INTERVAL_SECS_MAX."
#endif

#if (CY_OTA_RETRY_INTERVAL_SECS < CY_OTA_INTERVAL_SECS_MIN)
    #error  "CY_OTA_RETRY_INTERVAL_SECS must be greater or equal to CY_OTA_INTERVAL_SECS_MIN."
#endif
#if (CY_OTA_RETRY_INTERVAL_SECS > CY_OTA_INTERVAL_SECS_MAX)
    #error  "CY_OTA_RETRY_INTERVAL_SECS must be less than CY_OTA_INTERVAL_SECS_MAX."
#endif

#if (CY_OTA_PACKET_INTERVAL_SECS > CY_OTA_INTERVAL_SECS_MAX)
    #error  "CY_OTA_PACKET_INTERVAL_SECS must be less than CY_OTA_INTERVAL_SECS_MAX."
#endif

/***********************************************************************
 *
 * defines & enums
 *
 **********************************************************************/

/**
 * @brief The Message Name field in a JSON Job document.
 *
 * The Message Name field indicates the type of request/info that is contained in the Job document.
 */
#define CY_OTA_MESSAGE_FIELD                "Message"

/**
 * @brief The Manufacturer Name field in a JSON Job document.
 *
 * The Manufacturer Name can be used to determine if the OTA image available is correct.
 */
#define CY_OTA_MANUF_FIELD                  "Manufacturer"

/**
 * @brief The Manufacturer ID field in a JSON Job document.
 *
 * The Manufacturer ID can be used to determine whether the OTA image available is correct.
 */
#define CY_OTA_MANUF_ID_FIELD               "ManufacturerID"

/**
 * @brief The Product ID field in a JSON Job document.
 *
 * The Product ID can be used to determine whether the OTA image available is correct.
 */
#define CY_OTA_PRODUCT_FIELD                "Product"

/**
 * @brief The Serial Number field in a JSON Job document.
 *
 * The Serial Number can be used to determine whether the OTA image available is correct.
 */
#define CY_OTA_SERIAL_NUMBER_FIELD          "SerialNumber"

/**
 * @brief The Version field in a JSON Job document.
 *
 * The Version can be used to determine whether the OTA image available is an upgrade (for example,"1.6.3").
 */
#define CY_OTA_VERSION_FIELD                "Version"

/**
 * @brief The Board field in a JSON Job document.
 *
 * The Board can be used to determine whether the OTA image available is correct.
 */
#define CY_OTA_BOARD_FIELD                  "Board"

/**
 * @brief The Connection field in a JSON Job document.
 *
 * The Connection defines the connection type to use to obtain the OTA image ("HTTP" or "MQTT").
 */
#define CY_OTA_CONNECTION_FIELD             "Connection"

/**
 * @brief The Broker field for MQTT connection in a JSON Job document.
 *
 * MQTT is the MQTT Broker used to obtain the OTA image.
 */
#define CY_OTA_BROKER_FIELD                 "Broker"

/**
 * @brief The Port field in a JSON Job document.
 *
 * Port is used for HTTP or MQTT connection to obtain the OTA image.
 */
#define CY_OTA_PORT_FIELD                   "Port"

/**
 * @brief The Server field is for HTTP connection in a JSON Job document.
 *
 * The server is the HTTP server used to obtain the OTA image. See also @ref CY_OTA_PORT_FIELD @ref CY_OTA_FILE_FIELD.
 */
#define CY_OTA_SERVER_FIELD                 "Server"

/**
 * @brief The File field is for HTTP connection in a JSON Job document.
 *
 * The File is the name of the OTA image on the HTTP server. See also @ref CY_OTA_PORT_FIELD.
 */
#define CY_OTA_FILE_FIELD                   "File"

/**
 * @brief The Offset field is for a JSON Chunk Request document.
 *
 * The offset is where in the OTA image to start a chunk transfer.
 */
#define CY_OTA_OFFSET_FIELD                 "Offset"

/**
 * @brief The Size field is for a JSON Chunk Request document.
 *
 * The Size is the size of the chunk to read from the OTA image for a chunk transfer.
 */
#define CY_OTA_SIZE_FIELD                   "Size"

/**
 * @brief The Unique Topic field is the name of the topic used for receiving Job/data from the Publisher.
 *
 * The Unique Topic Name used for receipt of the Job/data from the Publisher.
 */
#define CY_OTA_UNIQUE_TOPIC_FIELD           "UniqueTopicName"

/**
 * @brief The MQTT Connection Type used in a JSON Job document.
 *
 * Test for this string in a JSON Job document for MQTT type connection. @ref CY_OTA_CONNECTION_FIELD.
 */
#define CY_OTA_MQTT_STRING                  "MQTT"
/**
 * @brief The HTTP connection type used in a JSON Job document.
 *
 * Test for this string in a JSON Job document for HTTP type connection. @ref CY_OTA_CONNECTION_FIELD.
 */
#define CY_OTA_HTTP_STRING                  "HTTP"
/**
 * @brief The HTTPS connection type used in a JSON Job document.
 *
 * Test for this string in a JSON Job document for HTTPS type connection. @ref CY_OTA_CONNECTION_FIELD.
 */
#define CY_OTA_HTTPS_STRING                 "HTTPS"

/**
 *  @brief The Max length of the "Message" field in a JSON Job document.
 */
#define CY_OTA_MESSAGE_LEN                  (32)

/**
 *  @brief The Max length of the "Manufacturer" field in a JSON Job document.
 */
#define CY_OTA_JOB_MANUF_LEN                (64)

/**
 *  @brief The Max length of the "ManufacturerID" field in a JSON Job document.
 */
#define CY_OTA_JOB_MANUF_ID_LEN             (16)

/**
 *  @brief The Max length of the "ProductID" field in a JSON Job document.
 */
#define CY_OTA_JOB_PRODUCT_ID_LEN           (64)

/**
 *  @brief The Max length of the "SerialNumber" field in a JSON Job document.
 */
#define CY_OTA_JOB_SERIAL_NUMBER_LEN        (32)

/**
 *  @brief The Max length of the "Version" field in a JSON Job document.
 */
#define CY_OTA_JOB_VERSION_LEN              (16)

/**
 *  @brief The Max length of the "Board" field in a JSON Job document.
 */
#define CY_OTA_JOB_BOARD_LEN                (48)

/**
 *  @brief The Max length of the "Broker" and "Server" fields in a JSON Job document.
 */
#define CY_OTA_JOB_URL_BROKER_LEN           (256)

/**
 * @brief The MQTT Broker port for a non-TLS connection.
 */
#define CY_OTA_MQTT_BROKER_PORT             (1883)

/**
 * @brief The MQTT Broker port for a TLS connection.
 */
#define CY_OTA_MQTT_BROKER_PORT_TLS         (8883)

/**
 * @brief The MQTT Broker port for a TLS connection with certificates.
 */
#define CY_OTA_MQTT_BROKER_PORT_TLS_CERT    (8884)


/**
 * @brief The HTTP connection port for a non-TLS connection.
 */
#define CY_OTA_HTTP_SERVER_PORT             (80)

/**
 * @brief The HTTP connection port for a TLS connection.
 */
#define CY_OTA_HTTP_SERVER_PORT_TLS         (443)


/**
 * @brief The Type of OTA update flow.
 * Direct Flow: The application knows the location of the update and directly downloads it.
 * Job Flow: The application loads a Job document which describes where the update is located.
 */
typedef enum
{
    CY_OTA_JOB_FLOW = 0,                /**< Use the Job flow to get the OTA Job document and data.  */
    CY_OTA_DIRECT_FLOW                  /**< Get the OTA image data directly.                        */
} cy_ota_update_flow_t;
/**
 * @brief The connection type to use.
 */
typedef enum
{
    CY_OTA_CONNECTION_UNKNOWN = 0,      /**< Unknown connection type.       */
    CY_OTA_CONNECTION_MQTT,             /**< Use MQTT connection.           */
    CY_OTA_CONNECTION_HTTP,             /**< Use HTTP connection.           */
    CY_OTA_CONNECTION_HTTPS,            /**< Use HTTPS connection.          */
    CY_OTA_CONNECTION_BLE,              /**< Use Bluetooth® connection       */
} cy_ota_connection_t;

/**
 * @brief The MQTT session clean flag.
 *
 * This flag signals the MQTT Broker to start a new session or restart an existing session.
 */
typedef enum
{
    CY_OTA_MQTT_SESSION_CLEAN = 0,      /**< Start a clean MQTT session with the Broker.         */
    CY_OTA_MQTT_SESSION_RESTART         /**< Restart an existing MQTT session with the Broker.   */
} cy_ota_mqtt_session_type_t;


/**
 * @brief The reasons for OTA callbacks to the application.
 */
typedef enum
{
    CY_OTA_REASON_STATE_CHANGE = 0,     /**< OTA Agent state changed, see cb_data->state.    */
    CY_OTA_REASON_SUCCESS,              /**< State function successful.                      */
    CY_OTA_REASON_FAILURE,              /**< State function failed.                          */

    CY_OTA_LAST_REASON                  /**< Placeholder, Do not use.                        */
} cy_ota_cb_reason_t;

/**
 * @brief The OTA state.
 */
typedef enum
{
    CY_OTA_STATE_NOT_INITIALIZED= 0,    /**< OTA system is not initialized.                          */
    CY_OTA_STATE_EXITING,               /**< OTA system is exiting.                                  */
    CY_OTA_STATE_INITIALIZING,          /**< OTA system is initializing.                             */
    CY_OTA_STATE_AGENT_STARTED,         /**< OTA Agent has started.                                  */
    CY_OTA_STATE_AGENT_WAITING,         /**< OTA Agent is waiting for the timer to start.            */

    CY_OTA_STATE_STORAGE_OPEN,          /**< OTA Agent will call cy_storage_open.                    */
    CY_OTA_STATE_STORAGE_WRITE,         /**< OTA Agent will call cy_storage_write.                   */
    CY_OTA_STATE_STORAGE_CLOSE,         /**< OTA Agent will call cy_storage_close.                   */

    CY_OTA_STATE_START_UPDATE,          /**< OTA Agent will determine the flow: Job or Direct.       */

    CY_OTA_STATE_JOB_CONNECT,           /**< OTA Agent will connect to the Job Broker/server.        */
    CY_OTA_STATE_JOB_DOWNLOAD,          /**< OTA Agent will get the Job from the Broker/server.      */
    CY_OTA_STATE_JOB_DISCONNECT,        /**< OTA Agent will disconnect from the Job Broker/server.   */

    CY_OTA_STATE_JOB_PARSE,             /**< OTA Agent will parse the Job.                           */
    CY_OTA_STATE_JOB_REDIRECT,          /**< OTA Agent will use the Job to change the Broker/server. */

    CY_OTA_STATE_DATA_CONNECT,          /**< OTA Agent will connect to the data Broker/server.       */
    CY_OTA_STATE_DATA_DOWNLOAD,         /**< OTA Agent will download the data.                       */
    CY_OTA_STATE_DATA_DISCONNECT,       /**< OTA Agent will disconnect from the data Broker/server.  */

    CY_OTA_STATE_VERIFY,                /**< OTA Agent will verify the download.                     */

    CY_OTA_STATE_RESULT_REDIRECT,       /**< OTA Agent will redirect back to the initial connection. */

    CY_OTA_STATE_RESULT_CONNECT,        /**< OTA Agent will connecting to the result Broker/server.  */
    CY_OTA_STATE_RESULT_SEND,           /**< OTA Agent will send the result.                         */
    CY_OTA_STATE_RESULT_RESPONSE,       /**< OTA Agent will wait for a result response.              */
    CY_OTA_STATE_RESULT_DISCONNECT,     /**< OTA Agent will disconnect from the result Broker/server.*/

    CY_OTA_STATE_OTA_COMPLETE,          /**< OTA Agent is done with current session.
                                             OTA Agent will reboot or will wait.                     */

    CY_OTA_NUM_STATES                   /**< Not used, placeholder. */
} cy_ota_agent_state_t;

#ifdef COMPONENT_OTA_BLUETOOTH

/* Command definitions for the OTA FW upgrade */
#define CY_OTA_UPGRADE_COMMAND_PREPARE_DOWNLOAD               1
#define CY_OTA_UPGRADE_COMMAND_DOWNLOAD                       2
#define CY_OTA_UPGRADE_COMMAND_VERIFY                         3
#define CY_OTA_UPGRADE_COMMAND_ABORT                          4

/* Event definitions for the OTA FW upgrade */
#define CY_OTA_UPGRADE_STATUS_OK                              0
#define CY_OTA_UPGRADE_STATUS_BAD                             1

/* Starting value for crc file check */
#define CRC32_INITIAL_VALUE             0x00000000

#endif  /* COMPONENT_OTA_BLUETOOTH */

/**
 *  The OTA context pointer.
 *
 * Returned from cy_ota_agent_start() and used for subsequent calls.
 */
typedef void *cy_ota_context_ptr;

/**
 * @brief Results from the registered OTA callback.
 *
 * These are the return values from the application to the OTA Agent at the end of a callback.
 *
 * Of the callback reasons, \ref cy_ota_cb_reason_t, the OTA Agent checks only the application
 * callback return values when the reason is CY_OTA_REASON_STATE_CHANGE. The other callback reasons
 * are informational for the application.
 *
 * When the callback reason is CY_OTA_REASON_STATE_CHANGE, the OTA Agent is about to call the function
 * to act on the new state. The application callback function can return a value that affects the
 * OTA Agent.
 *
 * There are a few state changes that will pick up changes made by the application and use those changes.
 *
 *      cb_data->reason             - Reason for the callback.
 *      cb_data->cb_arg             - Argument provided to cy_ota_agent_start() params.
 *      cb_data->ota_agt_state      - CY_OTA_REASON_STATE_CHANGE:
 *                                      - Start of cy_ota_agent_state_t, about to call the default function.
 *                                   - CY_OTA_REASON_SUCCESS:
 *                                      - Error function succeeded.
 *                                   - CY_OTA_REASON_FAILURE:
 *                                      - Error function failed.
 *      cb_data->error              - Same as cy_ota_get_last_error().
 *      cb_data->storage            - For the CY_OTA_STATE_STORAGE_WRITE callback, points to the storage info.
 *      cb_data->total_size         - Total # bytes to be downloaded.
 *      cb_data->bytes_written      - Total # bytes written.
 *      cb_data->percentage         - Percentage of data downloaded.
 *      cb_data->connection_type    - Current connection - MQTT or HTTP.
 *      cb_data->broker_server      - Current server URL and port.
 *      cb_data->credentials        - Pointer to the current credentials being used.
 *      cb_data->mqtt_connection    - Current MQTT connection (NULL if not connected):
 *                                      - For CY_OTA_REASON_STATE_CHANGE
 *                                        - CY_OTA_STATE_JOB_CONNECT or
 *                                          CY_OTA_STATE_DATA_CONNECT or
 *                                          CY_OTA_STATE_RESULT_CONNECT
 *                                      - OTA Agent about to connect, if Application fills this in;
 *                                        then the OTA Agent will use the provided connection instance.
 *      cb_data->http_connection    - Current HTTP connection (NULL if not conencted).
 *                                      - For CY_OTA_REASON_STATE_CHANGE:
 *                                        - CY_OTA_STATE_JOB_CONNECT or
 *                                          CY_OTA_STATE_DATA_CONNECT or
 *                                          CY_OTA_STATE_RESULT_CONNECT
 *                                      - OTA Agent about to connect, if Application fills this in;
 *                                        then the OTA Agent will use the provided connection instance.
 *      cb_data->file               - Filename for HTTP GET command.
 *      cb_data->unique_topic       - Unique topic name to receive data from MQTT Broker/Publisher.
 *      cb_data->json_doc           - JSON document used for:
 *                                      MQTT:
 *                                      - Send to request a Job from the MQTT Broker.
 *                                      - Received Job from the MQTT Broker.
 *                                      - Send to request data from the MQTT Broker.
 *                                      - Send to report the result to the MQTT Broker.
 *                                      HTTP
 *                                      - HTTP GET command for the Job.
 *                                      - Received the Job from the HTTP server.
 *                                      - HTTP GET command for data.
 *                                      - HTTP PUT command to the report result to the HTTP server (Not implemented yet).
 */

typedef enum
{
    CY_OTA_CB_RSLT_OTA_CONTINUE = 0,  /**< OTA Agent to continue with the function, using the modified data from the application. */
    CY_OTA_CB_RSLT_OTA_STOP,          /**< OTA Agent to end the current update session (do not quit the OTA Agent).               */
    CY_OTA_CB_RSLT_APP_SUCCESS,       /**< Application completed task; OTA Agent uses success.                                    */
    CY_OTA_CB_RSLT_APP_FAILED,        /**< Application failed task; OTA Agent uses failure.                                       */

    CY_OTA_CB_NUM_RESULTS             /**< Placeholder, do not use. */
} cy_ota_callback_results_t;

/** \} group_ota_typedefs */

/***********************************************************************
 *
 * Structures
 *
 **********************************************************************/

/**
 * \addtogroup group_ota_structures
 * \{
 *  Structures used for network connection and OTA Agent behavior.
 */

/**
 * @brief Struct to hold information on where to write the data.
 * Information on where to store the downloaded chunk of the OTA image.
 * \struct cy_ota_storage_write_info_t
 */
typedef struct
{
    uint32_t        total_size;     /**< Pass the total size to the storage module; 0 = disregard.  */

    uint32_t        offset;         /**< Offset into file/area where the data belongs.              */
    uint8_t         *buffer;        /**< Pointer to the buffer with the chunk of data.              */
    uint32_t        size;           /**< Size of the data in the buffer.                            */

                                    /* When using MQTT connection.                                  */
    uint16_t        packet_number;  /**< MQTT: The packet number of this chunk.                     */
    uint16_t        total_packets;  /**< MQTT: Total packets (chunks) in the OTA image.             */
} cy_ota_storage_write_info_t;

/**
 * @brief Struct to hold information on from where to read the data.
 * Information on from where to read the downloaded chunk of the OTA image.
 */
typedef cy_ota_storage_write_info_t cy_ota_storage_read_info_t;

/**
 * @brief OTA Agent storage context parameters structure.
 *
 * These parameters are for describing some storage aspects of the OTA Agent.
 * \struct cy_ota_storage_context_t
 */
typedef struct cy_ota_storage_context_s
{
    void        *storage_loc;               /**< can be cast as flash_area or FILE as needed                    */
    uint8_t     imgID;                      /**< Application ID or Image ID in case of Multi-App Updates        */
    uint32_t    total_image_size;           /**< Total size of OTA Image                                        */
    uint32_t    total_bytes_written;        /**< Number of bytes written to FLASH                               */
    uint32_t    last_offset;                /**< Last offset written to from cy_ota_storage_write()             */
    uint32_t    last_size;                  /**< last size of data written from cy_ota_storage_write()          */
    uint16_t    last_packet_received;       /**< Last Packet of data we have received                           */
    uint16_t    total_packets;              /**< Total number of Packets of data for the OTA Image              */
    uint16_t    num_packets_received;       /**< Total number of Packets received                               */
    uint16_t    last_num_packets_received;  /**< last time we saw how many were received, per-packet timer      */
    uint8_t     ota_is_tar_archive;         /**< !=0, this is a tar file                                        */
    uint8_t     reboot_upon_completion;     /**< 1 = Automatically reboot upon download completion and verify.  */
    uint8_t     validate_after_reboot;      /**< 0 = OTA will set upgrade image as permanent before reboot.
                                             *   1 = The application should validate and set the upgrade image as the permanent image after reboot. */
}cy_ota_storage_context_t;

#ifdef COMPONENT_OTA_HTTP
/**
 * @brief OTA HTTP-specific connection parameters.
 * \struct cy_ota_http_params_t
 */
typedef struct cy_ota_http_params_s
{
    cy_awsport_server_info_t        server;     /**< HTTP server to get the Job or OTA image.
                                                 *    Set use_get_job_flow in cy_ota_network_params_t
                                                 *    to determine the use of the Job document.
                                                 */

    const char                      *file;      /**< File name for the Job or OTA image.
                                                 *    Set use_get_job_flow in cy_ota_network_params_t
                                                 *    to determine the OTA update flow.
                                                 */
    cy_awsport_ssl_credentials_t    credentials;/**< Setting credentials uses TLS (NULL == non-TLS). */
} cy_ota_http_params_t;
#endif  /* COMPONENT_OTA_HTTP */

#ifdef COMPONENT_OTA_MQTT

/**
 *  OTA MQTT-specific connection parameters.
 *  \struct cy_ota_mqtt_params_t
 */
typedef struct cy_ota_mqtt_params_s
{
    bool                        awsIotMqttMode;    /**< 0 = normal MQTT; 1 = Special Amazon mode.    */
    const char                  *pIdentifier;      /**< Pointer to the device ID.                    */
    uint8_t                     numTopicFilters;   /**< Number of topics for MQTT Subscribe.         */
    const char                  **pTopicFilters;   /**< Topic filter text.                           */
    cy_ota_mqtt_session_type_t  session_type;      /**< @ref cy_ota_mqtt_session_type_t              */

    cy_awsport_server_info_t        broker;            /**< Broker to get the Job or OTA image.
                                                    *    Set use_get_job_flow in cy_ota_network_params_t
                                                    *    to determine the OTA update flow.
                                                    */
    cy_awsport_ssl_credentials_t    credentials;     /**< Setting credentials uses TLS (NULL == non-TLS). */

} cy_ota_mqtt_params_t;

#endif  /* COMPONENT_OTA_MQTT   */

/**
 * @brief Structure passed to the callback.
 *
 * This holds read/write data for the application to adjust various things in the callback.
 * The callback is not in an interrupt context, but try to keep it quick!
 * After your function returns, this structure is not available.
 * \struct cy_ota_cb_struct_t
 */
typedef struct cy_ota_cb_struct_s
{
    cy_ota_cb_reason_t          reason;         /**< Reason for the callback.                               */
    void                        *cb_arg;        /**< Argument passed when registering the callback.         */

    cy_ota_agent_state_t        ota_agt_state;  /**< Current OTA Agent state.                               */
    cy_rslt_t                   error;          /**< Current OTA Agent error status.                        */

    cy_ota_storage_write_info_t *storage;       /**< Pointer to a chunk of data to write.                   */
    uint32_t                    total_size;     /**< Total # bytes to be downloaded.                        */
    uint32_t                    bytes_written;  /**< Total # bytes downloaded.                              */
    uint32_t                    percentage;     /**< Percentage of bytes downloaded.                        */

    cy_ota_connection_t         connection_type; /**< Connection type @ref cy_ota_connection_t.             */

#if defined(COMPONENT_OTA_HTTP) || defined(COMPONENT_OTA_MQTT)
    cy_awsport_server_info_t    broker_server;   /**< MQTT Broker (or HTTP server) for connection.          */
    cy_awsport_ssl_credentials_t *credentials;     /**< Setting credentials uses TLS (NULL == non-TLS).     */
#endif

#ifdef COMPONENT_OTA_MQTT
    cy_mqtt_t                   mqtt_connection; /**< For Passing MQTT connection instance                  */
    char                        unique_topic[CY_OTA_MQTT_UNIQUE_TOPIC_BUFF_SIZE + 1];   /**< Topic for receiving the OTA data. */
#endif
#ifdef COMPONENT_OTA_HTTP
    cy_http_client_t            http_connection; /**< For Passing HTTP connection instance                  */
#endif

#if defined(COMPONENT_OTA_HTTP) || defined(COMPONENT_OTA_MQTT)
    char                        file[CY_OTA_MQTT_FILENAME_BUFF_SIZE]; /**< File name to request OTA data.   */

    /* For Get Job Message. */
    char                        json_doc[CY_OTA_JSON_DOC_BUFF_SIZE + 1]; /**< Message to request the OTA data.  */
#endif

} cy_ota_cb_struct_t;

/**
 * @brief OTA App file information.
 *
 * These parameters are for describing properties of application image(BOOT/UPGRADE).
 * \struct cy_ota_app_info_t
 */
typedef struct cy_ota_app_info_s
{
    uint16_t app_id;     /**< Application ID.             */
    uint8_t  major;      /**< Application major version.  */
    uint8_t  minor;      /**< Application major version.  */
    uint8_t  build;      /**< Application build number.   */
    uint8_t  revision;   /**< Application revision number.*/
    uint8_t  slot;       /**< Application slot number.    */
    uint16_t company_id; /**< Company ID.                 */
    uint16_t product_id; /**< Product ID.                 */
} cy_ota_app_info_t;

/** \} group_ota_structures */


/**
 * \addtogroup group_ota_callback
 * \{
 */

/**
 * @brief OTA Agent callback to the application.
 *
 * The application can use the callback to override default OTA functions.
 *
 * @param[in,out]   cb_data   Current information that Application callback can use/modify.
 *
 * @return          CY_OTA_CB_RSLT_OTA_CONTINUE
 *                  CY_OTA_CB_RSLT_OTA_STOP
 *                  CY_OTA_CB_RSLT_APP_SUCCESS
 *                  CY_OTA_CB_RSLT_APP_FAILED
 */
typedef cy_ota_callback_results_t ( *cy_ota_callback_t ) ( cy_ota_cb_struct_t *cb_data );

/**
 * @brief Creates and open new receive file for the data chunks as they come in.
 *
 * @note ota-update library expects user to implement this callback function.
 *
 * @note Opens the file indicated in the OTA agent storage context in the MCU file system.
 *
 * @note The previous image may be present in the designated image download partition or file, so the
 * partition or file must be completely erased or overwritten in this routine.
 *
 * @param[in]   storage_ptr     Pointer to the OTA Agent storage context @ref cy_ota_storage_context_t
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_OPEN_STORAGE
 */
typedef cy_rslt_t ( * cy_ota_file_open ) ( cy_ota_storage_context_t *storage_ptr );

/**
 * @brief Reads the flash data starting from the given offset.
 *
 * @note ota-update library expects user to implement this callback function.
 *
 * @param[in]   storage_ptr     Pointer to the OTA Agent storage context @ref cy_ota_storage_context_t
 * @param[in]   chunk_info      Pointer to the chunk information which includes buffer, length, and offset.
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_READ_STORAGE
 */
typedef cy_rslt_t ( * cy_ota_file_read ) ( cy_ota_storage_context_t *storage_ptr, cy_ota_storage_read_info_t *chunk_info );

/**
 * @brief Write a data to the flash at the given offset.
 *
 * @note ota-update library expects user to implement this callback function.
 *
 * @param[in]   storage_ptr     Pointer to the OTA Agent storage context @ref cy_ota_storage_context_t
 * @param[in]   chunk_info      Pointer to the chunk information which includes buffer, length, and offset.
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_WRITE_STORAGE
 */
typedef cy_rslt_t ( * cy_ota_file_write ) ( cy_ota_storage_context_t *storage_ptr, cy_ota_storage_write_info_t *chunk_info );

/**
 * @brief Close the underlying receive file in the specified OTA context.
 *
 * @note ota-update library expects user to implement this callback function.
 *
 * @param[in]   storage_ptr     Pointer to the OTA Agent storage context @ref cy_ota_storage_context_t
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_CLOSE_STORAGE
 */
typedef cy_rslt_t ( * cy_ota_file_close ) ( cy_ota_storage_context_t *storage_ptr );

/**
 * @brief Authenticate received OTA update file specified in the OTA context.
 *
 * @note ota-update library expects user to implement this callback function.
 *
 * @note Authentication method is user choice.
 *
 * @param[in]   storage_ptr     Pointer to the OTA Agent storage context @ref cy_ota_storage_context_t
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_VERIFY
 */
typedef cy_rslt_t ( * cy_ota_file_verify ) ( cy_ota_storage_context_t *storage_ptr );

/**
 * @brief Marks the image in the secondary/inactive slot as pending.
 * On the next reboot, the bootloader expected to perform a one-time boot of the the secondary slot image.
 *
 * @note ota-update library expects user to implement this callback function.
 *
 * @param[in]   storage_ptr     Pointer to the OTA Agent storage context @ref cy_ota_storage_context_t
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_VERIFY
 */
typedef cy_rslt_t ( * cy_ota_file_set_pending ) ( cy_ota_storage_context_t *storage_ptr );

/**
 * @brief The application has validated the new OTA image.
 *
 * @note This call must be after reboot and updated image is booted.
 *
 * @note  Calling this API makes currently executing image as boot image until next update.
 *
 * @param[in]   app_id          Application ID.
 *
 * @return CY_RSLT_SUCCESS on success; error codes otherwise.
 *
 */
typedef cy_rslt_t ( * cy_ota_file_validate ) ( uint16_t app_id );

/**
 * @brief Get Application info like Application version and Application ID.
 *
 * @note ota-update library expects user to implement this callback function.
 * @note This API implementation is optional for MCUBootloader based OTA.
 *
 * @param[in]        void*                  Pointer to file descriptor. It can be cast as flash_area or FILE as needed.
 * @param[in, out]   cy_ota_app_info_t*     Pointer to the OTA Application information context @ref cy_ota_app_info_t
 *
 * @return           CY_RSLT_SUCCESS
 *                   CY_RSLT_OTA_ERROR_GENERAL
 */
typedef cy_rslt_t ( * cy_ota_file_get_app_info ) ( uint16_t slot_id, uint16_t image_num, cy_ota_app_info_t *app_info );

/** \} group_ota_callback */

/**
 * \addtogroup group_ota_structures
 * \{
 */
/**
 * @brief OTA Connection parameters structure.
 *
 *  This information is used for MQTT and HTTP connections.
 *  \struct cy_ota_network_params_t
 */
typedef struct
{
    cy_ota_connection_t         initial_connection; /**< Initial connection type @ref cy_ota_connection_t.   */

    /* MQTT and HTTP  settings */
#ifdef COMPONENT_OTA_MQTT
    cy_ota_mqtt_params_t        mqtt;               /**< MQTT connection information.                        */
#endif
#ifdef COMPONENT_OTA_HTTP
    cy_ota_http_params_t        http;               /**< HTTP connection information.                        */
#endif
    cy_ota_update_flow_t        use_get_job_flow;   /**< Job flow (CY_OTA_JOB_FLOW or CY_OTA_DIRECT_FLOW).   */
} cy_ota_network_params_t;

/**
 * @brief OTA Agent parameters structure.
 *
 * These parameters are for describing some aspects of the OTA Agent.
 * \struct cy_ota_agent_params_t
 */
typedef struct cy_ota_agent_params_s
{
    uint8_t     reboot_upon_completion;     /**< 1 = Automatically reboot upon download completion and verify.  */
    uint8_t     validate_after_reboot;      /**< 0 = OTA will set MCUboot to permanent before reboot.
                                             *   1 = Application must call cy_ota_validated() after reboot.     */
    bool        do_not_send_result;         /**< If true, do not send the result status.                        */

    cy_ota_callback_t   cb_func;            /**< Notification callback function.                                */
    void                *cb_arg;            /**< Opaque argument passed to the notification callback function.  */
} cy_ota_agent_params_t;

/**
 * @brief OTA storage interface callback APIs structure.
 *
 *  This information is used for calling storage APIs.
 *  \struct cy_ota_storage_interface_t
 */
typedef struct cy_ota_storage_interface_s
{
    cy_ota_file_open           ota_file_open;             /**< To open OTA application image file.          */
    cy_ota_file_read           ota_file_read;             /**< To read OTA application image from storage.  */
    cy_ota_file_write          ota_file_write;            /**< To write OTA application image to storage.   */
    cy_ota_file_close          ota_file_close;            /**< To close OTA application image file.         */
    cy_ota_file_verify         ota_file_verify;           /**< To verify downloaded OTA application image.  */
    cy_ota_file_set_pending    ota_file_set_boot_pending; /**< To Mark the image in the inactive slot as pending. */
    cy_ota_file_validate       ota_file_validate;         /**< To validate current application.             */
    cy_ota_file_get_app_info   ota_file_get_app_info;     /**< To get application information.              */
} cy_ota_storage_interface_t;

/** \} group_ota_structures */


/***********************************************************************
 *
 * Functions
 *
 **********************************************************************/

/**
 * \addtogroup group_ota_functions
 * \{
 * OTA functions for starting, stopping, and getting information about the OTA Agent.
 */

/**
 * @brief Start the OTA Background Agent.
 *
 * Start a thread to monitor for OTA updates.
 * This consumes resources until cy_ota_agent_stop() is called.
 *
 * @param[in]   network_params     Pointer to cy_ota_network_params_t.
 * @param[in]   agent_params       Pointer to cy_ota_agent_params_t.
 * @param[in]   storage_interface  Pointer to cy_ota_storage_interface_t.
 * @param[out]  ctx_ptr            Handle to store the OTA Agent context structure pointer,
 *                                 Which is used for other OTA calls.
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BADARG
 *              CY_RSLT_OTA_ERROR_ALREADY_STARTED
 *              CY_RSLT_OTA_ERROR
 */
cy_rslt_t cy_ota_agent_start(cy_ota_network_params_t *network_params,
                             cy_ota_agent_params_t *agent_params,
                             cy_ota_storage_interface_t *storage_interface,
                             cy_ota_context_ptr *ctx_ptr);

/**
 * @brief Stop OTA Background Agent.
 *
 *  Stop the thread to monitor OTA updates and release resources.
 *
 * @param[in]   ctx_ptr         Pointer to the OTA Agent context storage returned from @ref cy_ota_agent_start();
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BADARG
 */
cy_rslt_t cy_ota_agent_stop(cy_ota_context_ptr *ctx_ptr);

/**
 * @brief Check for OTA update availability and download update now.
 *
 * Once cy_ota_start() has returned CY_RSLT_SUCCESS, you can initiate an OTA update availability check
 * whenever you require; this supercedes the timing values. It does not change the timer values, and will
 * set the timer to the current values after an OTA download cycle.
 *
 *
 * @param[in]   ctx_ptr         Pointer to the OTA Agent context storage returned from @ref cy_ota_agent_start();
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BADARG
 *              CY_RSLT_OTA_ERROR_ALREADY_STARTED
 */
cy_rslt_t cy_ota_get_update_now(cy_ota_context_ptr ctx_ptr);

/**
 * @brief Set the OTA log output level.
 *
 * To get the OTA log output, first call cy_log_init().
 * Call this routine at any time to set the OTA logging level.
 *
 * @param[in]  level    Required logging level from OTA logs.
 *
 * @result     N/A
 */
void cy_ota_set_log_level(CY_LOG_LEVEL_T level);

/**
 * @brief Get the OTA Agent State.
 *
 * Use this function to determine what state the OTA Agent is in. Do not check if the
 * OTA Agent is in CY_OTA_STATE_AGENT_WAITING because it will interrupt an OTA cycle.
 *
 * @param[in]  ota_ptr          Pointer to the OTA Agent context returned from @ref cy_ota_agent_start();
 * @param[out] ota_state        Current OTA State.
 *
 * @result     CY_RSLT_SUCCESS     sets @ref cy_ota_agent_state_t
 *             CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_get_state(cy_ota_context_ptr ota_ptr, cy_ota_agent_state_t *ota_state);

/**
 * @brief Get the last OTA error.
 *
 * The last error value is persistent, and can be queried after @ref cy_ota_agent_stop().
 *
 * @result the last error that occurred (see cy_rslt_t).
 */
cy_rslt_t cy_ota_get_last_error(void);

/**
 * @brief Get a string representation of the error.
 *
 * This can be called at any time.
 *
 * @param[in]   error      Value returned from cy_ota_get_last_error().
 *
 * @result Pointer to a string (NULL for unknown error).
 */
const char *cy_ota_get_error_string(cy_rslt_t error);

/**
 * @brief Get a string Representation of the OTA state.
 *
 * This can be called at any time.
 *
 * @param[in]   state_value      @ref cy_ota_agent_state_t
 *
 * @result  Pointer to the string (NULL for unknown state).
 */
const char *cy_ota_get_state_string(cy_ota_agent_state_t state_value);

/**
 * @brief Get a string representation for the callback reason.
 *
 * This can be called at any time.
 *
 * @param[in]   reason      @ref cy_ota_cb_reason_t
 *
 * @result  Pointer to the string (NULL for unknown reason).
 */
const char *cy_ota_get_callback_reason_string(cy_ota_cb_reason_t reason);

/**
 * @brief Prepare for OTA Download
 *
 * @param[in]   ctx_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_STORAGE
 */
cy_rslt_t cy_ota_update_prepare(cy_ota_context_ptr ctx_ptr);

/**
 * @brief OTA Download starting
 *
 * @param[in]   ctx_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   update_file_size        OTA update file size.
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_STORAGE
 */
cy_rslt_t cy_ota_update_download_start(cy_ota_context_ptr ctx_ptr, uint32_t update_file_size);

/**
 * @brief OTA data write
 *
 * @param[in]   ctx_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   data_buf                Pointer to data buffer.
 * @param[in]   len                     Data length.
 * @param[in]   offset                  Data buffer offset.
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_STORAGE
 */
cy_rslt_t cy_ota_update_write(cy_ota_context_ptr ctx_ptr, uint8_t *data_buf, uint16_t len, uint16_t offset);

/**
 * @brief OTA Verify download
 *
 * @param[in]   ctx_ptr                   Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   update_file_crc           32 Bit CRC sent from the Host.
 * @param[in]   verify_crc_or_signature   OTA library need to verify CRC.
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_VERIFY
 */
cy_rslt_t cy_ota_update_verify(cy_ota_context_ptr ctx_ptr, uint32_t update_file_crc, bool verify_crc_or_signature);

/**
 * @brief OTA Set boot pending
 *
 * @param[in]   ctx_ptr                   Pointer to OTA agent context @ref cy_ota_context_ptr
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_STORAGE
 */
cy_rslt_t cy_ota_update_image_set_pending(cy_ota_context_ptr ctx_ptr);

/**
 * @brief Abort OTA download
 *
 * @param[in]   ctx_ptr     Pointer to OTA agent context @ref cy_ota_context_ptr
 *
 * @return      CY_RSLT_SUCCESS
 */
cy_rslt_t cy_ota_update_abort(cy_ota_context_ptr ctx_ptr);

#if defined(COMPONENT_OTA_BLUETOOTH) || defined(CY_DOXYGEN)
/**
 * @brief Prepare for OTA Download
 *
 * @param[in]   ctx_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_STORAGE
 */
#define cy_ota_ble_download_prepare    cy_ota_update_prepare

/**
 * @brief OTA Download starting
 *
 * @param[in]   ctx_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   update_file_size        OTA update file size.
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_STORAGE
 */
#define cy_ota_ble_download            cy_ota_update_download_start

/**
 * @brief OTA data write
 *
 * @param[in]   ctx_ptr                 Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   data_buf                Pointer to data buffer.
 * @param[in]   len                     Data length.
 * @param[in]   offset                  Data buffer offset.
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_STORAGE
 */
#define cy_ota_ble_download_write      cy_ota_update_write

/**
 * @brief OTA Verify download
 *
 * @param[in]   ctx_ptr                   Pointer to OTA agent context @ref cy_ota_context_ptr
 * @param[in]   update_file_crc           32 Bit CRC sent from the Host.
 * @param[in]   verify_crc_or_signature   OTA library need to verify CRC.
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BLE_VERIFY
 */
#define cy_ota_ble_download_verify     cy_ota_update_verify

/**
 * @brief Abort OTA download
 *
 * @param[in]   ctx_ptr     Pointer to OTA agent context @ref cy_ota_context_ptr
 *
 * @return      CY_RSLT_SUCCESS
 */
#define cy_ota_ble_download_abort      cy_ota_update_abort
#endif  /* defined(COMPONENT_OTA_BLUETOOTH) || defined(CY_DOXYGEN) */

/** \} group_ota_functions */

#ifdef __cplusplus
    }
#endif

#endif /* CY_OTA_API_H__ */

 /** \} group_cy_ota */
