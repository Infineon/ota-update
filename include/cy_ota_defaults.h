/*
 * Copyright 2023, Cypress Semiconductor Corporation (an Infineon company)
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file
 *
 * Cypress OTA API default values.
 *
 *  Default values for the OTA library
 *
 *  You can override these defines using the cy_ota_config.h file.
 *  Copy from configs/cy_ota_config.h to the root directory of your build.
 *
 **********************************************************************/

/**
 * \addtogroup group_cy_ota
 * \{
 */

#ifndef CY_OTA_DEFAULTS_H__
#define CY_OTA_DEFAULTS_H__ 1


#ifdef __cplusplus
extern "C" {
#endif

#include "cy_result.h"

/**
 * \addtogroup group_ota_typedefs
 * \{
 */

/**
 * @brief Initial OTA check.
 *
 * Time from when cy_ota_start() is called to when the OTA Agent first checks for an update.
 * You can override this define in cy_ota_config.h.
 *
 * Minimum value is  CY_OTA_INTERVAL_SECS_MIN.
 * Maximum value is  CY_OTA_INTERVAL_SECS_MAX.
 */
#ifndef CY_OTA_INITIAL_CHECK_SECS
#define CY_OTA_INITIAL_CHECK_SECS           (60)           /* 60 seconds. */
#endif

/**
 * @brief Next OTA check.
 *
 * Time from when after the OTA Agent completes or fails an update until the next check.
 * You can override this define in cy_ota_config.h.
 *
 * Minimum value is  CY_OTA_INTERVAL_SECS_MIN.
 * Maximum value is  CY_OTA_INTERVAL_SECS_MAX.
 */
#ifndef CY_OTA_NEXT_CHECK_INTERVAL_SECS
#define CY_OTA_NEXT_CHECK_INTERVAL_SECS     (60 * 60 * 24) /* Once per day. */
#endif

/**
 * @brief Retry interval.
 *
 * Time between failures for connection before you can retry.
 * You can override this define in cy_ota_config.h.
 * Minimum value is  CY_OTA_INTERVAL_SECS_MIN.
 * Maximum value is  CY_OTA_INTERVAL_SECS_MAX.
 */
#ifndef CY_OTA_RETRY_INTERVAL_SECS
#define CY_OTA_RETRY_INTERVAL_SECS          (5)            /* 5 seconds. */
#endif

/**
 * @brief Length of time to check for downloads.
 *
 * The OTA Agent wakes up, connects to server, and waits this much time before disconnecting.
 * This allows the OTA Agent to be inactive for long periods of time, only checking for short periods.
 * Use 0x00 to continue checking once started.
 */
#ifndef CY_OTA_CHECK_TIME_SECS
#define CY_OTA_CHECK_TIME_SECS              (10 * 60)       /* 10 minutes. */
#endif

/**
* @brief Expected maximum download time between each OTA packet arrival.
*
* This is used check that the download occurs in a reasonable time frame.
*/
#ifndef CY_OTA_PACKET_INTERVAL_SECS
#define CY_OTA_PACKET_INTERVAL_SECS         (60)           /* 1 minute. */
#endif

/**
 * @brief Length of time to check for getting the Job document.
 *
 * The OTA Agent wakes up, connects to broker/server, and waits this much time before disconnecting.
 * This allows the OTA Agent to be inactive for long periods of time, only checking for short periods.
 * Use 0x00 to continue checking once started.
 */
#ifndef CY_OTA_JOB_CHECK_TIME_SECS
#define CY_OTA_JOB_CHECK_TIME_SECS           (30)          /* 30 seconds. */
#endif

/**
 * @brief Length of time to check for getting the OTA image data.
 *
 * After getting the Job (or during a Direct download), this is the amount of time to wait before
 * canceling the download.
 * Use 0x00 to disable.
 */
#ifndef CY_OTA_DATA_CHECK_TIME_SECS
#define CY_OTA_DATA_CHECK_TIME_SECS          (20 * 60)      /* 20 minutes. */
#endif

/**
 * @brief Length of time to check for OTA image downloads.
 *
 * The OTA Agent wakes up, connects to server, and waits this much time before disconnecting.
 * This allows the OTA Agent to be inactive for long periods of time, only checking for short periods.
 * Use 0x00 to continue checking once started.
 */
#ifndef CY_OTA_CHECK_TIME_SECS
#define CY_OTA_CHECK_TIME_SECS                (60 * 10)    /* 10 minutes. */
#endif

/**
 * @brief Number of OTA session retries.
 *
 * Retry count for overall OTA session attempts.
 */
#ifndef CY_OTA_RETRIES
#define CY_OTA_RETRIES                          (5)        /* 5 overall OTA retries. */
#endif

/**
 * @brief Number of OTA connect to server/Broker retries.
 *
 * Retry count for connection attempts.
 */
#ifndef CY_OTA_CONNECT_RETRIES
#define CY_OTA_CONNECT_RETRIES                  (3)        /* 3 server connect retries.  */
#endif

/**
 * @brief Number of OTA download retries.
 *
 * Retry count for attempts at downloading the OTA image.
 */
#ifndef CY_OTA_MAX_DOWNLOAD_TRIES
#define CY_OTA_MAX_DOWNLOAD_TRIES               (3)         /* 3 download OTA image retries. */
#endif

/**
 * @brief HTTP timeout for sending messages
 *
 */
#ifndef CY_OTA_HTTP_TIMEOUT_SEND
#define CY_OTA_HTTP_TIMEOUT_SEND                (3000)         /* 3 second send timeout. */
#endif


/**
 * @brief HTTP timeout for receiving messages
 *
 */
#ifndef CY_OTA_HTTP_TIMEOUT_RECEIVE
#define CY_OTA_HTTP_TIMEOUT_RECEIVE             (3000)         /* 3 second receive timeout. */
#endif

/**********************************************************************
 * Message Defines
 **********************************************************************/


/**
 * @brief Last part of the topic to subscribe.
 *
 * Topic for the device to send a message to the Publisher:
 *  "COMPANY_TOPIC_PREPEND / BOARD_NAME / PUBLISHER_LISTEN_TOPIC"
 *  The combined topic must match the Publisher's subscribe topic.
 *
 * Override in cy_ota_config.h.
 */
#ifndef PUBLISHER_LISTEN_TOPIC
#define PUBLISHER_LISTEN_TOPIC              "publish_notify"
#endif

/**
 * @brief First part of the topic to subscribe/publish.
 *
 * Topic for the device to send a message to the Publisher:
 *  "COMPANY_TOPIC_PREPEND / BOARD_NAME / PUBLISHER_LISTEN_TOPIC"
 *
 * Override in cy_ota_config.h.
 */
#ifndef COMPANY_TOPIC_PREPEND
#define COMPANY_TOPIC_PREPEND               "OTAUpdate"
#endif

/**
 * @brief End of topic to send a message to the Publisher for Direct download.
 *
 * Override in cy_ota_config.h.
 */
#ifndef PUBLISHER_DIRECT_TOPIC
#define PUBLISHER_DIRECT_TOPIC               "OTAImage"
#endif

/**
 * @brief "Update successful" message.
 *
 * Used with sprintf() to create a RESULT message to the Broker/server.
 */
#ifndef CY_OTA_RESULT_SUCCESS
#define CY_OTA_RESULT_SUCCESS               "Success"
#endif

/**
* @brief "Update failed" message.
*
* Used with sprintf() to create a RESULT message to the Broker/server.
*/
#ifndef CY_OTA_RESULT_FAILURE
#define CY_OTA_RESULT_FAILURE               "Failure"
#endif


/**
 * @brief Default Job document name.
 *
 * Name of the update JSON file for HTTP.
 */
#ifndef CY_OTA_HTTP_JOB_FILE
#define CY_OTA_HTTP_JOB_FILE               "/ota_update.json"
#endif

/**
 * @brief Default OTA image file name.
 *
 * Name of the OTA image for HTTP.
 */
#ifndef CY_OTA_HTTP_DATA_FILE
#define CY_OTA_HTTP_DATA_FILE              "/ota-update.bin"
#endif

/**
 * @brief Device message to the Publisher to ask about updates.
 * Used with sprintf() to insert the current version and UniqueTopicName at runtime.
 * Override if required by defining in cy_ota_config.h.
 */
#ifndef CY_OTA_SUBSCRIBE_UPDATES_AVAIL
#define CY_OTA_SUBSCRIBE_UPDATES_AVAIL \
"{\
\"Message\":\"Update Availability\", \
\"Manufacturer\": \"Express Widgits Corporation\", \
\"ManufacturerID\": \"EWCO\", \
\"ProductID\": \"Easy Widgit\", \
\"SerialNumber\": \"ABC213450001\", \
\"BoardName\": \"CY8CPROTO_062_4343W\", \
\"Version\": \"%d.%d.%d\", \
\"UniqueTopicName\": \"%s\"\
}"
#endif

/**
 * @brief Device message to the Publisher to ask for a full download with one request.
 * *
 * Used with sprintf() to insert the current version and UniqueTopicName at runtime.
 * Override if required by defining in cy_ota_config.h.
 */
#ifndef CY_OTA_DOWNLOAD_REQUEST
#define CY_OTA_DOWNLOAD_REQUEST \
"{\
\"Message\":\"Request Update\", \
\"Manufacturer\": \"Express Widgits Corporation\", \
\"ManufacturerID\": \"EWCO\", \
\"ProductID\": \"Easy Widgit\", \
\"SerialNumber\": \"ABC213450001\", \
\"BoardName\": \"CY8CPROTO_062_4343W\", \
\"Version\": \"%d.%d.%d\", \
\"UniqueTopicName\": \"%s\"\
}"
#endif

/**
 * @brief Device message to the Publisher to ask for a chunk of data at a time.
 * *
 * Used with sprintf() to insert the current version and UniqueTopicName at runtime.
 * Override if required by defining in cy_ota_config.h.
 */
#ifndef CY_OTA_DOWNLOAD_CHUNK_REQUEST
#define CY_OTA_DOWNLOAD_CHUNK_REQUEST \
"{\
\"Message\":\"Request Data Chunk\", \
\"Manufacturer\": \"Express Widgits Corporation\", \
\"ManufacturerID\": \"EWCO\", \
\"ProductID\": \"Easy Widgit\", \
\"SerialNumber\": \"ABC213450001\", \
\"BoardName\": \"CY8CPROTO_062_4343W\", \
\"Version\": \"%d.%d.%d\", \
\"UniqueTopicName\": \"%s\", \
\"Filename\": \"%s\", \
\"Offset\": \"%ld\", \
\"Size\": \"%ld\"\
}"
#endif

/**
 * @brief Device JSON document to respond to the MQTT Publisher.
 *
 * Used with sprintf() to create the JSON message.
 * Override if required by defining in cy_ota_config.h.
 */
#ifndef CY_OTA_MQTT_RESULT_JSON
#define CY_OTA_MQTT_RESULT_JSON \
"{\
\"Message\":\"%s\", \
\"UniqueTopicName\": \"%s\"\
}"
#endif

/**
 * @brief Device JSON document to respond to the HTTP server.
 *
 * Used with sprintf() to create the JSON message.
 * Override if required by defining in cy_ota_config.h.
 */
#ifndef CY_OTA_HTTP_RESULT_JSON
#define CY_OTA_HTTP_RESULT_JSON \
"{\
\"Message\":\"%s\", \
\"File\":\"%s\" \
}"
#endif

/**
 * @brief HTTP GET template.
 *
 * Used with sprintf() to create the GET request for the HTTP server.
 * Override if required by defining in cy_ota_config.h.
 */
#ifndef CY_OTA_HTTP_GET_TEMPLATE
#define CY_OTA_HTTP_GET_TEMPLATE \
    "GET %s HTTP/1.1\r\n" \
    "Host: %s:%d \r\n" \
    "\r\n"
#endif

/**
 * @brief HTTP GET Range template.
 *
 * Used with sprintf() to create the GET request for the HTTP server
 * when requesting a range of data.
 */
#ifndef CY_OTA_HTTP_GET_RANGE_TEMPLATE
#define CY_OTA_HTTP_GET_RANGE_TEMPLATE \
    "GET %s HTTP/1.1\r\n" \
    "Host: %s:%d \r\n" \
    "Range: bytes=%ld-%ld \r\n" \
    "\r\n"
#endif

/**
 * @brief HTTP POST template.
 *
 * Used with sprintf() to create the POST message for the HTTP server.
 * Override if required by defining in cy_ota_config.h.
 */
#ifndef CY_OTA_HTTP_POST_TEMPLATE
#define CY_OTA_HTTP_POST_TEMPLATE \
    "POST %s HTTP/1.1\r\n" \
    "Content-Length:%ld \r\n" \
    "\r\n%s"
#endif

/***********************************************************************
 *
 * MQTT Specific defines
 *
 **********************************************************************/

/**
 * @brief Keepalive interval for MQTT.
 * @brief Maximum number of MQTT topics.
 *
 * An MQTT ping request will be sent periodically at this interval.
 * The maximum number of topics for subscribing.
 */
#ifndef CY_OTA_MQTT_KEEP_ALIVE_SECONDS
#define CY_OTA_MQTT_KEEP_ALIVE_SECONDS      (60)                /* 60 second keepalive. */
#endif

/**
 * @brief Maximum number of MQTT topics.
 *
 * The maximum number of topics for subscribing.
 */
#ifndef CY_OTA_MQTT_MAX_TOPICS
#define CY_OTA_MQTT_MAX_TOPICS                  (2)
#endif

/**
 * @brief Topic prefix.
 *
 * Used as the prefix for "Will" and "Acknowledgement" messages.
 */
#ifndef CY_OTA_MQTT_TOPIC_PREFIX
#define CY_OTA_MQTT_TOPIC_PREFIX                   "cy_ota_device"
#endif

/**
 * @brief The first characters in the client identifier.
 *
 * A timestamp is appended to this prefix to create a unique
 *   client identifer for each connection.
 */
#ifndef CY_OTA_MQTT_CLIENT_ID_PREFIX
#define CY_OTA_MQTT_CLIENT_ID_PREFIX                "cy_device"
#endif

/** \} group_ota_typedefs */

#ifdef __cplusplus
    }
#endif

#endif /* CY_OTA_DEFAULTS_H__ */

/** \} group_cy_ota */
