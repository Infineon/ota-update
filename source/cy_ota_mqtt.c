/*
 * Copyright 2022, Cypress Semiconductor Corporation (an Infineon company)
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

/*
 *  Cypress OTA Agent network abstraction for MQTT
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "cy_ota_api.h"

#ifdef COMPONENT_OTA_MQTT

#include "cy_ota_internal.h"
#include "cyabs_rtos.h"

#include "cy_mqtt_api.h"
#include "cy_json_parser.h"
#include "cy_log.h"


#define IOT_PUBLISH_RETRY_LIMIT                        (10)
#define IOT_PUBLISH_RETRY_MS                           (1000)
#define IOT_TOPIC_FILTER_COUNT                         (1)

/* Publish 1 MQTT request, publisher chunks and sends all data
 * Enabled is current version.
 *
 * Comment out to have Device request each chunk separately.
 * */
#define CY_MQTT_GET_ALL_DATA_WITH_ONE_CALL

/***********************************************************************
 *
 * defines & enums
 *
 **********************************************************************/

#define CY_OTA_MQTT_SUBSCRIBE           0
#define CY_OTA_MQTT_UNSUBSCRIBE	        1

/* For debugging */
//#define PRINT_MQTT_DATA         1
//#define DEBUG_PACKET_RECEIPT_TIME_DIFF   1

/**
 * @brief The longest client identifier that an MQTT server must accept (as defined
 * by the MQTT 3.1.1 spec) is 23 characters. Add 1 to include the length of the NULL
 * terminator.
 */
#define CLIENT_IDENTIFIER_MAX_LENGTH                (24)

/**
 * @brief Number of decimal digits for a 16 bit number (+1 for '\0')
 * Used to size the Client ID string to be able to add a unique number to it.
 */
#define UINT16_DECIMAL_LENGTH                       (6)

/**
 * @brief The Last Will and Testament topic name in this demo.
 *
 * The MQTT server will publish a message to this topic name if this client is
 * unexpectedly disconnected.
 */
#define WILL_TOPIC_NAME                             CY_OTA_MQTT_TOPIC_PREFIX "/will"

/**
 * @brief The length of #WILL_TOPIC_NAME.
 */
#define WILL_TOPIC_NAME_LENGTH                      ( (uint16_t)(sizeof(WILL_TOPIC_NAME) - 1) )

/**
 * @brief The message to publish to #WILL_TOPIC_NAME.
 */
#define WILL_MESSAGE                                "MQTT demo unexpectedly disconnected."

/**
 * @brief The length of #WILL_MESSAGE.
 */
#define WILL_MESSAGE_LENGTH                         ( ( size_t ) (sizeof(WILL_MESSAGE) - 1) )

/**
 * @brief The maximum number of times each PUBLISH in this demo will be retried.
 */
#define PUBLISH_RETRY_LIMIT                         (10)

/**
 * @brief A PUBLISH message is retried if no response is received within this
 * time.
 */
#define PUBLISH_RETRY_MS                            (1000)

/**
 * @brief The topic name on which acknowledgement messages for incoming publishes
 * should be published.
 */
#define ACKNOWLEDGEMENT_TOPIC_NAME                  CY_OTA_MQTT_TOPIC_PREFIX "/acknowledge"

/**
 * @brief The length of #ACKNOWLEDGEMENT_TOPIC_NAME.
 */
#define ACKNOWLEDGEMENT_TOPIC_NAME_LENGTH           ( ( uint16_t ) (sizeof(ACKNOWLEDGEMENT_TOPIC_NAME) - 1) )

/**
 * @brief Format string of PUBLISH acknowledgement messages in this demo.
 */
#define ACKNOWLEDGEMENT_MESSAGE_FORMAT              "Client has received PUBLISH %.*s from server."

/**
 * @brief Size of the buffers that hold acknowledgement messages in this demo.
 */
#define ACKNOWLEDGEMENT_MESSAGE_BUFFER_LENGTH       (sizeof(ACKNOWLEDGEMENT_MESSAGE_FORMAT) + 2)

/***********************************************************************
 *
 * Macros
 *
 **********************************************************************/

/***********************************************************************
 *
 * Structures
 *
 **********************************************************************/

/**
 * @brief MQTT Payload type
 */
typedef enum {
    CY_OTA_MQTT_HEADER_TYPE_ONE_FILE = 0    /**< OTA Image with single file (main application)  */
} cy_ota_mqtt_header_ota_type_t;

/**
 * @brief MQTT Payload header (not the MQTT header, this is our data header)
 */
#pragma pack(push,1)
typedef struct cy_ota_mqtt_chunk_payload_header_s {
    const uint8_t   magic[sizeof(CY_OTA_MQTT_MAGIC) - 1];       /**< "OTAImage" @ref CY_OTA_MQTT_MAGIC                  */
    const uint16_t  offset_to_data;                             /**< offset within this payload to start of data        */
    const uint16_t  ota_image_type;                             /**< 0 = single application OTA Image  @ref cy_ota_mqtt_header_ota_type_t */
    const uint16_t  update_version_major;                       /**< OTAImage Major version number                      */
    const uint16_t  update_version_minor;                       /**< OTAImage minor version number                      */
    const uint16_t  update_version_build;                       /**< OTAImage build version number                      */
    const uint32_t  total_size;                                 /* total size of OTA Image (all chunk data concatenated)*/
    const uint32_t  image_offset;                               /* offset within the final OTA Image of THIS chunk data */
    const uint16_t  data_size;                                  /* Size of chunk data in THIS payload                   */
    const uint16_t  total_num_payloads;                         /* Total number of payloads                             */
    const uint16_t  this_payload_index;                         /* THIS payload index                                   */
} cy_ota_mqtt_chunk_payload_header_t;
#pragma pack(pop)


/***********************************************************************
 *
 * Variables
 *
 **********************************************************************/

/***********************************************************************
 *
 * Function declarations
 *
 **********************************************************************/
static cy_rslt_t cy_ota_modify_subscriptions(cy_ota_context_t *ctx,
                                             uint8_t operation,
                                             uint8_t numTopics,
                                             const char ** pTopicFilters);

/***********************************************************************
 *
 * Functions
 *
 **********************************************************************/

static void cy_ota_print_connect_info(const cy_mqtt_connect_info_t *connection)
{
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "cy_mqtt_connect_info_t:\n");
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "   ID       : %.*s\n", connection->client_id_len, connection->client_id);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "   user     : %.*s\n", connection->username_len, (connection->username == NULL) ? "" : connection->username);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "   pass     : %.*s\n", connection->password_len, (connection->password == NULL) ? "" : connection->password);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "   clean    : %d\n", connection->clean_session);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "   WILL     : %p\n", connection->will_info);
    if (connection->will_info != NULL)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "          topic : %.*s\n", connection->will_info->topic_len, (connection->will_info->topic == NULL) ? "" : connection->will_info->topic);
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "          qos   : %d\n", connection->will_info->qos);
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "         retain : %ld\n", connection->will_info->retain);
    }
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "keep alive  : %d\n", connection->keep_alive_sec);

#ifndef DISABLE_MBEDTLS_ACCELERATION
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "using accel : YES\n");
#else
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "using accel : NO\n");
#endif
}

static void cy_ota_print_broker_info(const cy_mqtt_broker_info_t *broker)
{
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "cy_mqtt_broker_info_t:\n");
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "             server : %.*s\n", broker->hostname_len, (broker->hostname == NULL) ? "" : broker->hostname);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "             port   : %d\n", broker->port);
}

static void cy_ota_mqtt_timer_callback(cy_timer_callback_arg_t arg)
{
    cy_ota_context_t *ctx = (cy_ota_context_t *)arg;
    CY_OTA_CONTEXT_ASSERT(ctx);

    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() new event:%d\n", __func__, ctx->mqtt.mqtt_timer_event);
    /* yes, we set the ota_event as the mqtt get() function uses the same event var */
    cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)ctx->mqtt.mqtt_timer_event, 0);
}

static cy_rslt_t cy_ota_stop_mqtt_timer(cy_ota_context_t *ctx)
{
    CY_OTA_CONTEXT_ASSERT(ctx);
    return cy_rtos_stop_timer(&ctx->mqtt.mqtt_timer);
}

static cy_rslt_t cy_ota_start_mqtt_timer(cy_ota_context_t *ctx, uint32_t secs, ota_events_t event)
{
    cy_rslt_t result;
    uint32_t    num_ms = SECS_TO_MILLISECS(secs);

    CY_OTA_CONTEXT_ASSERT(ctx);

    cy_ota_stop_mqtt_timer(ctx);
    ctx->mqtt.mqtt_timer_event = event;
    result = cy_rtos_start_timer(&ctx->mqtt.mqtt_timer, num_ms);
    return result;
}

static cy_rslt_t cy_ota_mqtt_create_json_request(cy_ota_context_t *ctx,
                                        const char *message_doc,
                                        const char *filename,
                                        uint32_t offset,
                                        uint32_t size)
{
    /* create json doc for get job */
    cy_rslt_t   result = CY_RSLT_SUCCESS;
    uint32_t    needed_size;

    (void)memset(ctx->mqtt.json_doc, 0x00, sizeof(ctx->mqtt.json_doc) );

    (void)filename;		/* Coverity fix */
    (void)offset;		/* Coverity fix */
    (void)size;			/* Coverity fix */

#ifdef CY_MQTT_GET_ALL_DATA_WITH_ONE_CALL
    /* Current default. Send one request for the entire file,
     * Publisher.py will chunk and send separate chunks.
     */
    needed_size = snprintf(NULL, 0, message_doc, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD, ctx->mqtt.unique_topic);
    if (needed_size > (sizeof(ctx->mqtt.json_doc)-1) )
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() Need to increase size of job_doc from CY_OTA_JSON_DOC_BUFF_SIZE (%ld) to at least (%ld)\n", __func__, CY_OTA_JSON_DOC_BUFF_SIZE, needed_size);
        return CY_RSLT_OTA_ERROR_OUT_OF_MEMORY;
    }
    sprintf(ctx->mqtt.json_doc, message_doc, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD, ctx->mqtt.unique_topic);
#else
    /* This code is for requesting each chunk separately. */
    needed_size = snprintf(NULL, 0, message_doc, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD, ctx->mqtt.unique_topic,
            filename, offset, size);
    if (needed_size > (sizeof(ctx->mqtt.json_doc)-1) )
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() Need to increase size of job_doc from CY_OTA_JSON_DOC_BUFF_SIZE (%ld) to at least (%ld)\n", __func__, CY_OTA_JSON_DOC_BUFF_SIZE, needed_size);
        return CY_RSLT_OTA_ERROR_OUT_OF_MEMORY;
    }
    sprintf(ctx->mqtt.json_doc, message_doc, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD, ctx->mqtt.unique_topic,
            filename, offset, size);
#endif

    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() Messg: %s\n", __func__, ctx->mqtt.json_doc);

    return result;
}

static cy_rslt_t cy_ota_mqtt_publish_request(cy_ota_context_t *ctx, char *mqtt_topic, char *mqtt_message)
{
    /*Publish for the topic */
    cy_rslt_t   result = CY_RSLT_SUCCESS;

    CY_OTA_CONTEXT_ASSERT(ctx);

    cy_mqtt_publish_info_t  pub_msg;

    pub_msg.qos         = CY_MQTT_QOS1;
    pub_msg.retain      = false;            /* TODO: ???  Whether this is a retained message. */
    pub_msg.dup         = false;            /* TODO: ???  Whether this is a duplicate publish message. */
    pub_msg.topic       = mqtt_topic;
    pub_msg.topic_len   = strlen(pub_msg.topic);
    pub_msg.payload     = mqtt_message;
    pub_msg.payload_len = strlen(pub_msg.payload);

    /* Publish the message. */
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "Publish to %.*s:\n>%.*s<\n\n\n",
                pub_msg.topic_len, pub_msg.topic, pub_msg.payload_len, pub_msg.payload);

    result = cy_mqtt_publish( ctx->mqtt.mqtt_connection, &pub_msg );
    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "MQTT PUBLISH error.\n");
        return CY_RSLT_OTA_ERROR_MQTT_PUBLISH;
    }
    else
    {
         cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "PUBLISH completed successfully.\n");
    }

    return CY_RSLT_SUCCESS;
}

static cy_rslt_t cy_ota_subscribe_to_unique_topic(cy_ota_context_t *ctx)
{
    char        *unique_topic[1];
    cy_rslt_t   result = CY_RSLT_SUCCESS;

    CY_OTA_CONTEXT_ASSERT(ctx);

    if (ctx->mqtt.unique_topic_subscribed == false)
    {
        unique_topic[0] = ctx->mqtt.unique_topic;

        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() Unique Topic Subscribe %s\n", __func__, ctx->mqtt.unique_topic);
        result = cy_ota_modify_subscriptions(ctx,
                                             CY_OTA_MQTT_SUBSCRIBE,
                                             1,
                                             (const char **)unique_topic);

        if (result != CY_RSLT_SUCCESS)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() Unique Topic Subscribe failed\n", __func__);
            result = CY_RSLT_OTA_ERROR_GENERAL;
        }
        else
        {
            ctx->mqtt.unique_topic_subscribed = true;
        }
    }

    return result;
}

/*-----------------------------------------------------------*/
/**
 * @brief Called by MQTT callback - parse the data chunk header
 *
 * @param[in]   buffer      - payload from MQTT callback
 * @param[in]   length      - length of the buffer
 * @param[out]  chunk_info  - Filled for writing data to FLASH @ref cy_ota_storage_write_info_t
 *
 * @return      CY_RSLT_SUCCESS - use chunk_info to write to FLASH
 *              CY_RSLT_OTA_ERROR_GENERAL
 *              CY_RSLT_OTA_ERROR_BADARG
 *              CY_RSLT_OTA_ERROR_NOT_A_HEADER
 */
static cy_rslt_t cy_ota_mqtt_parse_chunk(const uint8_t *buffer, uint32_t length, cy_ota_storage_write_info_t *chunk_info)
{
    const cy_ota_mqtt_chunk_payload_header_t  *header = (cy_ota_mqtt_chunk_payload_header_t *)buffer;

    if ( (header == NULL) || (length == 0) || (chunk_info == NULL) )
    {
        return CY_RSLT_OTA_ERROR_BADARG;
    }

    /* start with clean slate */
    memset(chunk_info, 0x00, sizeof(cy_ota_storage_write_info_t) );

    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "Chunk length: %d \n", length);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "Magic: %c%c%c%c%c%c%c%c\n", header->magic[0], header->magic[1], header->magic[2], header->magic[3],
                                            header->magic[4], header->magic[5], header->magic[6], header->magic[7]);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "header->offset_to_data     off:%d : %d\n", offsetof(cy_ota_mqtt_chunk_payload_header_t,offset_to_data), header->offset_to_data);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "header->ota_image_type     off:%d : %d\n", offsetof(cy_ota_mqtt_chunk_payload_header_t,ota_image_type), header->ota_image_type);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "header->version                   : %d.%d.%d\n", header->update_version_major, header->update_version_minor, header->update_version_build);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "header->total_size         off:%d : %ld\n", offsetof(cy_ota_mqtt_chunk_payload_header_t,total_size), header->total_size);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "header->image_offset       off:%d : %ld\n", offsetof(cy_ota_mqtt_chunk_payload_header_t,image_offset), header->image_offset);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "header->data_size          off:%d : %d\n", offsetof(cy_ota_mqtt_chunk_payload_header_t,data_size), header->data_size);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "header->total_num_payloads off:%d : %d\n", offsetof(cy_ota_mqtt_chunk_payload_header_t,total_num_payloads), header->total_num_payloads);
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "header->this_payload_index off:%d : %d\n", offsetof(cy_ota_mqtt_chunk_payload_header_t,this_payload_index), header->this_payload_index);

    /* test for magic */
    if (memcmp(header->magic, (uint8_t *)CY_OTA_MQTT_MAGIC, (strlen(CY_OTA_MQTT_MAGIC) - 1) ) != 0 )
    {
        return CY_RSLT_OTA_ERROR_NOT_A_HEADER;
    }
    /* test for some other validity measures */
    if ( (header->offset_to_data > length) ||
         (header->ota_image_type != CY_OTA_MQTT_HEADER_TYPE_ONE_FILE) ||
         (header->data_size > header->total_size) ||
         (header->this_payload_index > header->total_num_payloads) )
    {
        return CY_RSLT_OTA_ERROR_NOT_A_HEADER;
    }

    /* validate version is higher than current application */
    if ( ( (uint32_t)(APP_VERSION_MAJOR + 1) >			   /* fix Coverity APP_VERSION_MAJOR == 0 */
               (uint32_t)(header->update_version_major + 1) ) ||
         ( (APP_VERSION_MAJOR == header->update_version_major) &&
           ( ( (uint32_t)(APP_VERSION_MINOR + 1) ) >    /* fix Coverity 238370 when APP_VERSION_MINOR == 0 */
               ( (uint32_t)(header->update_version_minor + 1) ) ) ) ||
         ( (APP_VERSION_MAJOR == header->update_version_major) &&
           (APP_VERSION_MINOR == header->update_version_minor) &&
           (APP_VERSION_BUILD >= header->update_version_build) ) )
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "Current Application version %d.%d.%d update %d.%d.%d. Fail.\n",
                    APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD,
                    header->update_version_major, header->update_version_minor, header->update_version_build);
        return CY_RSLT_OTA_ERROR_INVALID_VERSION;
    }

    chunk_info->total_size      = header->total_size;
    chunk_info->offset          = header->image_offset;
    chunk_info->buffer          = (uint8_t *)&buffer[header->offset_to_data];
    chunk_info->size            = header->data_size;
    chunk_info->packet_number   = header->this_payload_index;
    chunk_info->total_packets   = header->total_num_payloads;

    return CY_RSLT_SUCCESS;
}

/**
 * @brief Write a chunk of OTA data to FLASH
 *
 * @param[in]   ctx         - ptr to OTA context
 * @param[in]   chunk_info  - ptr to a chunk_info structure
 *
 * @return      CY_RSLT_SUCCESS
 *              CY_RSLT_OTA_ERROR_BADARG
 *              CY_RSLT_OTA_ERROR_WRITE_STORAGE
 */
static cy_rslt_t cy_ota_mqtt_write_chunk_to_flash(cy_ota_context_t *ctx, cy_ota_storage_write_info_t *chunk_info)
{
    cy_rslt_t                 result;
    cy_ota_callback_results_t cb_result;

    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s()\n", __func__);

    if ( (ctx == NULL) || (chunk_info == NULL) )
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() Bad args\n", __func__);
        return CY_RSLT_OTA_ERROR_BADARG;
    }

    ctx->num_packets_received++;    /* this is so we don't have a false failure with the per packet timer */

    cy_log_msg(CYLF_OTA, CY_LOG_INFO, "%s() num_packets_received: %d\n", __func__, ctx->num_packets_received);

    /* check for receipt of duplicate packets - do not write twice */
    if (chunk_info->packet_number >= CY_OTA_MAX_PACKETS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "MQTT PACKET index %d too large. increase  CY_OTA_MAX_PACKETS (%d)\n", chunk_info->packet_number, CY_OTA_MAX_PACKETS);
    }
    else
    {
        ctx->mqtt.received_packets[chunk_info->packet_number]++;
        if (ctx->mqtt.received_packets[chunk_info->packet_number] > 1)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "DEBUG PACKET index %d Duplicate - not written\n", chunk_info->packet_number);
            return CY_RSLT_SUCCESS;
        }
    }

    /* store the chunk */
    ctx->storage = chunk_info;
    cb_result = cy_ota_internal_call_cb(ctx, CY_OTA_REASON_STATE_CHANGE, CY_OTA_STATE_STORAGE_WRITE);
    switch( cb_result )
    {
    default:
		/* Fall through */
    case CY_OTA_CB_RSLT_OTA_CONTINUE:
        result = cy_ota_write_incoming_data_block(ctx, chunk_info);
        if (result != CY_RSLT_SUCCESS)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() Write failed\n", __func__);
            cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_DATA_FAIL, 0);
            return result;
        }
        break;
    case CY_OTA_CB_RSLT_OTA_STOP:
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() App returned OTA Stop for STATE_CHANGE for cy_ota_write_incoming_data_block()\n", __func__);
        return CY_RSLT_OTA_ERROR_APP_RETURNED_STOP;
    case CY_OTA_CB_RSLT_APP_SUCCESS:
        cy_log_msg(CYLF_OTA, CY_LOG_NOTICE, "%s() App returned APP_SUCCESS for STATE_CHANGE for cy_ota_write_incoming_data_block()\n", __func__);
        break;
    case CY_OTA_CB_RSLT_APP_FAILED:
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() App returned APP_FAILED for STATE_CHANGE for cy_ota_write_incoming_data_block()\n", __func__);
        return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
    case CY_OTA_CB_NUM_RESULTS:
        return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
    }

    /* Test for out-of-order chunks
     * Out of order will be a problem when using
     * TAR archives for multi-file downloads.
     * Tar archives must use HTTP for Connection.
     */
    if ( (chunk_info->packet_number > 0) &&
         (chunk_info->packet_number != (ctx->last_packet_received + 1) ) )
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "OUT OF ORDER last:%d current:%d\n",
                    ctx->last_packet_received, chunk_info->packet_number);
    }

    /* update the stats */
    ctx->total_bytes_written   += chunk_info->size;
    ctx->last_offset            = chunk_info->offset;
    ctx->last_size              = chunk_info->size;
    ctx->last_packet_received   = chunk_info->packet_number;
    ctx->total_packets          = chunk_info->total_packets;

    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "Written packet %d of %d to offset:%ld  %ld of %ld\n",
                ctx->last_packet_received, ctx->total_packets,
                ctx->last_offset, ctx->total_bytes_written, ctx->total_image_size);

    return CY_RSLT_SUCCESS;
}

/**
 * @brief Called by the MQTT library when an incoming PUBLISH message is received.
 *
 * The demo uses this callback to handle incoming PUBLISH messages. This callback
 * prints the contents of an incoming message and publishes an acknowledgment
 * to the MQTT server.
 * @param[in] param1 Counts the total number of received PUBLISH messages. This
 * callback will increment this counter.
 * @param[in] pPublish Information about the incoming PUBLISH message passed by
 * the MQTT library.
 */

static void cy_ota_mqtt_callback( cy_mqtt_t mqtt_handle, cy_mqtt_event_t event, void *user_data )
{
	/* static so that it does not use stack space */
	static cy_ota_storage_write_info_t mqtt_chunk_info;
    cy_rslt_t   result = CY_RSLT_SUCCESS;
    cy_ota_context_t *ctx = (cy_ota_context_t *)user_data;

    if ( (mqtt_handle == NULL) || (user_data == NULL) )
    {
        /* bad callback - no info! */
       cy_log_msg(CYLF_OTA, CY_LOG_WARNING, "%s() Bad args! (%p ,, %p)\n", __func__, mqtt_handle, user_data);
       return;
    }

    CY_OTA_CONTEXT_ASSERT(ctx);

    if (event.type == CY_MQTT_EVENT_TYPE_DISCONNECT)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "Network disconnected..........reason: %d\n", event.data.reason);
        if (event.data.reason ==  CY_MQTT_DISCONN_TYPE_BROKER_DOWN)
        {
            /* Only report disconnect if we are downloading */
            if ( (ctx->curr_state == CY_OTA_STATE_JOB_DOWNLOAD) ||
                 (ctx->curr_state == CY_OTA_STATE_DATA_DOWNLOAD) ||
                 (ctx->curr_state == CY_OTA_STATE_RESULT_SEND) ||
                 (ctx->curr_state == CY_OTA_STATE_RESULT_RESPONSE) )
            {
                cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() CY_OTA_EVENT_DROPPED_US Network MQTT disconnect reason:%d state:%d %s!\n",
                        __func__,  event.data.reason, ctx->curr_state, cy_ota_get_state_string(ctx->curr_state));
                cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_DROPPED_US, 0);
            }
        }
    }

    if (event.type == CY_MQTT_EVENT_TYPE_PUBLISH_RECEIVE)
    {
        /* Make sure the callback we get is when we are expecting it
        * Do this before grabbing the Mutex !
        */
        cy_log_msg(CYLF_OTA, CY_LOG_INFO, "               CY_MQTT_EVENT_TYPE_PUBLISH_RECEIVE !! state:%d mutex:%d\n", ctx->curr_state, ctx->sub_callback_mutex_inited);

       if (ctx->curr_state == CY_OTA_STATE_JOB_DOWNLOAD)
       {
           cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() Received Job packet.\n", __func__);
       }
       else if (ctx->curr_state == CY_OTA_STATE_RESULT_SEND)
       {
           cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() Received Result response.\n", __func__);
       }
       else if ( (ctx->curr_state != CY_OTA_STATE_DATA_DOWNLOAD) || (ctx->sub_callback_mutex_inited != 1) )
       {
	   const cy_ota_mqtt_chunk_payload_header_t *header;
           CY_ASSERT(event.data.pub_msg.received_message.payload != NULL);

           cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() Received DATA.\n", __func__);

           header = (const cy_ota_mqtt_chunk_payload_header_t *)event.data.pub_msg.received_message.payload;
           if (memcmp(header->magic, (uint8_t *)CY_OTA_MQTT_MAGIC, (strlen(CY_OTA_MQTT_MAGIC) - 1) ) != 0 )
           {
               cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() Received packet outside of downloading on topic %.*s.\n", __func__,
                          event.data.pub_msg.received_message.topic_len, event.data.pub_msg.received_message.topic);
           }

           /* return before we get the mutex */
           return;
       }
       else
       {
           cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() Unknown packet.\n", __func__);
       }

       result = cy_rtos_get_mutex(&ctx->sub_callback_mutex,(CY_OTA_WAIT_MQTT_MUTEX_MS) );
       if (result != CY_RSLT_SUCCESS)
       {
           /* we didn't get the mutex - something is wrong! */
           cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() Mutex timeout!\n", __func__);
           return;
       }

#ifdef DEBUG_PACKET_RECEIPT_TIME_DIFF
       {
           static cy_time_t last_packet_time, curr_packet_time;
           cy_rtos_get_time(&curr_packet_time);
           if (last_packet_time != 0)
           {
               cy_time_t diff = curr_packet_time - last_packet_time;
               cy_log_msg(CYLF_OTA, CY_LOG_INFO, "Time difference from last packet rec'd: %ld\n", diff);
           }
           last_packet_time = curr_packet_time;
       }
#endif

       CY_ASSERT(event.data.pub_msg.received_message.payload != NULL);

       cy_mqtt_publish_info_t *pub_msg = &event.data.pub_msg.received_message;
       cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "\n\n");
       cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "Received pub_msg:\n");
       cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "               qos: %d\n", pub_msg->qos);
       cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "            retain: %d\n", pub_msg->retain);
       cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "         duplicate: %d\n", pub_msg->dup);
       cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "             Topic: %.*s\n", pub_msg->topic_len, pub_msg->topic);
       cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "    payload length: %d\n", pub_msg->payload_len);
       cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "           payload: %p\n", pub_msg->payload);

#ifdef PRINT_MQTT_DATA
       cy_ota_print_data(pub_msg->payload, 32);
#endif

       if (ctx->curr_state == CY_OTA_STATE_JOB_DOWNLOAD)
       {
           if ( (pub_msg->payload == NULL) || (pub_msg->payload_len == 0) )
           {
               cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "Payload from Publisher Error!\n");
               result = CY_RSLT_OTA_ERROR_BADARG;
               goto _callback_exit;
           }

           /* Copy the Job document. We send it back to the Broker when we send the unique topic name
            * We parse the job document in cy_ota_agent.c
            */
           if (pub_msg->payload_len > sizeof(ctx->job_doc) )
           {
               cy_log_msg(CYLF_OTA, CY_LOG_ERR, "MQTT: Job doc too long! %d bytes! Change CY_OTA_JOB_MAX_LEN!\n", pub_msg->payload_len);
               result = CY_RSLT_OTA_ERROR_MALFORMED_JOB_DOC;
               goto _callback_exit;
           }
           memset(ctx->job_doc, 0x00, sizeof(ctx->job_doc) );
           memcpy(ctx->job_doc, pub_msg->payload, pub_msg->payload_len);
           result = CY_RSLT_SUCCESS;
       }
       else if (ctx->curr_state == CY_OTA_STATE_RESULT_SEND)
       {
           /* TODO: STDE - Do we need to do anything? maybe just acknowledge for OTA Agent? */
           result = CY_RSLT_SUCCESS;
       }
       else if (ctx->curr_state == CY_OTA_STATE_DATA_DOWNLOAD)
       {
           result = cy_ota_mqtt_parse_chunk( (const uint8_t *)pub_msg->payload, (uint32_t)pub_msg->payload_len, &mqtt_chunk_info);

           if (result == CY_RSLT_SUCCESS)
           {
               cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "Received packet %d of %d\n", mqtt_chunk_info.packet_number, mqtt_chunk_info.total_packets);

               /* update ctx with appropriate sizes */
               if (ctx->total_image_size == 0)
               {
                   ctx->total_image_size = mqtt_chunk_info.total_size;
               }

               /* write the data */
               result = cy_ota_mqtt_write_chunk_to_flash(ctx, &mqtt_chunk_info);
               /* Errors handled below in _callback_exit */
           }
           else
           {
               cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "Packet %d had errors in header\n", mqtt_chunk_info.packet_number);
           }
       }
       else
       {
           result = CY_RSLT_OTA_ERROR_GENERAL;
       }

_callback_exit:

       /* Handle Job and Data error conditions */
       if (result == CY_RSLT_OTA_ERROR_MALFORMED_JOB_DOC)
       {
           cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, " CY_RSLT_OTA_ERROR_MALFORMED_JOB_DOC !\n");
           cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_MALFORMED_JOB_DOC, 0);
       }
       else if (result == CY_RSLT_OTA_ERROR_WRITE_STORAGE)
       {
           cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, " CY_OTA_EVENT_STORAGE_ERROR !\n");
           cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_STORAGE_ERROR, 0);
       }
       else if(result == CY_RSLT_OTA_ERROR_APP_RETURNED_STOP)
       {
           cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, " CY_OTA_EVENT_APP_STOPPED_OTA !\n");
           cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_APP_STOPPED_OTA, 0);
       }
       else if (result == CY_RSLT_OTA_ERROR_INVALID_VERSION)
       {
           cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, " CY_OTA_EVENT_INVALID_VERSION !\n");
           cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_INVALID_VERSION, 0);
       }
       else if (result != CY_RSLT_SUCCESS)
       {
           cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, " CY_OTA_EVENT_DATA_FAIL !\n");
           cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_DATA_FAIL, 0);
       }
       else
       {
           cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, " CY_OTA_EVENT_GOT_DATA!\n");
           cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_GOT_DATA, 0);
       }
       cy_rtos_set_mutex(&ctx->sub_callback_mutex);
    }

}

/*-----------------------------------------------------------*/

/**
 * @brief Establish a new connection to the MQTT server.
 *
 * @param[in] awsIotMqttMode Specify if this demo is running with the AWS IoT
 * MQTT server. Set this to `false` if using another MQTT server.
 * @param[in] pIdentifier NULL-terminated MQTT client identifier.
 * @param[in] pNetworkServerInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkCredentialInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkInterface The network interface to use for this demo.
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GET_JOB
 *
 */
static cy_rslt_t cy_ota_establish_MQTT_connection(cy_ota_context_t *ctx,
                                                 bool awsIotMqttMode,
                                                 char * pIdentifier,
                                                 cy_awsport_ssl_credentials_t *security )
{
    cy_rslt_t               result;
    uint8_t                 *buffer;
    uint16_t                buff_len;
    cy_mqtt_publish_info_t  will_info;
    cy_mqtt_connect_info_t  connect_info;
    cy_mqtt_broker_info_t   broker_info;

    CY_OTA_CONTEXT_ASSERT(ctx);

    /* Point to a buffer (part of the ota context) */
    buffer = ctx->chunk_buffer;
    buff_len = sizeof(ctx->chunk_buffer);

    /* will info */
    memset(&will_info, 0x00, sizeof(will_info));
    will_info.qos           = CY_MQTT_QOS1;
    will_info.topic         = WILL_TOPIC_NAME;
    will_info.topic_len     = WILL_TOPIC_NAME_LENGTH;
    will_info.payload       = WILL_MESSAGE;
    will_info.payload_len   = WILL_MESSAGE_LENGTH;

    /* set up broker info */
    memset(&broker_info, 0x00, sizeof(cy_mqtt_broker_info_t) );
    broker_info.hostname       = ctx->curr_server->host_name;
    broker_info.hostname_len   = strlen(broker_info.hostname);
    broker_info.port           = ctx->curr_server->port;           /** Server port in host-order. */

    /* set up connect info */
    memset(&connect_info, 0x00, sizeof(cy_mqtt_connect_info_t) );
    connect_info.client_id      = pIdentifier;
    connect_info.client_id_len  = (uint16_t ) strlen(pIdentifier);
    if (security != NULL)
    {
        connect_info.username       = security->username;
        connect_info.username_len   = security->username_size;
        connect_info.password       = security->password;
        connect_info.password_len   = security->password_size;
    }
    connect_info.clean_session =  (ctx->network_params.mqtt.session_type == CY_OTA_MQTT_SESSION_CLEAN) ? true : false;
    connect_info.keep_alive_sec = CY_OTA_MQTT_KEEP_ALIVE_SECONDS;
    connect_info.will_info = &will_info;

    /* Print info for log_level DEBUG */
    cy_ota_print_connect_info(&connect_info);
    cy_ota_print_broker_info(&broker_info);

    result = cy_mqtt_create( buffer, buff_len,
                             security,
                             &broker_info,
                             cy_ota_mqtt_callback,
                             (void *)ctx,                   /* User data for callback */
                             &ctx->mqtt.mqtt_connection );
    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "cy_mqtt_create() failed result:0x%lx\n", result);
        return CY_RSLT_OTA_ERROR_GENERAL;
    }

    result = cy_mqtt_connect(ctx->mqtt.mqtt_connection,
                             &connect_info);
    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "cy_mqtt_connect() failed result:0x%lx\n", result);
        cy_mqtt_delete(ctx->mqtt.mqtt_connection);
        return CY_RSLT_OTA_ERROR_GENERAL;
    }

    return result;
}
/*-----------------------------------------------------------*/

/**
 * @brief Add or remove subscriptions by either subscribing or unsubscribing.
 *
 * @param[in] operation Either #0 (subscribe) or #1 Unsubscribe.
 * @param[in] pTopicFilters Array of topic filters for subscriptions.
 *
 * @return CY_RSLT_SUCCESS
 *         CY_RSLT_OTA_ERROR_MQTT_SUBSCRIBE
 *
 */
static cy_rslt_t cy_ota_modify_subscriptions(cy_ota_context_t *ctx,
                                             uint8_t operation,
                                             uint8_t numTopics,
                                             const char ** pTopicFilters)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    int32_t i = 0;

    cy_mqtt_subscribe_info_t sub_msgs[ CY_OTA_MQTT_MAX_TOPICS ];

    if ( (numTopics > 0) && (pTopicFilters != NULL) && (*pTopicFilters != NULL) )
    {
        /* Set the members of the subscription list. */
        for(i = 0; i < numTopics; i++ )
        {
            sub_msgs[ i ].qos       = CY_MQTT_QOS1;
            sub_msgs[ i ].topic     = pTopicFilters[ i ];
            sub_msgs[ i ].topic_len = strlen(pTopicFilters[ i ]);
        }

        /* Modify subscriptions by either subscribing or unsubscribing. */
        if (operation == CY_OTA_MQTT_SUBSCRIBE)
        {
            result = cy_mqtt_subscribe( ctx->mqtt.mqtt_connection,
                                                    sub_msgs,
                                                    numTopics );

            /* Check the status of SUBSCRIBE. */
            if (result != CY_RSLT_SUCCESS)
            {
                cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "Subscribe failed result:0x%lx\n", result);
            }
        }
        else
        {
            result = cy_mqtt_unsubscribe( ctx->mqtt.mqtt_connection,
                                                    sub_msgs,
                                                    numTopics );

            /* Check the status of SUBSCRIBE. */
            if (result != CY_RSLT_SUCCESS)
            {
                cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "UN-Subscribe failed\n");
            }
        }

        if (operation == CY_OTA_MQTT_SUBSCRIBE)
        {
            for(i = 0; i < numTopics; i++ )
            {
                cy_log_msg(CYLF_OTA, CY_LOG_NOTICE, "   MQTT Topic: %s\n", pTopicFilters[ i ]);
            }
        }
    }
    return result;
}


/**
 * @brief Validate network parameters
 *
 * NOTE: Individual Network Connection type will test appropriate fields
 *
 * @param[in]  network_params   pointer to Network parameter structure
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_mqtt_validate_network_params(const cy_ota_network_params_t *network_params)
{
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s()\n", __func__);
    if (network_params == NULL)
    {
        return CY_RSLT_OTA_ERROR_BADARG;
    }

    if ( (network_params->mqtt.pIdentifier == NULL) ||
         (network_params->mqtt.broker.host_name == NULL) ||
         (network_params->mqtt.broker.port == 0) )
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() BAD ARGS\n", __func__);
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "    Identifier:%s    Broker:%s : %d \n",
                    network_params->mqtt.pIdentifier,
                    network_params->mqtt.broker.host_name,
                    network_params->mqtt.broker.port);
        return CY_RSLT_OTA_ERROR_BADARG;
    }

    return CY_RSLT_SUCCESS;
}

/**
 * @brief Connect to OTA Update server
 *
 * NOTE: Individual Network Connection type will do whatever is necessary
 *       This function must have it's own timeout for connection failure
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_mqtt_connect(cy_ota_context_t *ctx)
{
    cy_time_t                   tval;
    cy_rslt_t                   result;
    cy_awsport_server_info_t    server;
    cy_awsport_ssl_credentials_t *security;
    char pClientIdentifierBuffer[ CLIENT_IDENTIFIER_MAX_LENGTH + 1 ];

    CY_OTA_CONTEXT_ASSERT(ctx);

    security = &ctx->network_params.mqtt.credentials;

    server.host_name = ctx->curr_server->host_name;
    server.port = ctx->curr_server->port;

    /* If application changed job_doc we need to use the parsed info */
    if ( (ctx->curr_state == CY_OTA_STATE_DATA_CONNECT) &&
         (ctx->network_params.use_get_job_flow == CY_OTA_JOB_FLOW)  &&
         (ctx->parsed_job.parse_result == CY_RSLT_OTA_CHANGING_SERVER) )
    {
        server.host_name = ctx->parsed_job.broker_server.host_name;
        server.port = ctx->parsed_job.broker_server.port;
        if (ctx->callback_data.credentials != NULL)
        {
            security = ctx->callback_data.credentials;
        }
    }

    if (ctx->curr_server->port == CY_OTA_MQTT_BROKER_PORT)
    {
        security = NULL;
    }

    /* Use the parameter client identifier if provided. Otherwise, generate a
     * unique client identifier. */
    cy_rtos_get_time(&tval);
    memset(pClientIdentifierBuffer, 0x00, sizeof(pClientIdentifierBuffer) );

    if ( (ctx->network_params.mqtt.pIdentifier == NULL) || (strlen(ctx->network_params.mqtt.pIdentifier) == 0 ) )
    {
		if (strlen(CY_OTA_MQTT_CLIENT_ID_PREFIX) > (CLIENT_IDENTIFIER_MAX_LENGTH - UINT16_DECIMAL_LENGTH) )
		{
		size_t copy_max = (CLIENT_IDENTIFIER_MAX_LENGTH - UINT16_DECIMAL_LENGTH);
		if (strlen(CY_OTA_MQTT_CLIENT_ID_PREFIX) < copy_max)
		{
			copy_max = strlen(CY_OTA_MQTT_CLIENT_ID_PREFIX);
		}
			strncpy(pClientIdentifierBuffer, CY_OTA_MQTT_CLIENT_ID_PREFIX, copy_max );
		}
		else
		{
			strcpy(pClientIdentifierBuffer, CY_OTA_MQTT_CLIENT_ID_PREFIX);
		}
    }
    else
    {
		if (strlen(ctx->network_params.mqtt.pIdentifier) > (CLIENT_IDENTIFIER_MAX_LENGTH - UINT16_DECIMAL_LENGTH) )
		{
				strncpy(pClientIdentifierBuffer, ctx->network_params.mqtt.pIdentifier, (CLIENT_IDENTIFIER_MAX_LENGTH - UINT16_DECIMAL_LENGTH) );
		}
		else
		{
			strcpy(pClientIdentifierBuffer, ctx->network_params.mqtt.pIdentifier);
		}
    }

    /* Every active MQTT connection must have a unique client identifier. We
     * generate this unique client identifier by appending a timestamp to a common
     * prefix. */
    snprintf(&pClientIdentifierBuffer[strlen(pClientIdentifierBuffer)],
                          UINT16_DECIMAL_LENGTH, "%d", (uint16_t)(tval & 0x0000FFFF) );

    /* Establish a new MQTT connection. called function has a timeout */
    result = cy_ota_establish_MQTT_connection(ctx,
                                              ctx->network_params.mqtt.awsIotMqttMode,
                                              pClientIdentifierBuffer,
                                              security);

    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "MQTT Connection %s:%d failed\n",
                               server.host_name, server.port);
        result = CY_RSLT_OTA_ERROR_MQTT_INIT;
    }
    else
    {
        /* Mark the MQTT connection as established. */
        ctx->mqtt.connection_established = true;

        cy_log_msg(CYLF_OTA, CY_LOG_INFO, "MQTT %p Connect SUCCESS ID: '%s' broker: %s:%d TLS:%s\n",
                        ctx->mqtt.mqtt_connection,
                       pClientIdentifierBuffer,
                       server.host_name,
                       server.port,
                       (security == NULL) ? "No" : "Yes");

        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "MQTT Subscribe topics from Application.\n");
        result = cy_ota_modify_subscriptions(ctx,
                                             CY_OTA_MQTT_SUBSCRIBE,
                                             ctx->network_params.mqtt.numTopicFilters,
                                             ctx->network_params.mqtt.pTopicFilters);
        if (result != CY_RSLT_SUCCESS)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_ERR, "MQTT subscribe failed\n");
            cy_ota_mqtt_disconnect(ctx);
        }
    }
    return result;
}

/**
 * @brief get the OTA job
 *
 * NOTE: Individual Network Connection type will do whatever is necessary
 *      ex: MQTT
 *          - subscribe to start data transfer
 *          TCP
 *          - pull the data from the server
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_mqtt_get_job(cy_ota_context_t *ctx)
{
    uint32_t    waitfor_clear;
    cy_rslt_t   result = CY_RSLT_SUCCESS;
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s()\n", __func__);

    CY_OTA_CONTEXT_ASSERT(ctx);

    if (ctx->mqtt.connection_established != true)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() connection not established\n", __func__);
        return CY_RSLT_OTA_ERROR_GET_JOB;
    }

    if (cy_rtos_init_mutex(&ctx->sub_callback_mutex) != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() sub_callback_mutex init failed\n", __func__);
        return CY_RSLT_OTA_ERROR_GET_JOB;
    }
    ctx->sub_callback_mutex_inited = 1;

    /* clear any lingering events */
    waitfor_clear = CY_OTA_EVENT_MQTT_EVENTS;
    result = cy_rtos_waitbits_event(&ctx->ota_event, &waitfor_clear, 1, 0, 1);
    if (waitfor_clear != 0)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() Clearing waitfor: 0x%lx\n", __func__, waitfor_clear);
    }

    /* Ask if there is an update available */
    result = cy_ota_subscribe_to_unique_topic(ctx);
    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() subscribe unique () failed result:0x%lx\n", __func__, result);
        goto cleanup_and_exit;
    }
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "MQTT Subscribe unique completed\n");

    /* Ask if there is an update available */
    result = cy_ota_mqtt_create_json_request(ctx, CY_OTA_SUBSCRIBE_UPDATES_AVAIL, " ", 0, -1);
    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() cy_ota_mqtt_create_json_request() failed\n", __func__);
        result = CY_RSLT_OTA_ERROR_MQTT_PUBLISH;
        goto cleanup_and_exit;
    }

    result = cy_ota_mqtt_publish_request(ctx, (char *)SUBSCRIBER_PUBLISH_TOPIC, ctx->mqtt.json_doc);
    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() cy_ota_mqtt_publish_request() failed\n", __func__);
        result = CY_RSLT_OTA_ERROR_MQTT_PUBLISH;
        goto cleanup_and_exit;
    }

    while ( true )
    {
        uint32_t waitfor;

        /* get event */
        waitfor = CY_OTA_EVENT_MQTT_EVENTS;
        result = cy_rtos_waitbits_event(&ctx->ota_event, &waitfor, 1, 0, CY_OTA_WAIT_MQTT_EVENTS_MS);
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() MQTT cy_rtos_waitbits_event: 0x%lx result:0x%lx\n", __func__, waitfor, result);

        /* We only want to act on events we are waiting on.
         * For timeouts, just loop around.
         */
        if (waitfor == 0)
        {
            continue;
        }
        if (waitfor & CY_OTA_EVENT_SHUTDOWN_NOW)
        {
            /* Pass along to Agent thread */
            cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_SHUTDOWN_NOW, 0);
            result = CY_RSLT_SUCCESS;
            break;
        }

        if (waitfor & CY_OTA_EVENT_DATA_DOWNLOAD_TIMEOUT)
        {
            /* This was generated by a timer in cy_ota_agent.c
             * Pass along to Agent thread.
             */
            cy_log_msg(CYLF_OTA, CY_LOG_INFO, "MQTT: JOB Download Timeout return:0x%lx\n", CY_RSLT_OTA_NO_UPDATE_AVAILABLE);
            result = CY_RSLT_OTA_NO_UPDATE_AVAILABLE;
            break;
        }

        if (waitfor & CY_OTA_EVENT_GOT_DATA)
        {
            /* If we get malformed (short) job doc, look into using
             * DATA_DONE instead of GOT_DATA
             */
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "MQTT: JOB Download Got Data\n");
            result = CY_RSLT_SUCCESS;
            break;
        }

        if (waitfor & CY_OTA_EVENT_MALFORMED_JOB_DOC)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() MQTT CY_OTA_EVENT_MALFORMED_JOB_DOC\n", __func__);
            result = CY_RSLT_OTA_ERROR_MALFORMED_JOB_DOC;
            break;
        }

        if (waitfor & CY_OTA_EVENT_INVALID_VERSION)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() MQTT CY_OTA_EVENT_INVALID_VERSION\n", __func__);
            result = CY_RSLT_OTA_ERROR_INVALID_VERSION;
            break;
        }

        if (waitfor & CY_OTA_EVENT_DATA_FAIL)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() MQTT CY_OTA_EVENT_DATA_FAIL\n", __func__);
            result = CY_RSLT_OTA_ERROR_GET_JOB;
            break;
        }

        if (waitfor & CY_OTA_EVENT_DROPPED_US)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() MQTT CY_OTA_EVENT_DROPPED_US state:%d %s\n", __func__, ctx->curr_state, cy_ota_get_state_string(ctx->curr_state));
            result = CY_RSLT_OTA_ERROR_SERVER_DROPPED;
            break;
        }

    }

  cleanup_and_exit:
    ctx->sub_callback_mutex_inited = 0;
    cy_rtos_deinit_mutex(&ctx->sub_callback_mutex);

    return result;
}

/**
 * @brief get the OTA download
 *
 * NOTE: Individual Network Connection type will do whatever is necessary
 *      ex: MQTT
 *          - subscribe to start data transfer
 *          TCP
 *          - pull the data from the server
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_mqtt_get_data(cy_ota_context_t *ctx)
{
	uint16_t                    i;
    uint32_t                    waitfor_clear;
    cy_rslt_t                   result = CY_RSLT_SUCCESS;
    cy_ota_callback_results_t   cb_result;

    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s()\n", __func__);

    CY_OTA_CONTEXT_ASSERT(ctx);

    if (ctx->mqtt.connection_established != true)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() connection not established\n", __func__);
        return CY_RSLT_OTA_ERROR_GET_DATA;
    }

    if (cy_rtos_init_mutex(&ctx->sub_callback_mutex) != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() sub_callback_mutex init failed\n", __func__);
        return CY_RSLT_OTA_ERROR_GET_DATA;
    }
    ctx->sub_callback_mutex_inited = 1;

    /* clear any lingering events */
    waitfor_clear = CY_OTA_EVENT_MQTT_EVENTS;
    result = cy_rtos_waitbits_event(&ctx->ota_event, &waitfor_clear, 1, 0, 1);
    if (waitfor_clear != 0)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() Clearing waitfor: 0x%lx\n", __func__, waitfor_clear);
    }

        /* we might have gotten the Job from HTTP, always subscribe to unique topic */
    result = cy_ota_subscribe_to_unique_topic(ctx);
    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() subscribe/publish () failed result:0x%lx\n", __func__, result);
        goto cleanup_and_exit;
    }

    /* Create json doc for the request */
    memset(ctx->mqtt.json_doc, 0x00, sizeof(ctx->mqtt.json_doc));
#ifdef CY_MQTT_GET_ALL_DATA_WITH_ONE_CALL
    /* Current default. Send one request for the entire file,
     * Publisher.py will chunk and send separate chunks.
     */
    result = cy_ota_mqtt_create_json_request(ctx, CY_OTA_DOWNLOAD_REQUEST, "", 0, 0);
#else
    /* This code is for requesting each chunk separately. */
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "MQTT Subscribe for CHUNK download DATA Messages..............\n");
    result = cy_ota_mqtt_create_json_request(ctx, CY_OTA_DOWNLOAD_CHUNK_REQUEST,
                                                        ctx->parsed_job.file, 0, CY_OTA_CHUNK_SIZE);
#endif

    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() cy_ota_mqtt_create_json_request() failed\n", __func__);
        result = CY_RSLT_OTA_ERROR_MQTT_PUBLISH;
    }

    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%d : %s() CALLING CB STATE_CHANGE %s stop_OTA_session:%d\n", __LINE__, __func__,
                cy_ota_get_state_string(ctx->curr_state), ctx->stop_OTA_session);

    cb_result = cy_ota_internal_call_cb(ctx, CY_OTA_REASON_STATE_CHANGE, ctx->curr_state);
    switch( cb_result )
    {
    default:
		/* Fall through */
    case CY_OTA_CB_RSLT_OTA_CONTINUE:
        result = cy_ota_mqtt_publish_request(ctx, (char *)SUBSCRIBER_PUBLISH_TOPIC, ctx->mqtt.json_doc);
        if (result != CY_RSLT_SUCCESS)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() cy_ota_mqtt_publish_request() for Data failed\n", __func__);
            goto cleanup_and_exit;
        }
        break;

    case CY_OTA_CB_RSLT_OTA_STOP:
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() App returned OTA Stop for STATE_CHANGE for DATA_DOWNLOAD\n", __func__);
        result = CY_RSLT_OTA_ERROR_APP_RETURNED_STOP;
        goto cleanup_and_exit;

    case CY_OTA_CB_RSLT_APP_SUCCESS:
        cy_log_msg(CYLF_OTA, CY_LOG_NOTICE, "%s() App returned APP_SUCCESS for STATE_CHANGE for DATA_DOWNLOAD\n", __func__);
        result = CY_RSLT_SUCCESS;
        goto cleanup_and_exit;

    case CY_OTA_CB_RSLT_APP_FAILED:
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() App returned APP_FAILURE for STATE_CHANGE for DATA_DOWNLOAD\n", __func__);
        result = CY_RSLT_OTA_ERROR_GET_DATA;
        goto cleanup_and_exit;

    case CY_OTA_CB_NUM_RESULTS:
        result = CY_RSLT_OTA_ERROR_GET_DATA;
        goto cleanup_and_exit;
    }

    /* Create download interval timer */
   result = cy_rtos_init_timer(&ctx->mqtt.mqtt_timer, CY_TIMER_TYPE_ONCE,
                               cy_ota_mqtt_timer_callback, (cy_timer_callback_arg_t)ctx);
   if (result != CY_RSLT_SUCCESS)
   {
       /* Event create failed */
       cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() Timer Create Failed!\n", __func__);
       result = CY_RSLT_OTA_ERROR_GET_DATA;
       goto cleanup_and_exit;
   }
   ctx->mqtt.mqtt_timer_inited = true;

   if (ctx->packet_timeout_sec > 0 )
   {
       /* Start the download interval timer */
       cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() MQTT DATA START PACKET TIMER %ld secs\n", __func__, ctx->packet_timeout_sec);
       cy_ota_start_mqtt_timer(ctx, ctx->packet_timeout_sec, CY_OTA_EVENT_PACKET_TIMEOUT);
   }

   /* clear out tally of received / written packets */
    memset(&ctx->mqtt.received_packets, 0, sizeof(ctx->mqtt.received_packets) );

    while (true)
    {
        uint32_t waitfor;

        /* get event */
        waitfor = CY_OTA_EVENT_MQTT_EVENTS;
        result = cy_rtos_waitbits_event(&ctx->ota_event, &waitfor, 1, 0, CY_OTA_WAIT_MQTT_EVENTS_MS);
        cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() MQTT cy_rtos_waitbits_event: 0x%lx type:%d mod:0x%lx code:%d\n", __func__, waitfor, CY_RSLT_GET_TYPE(result), CY_RSLT_GET_MODULE(result), CY_RSLT_GET_CODE(result) );

        /* We only want to act on events we are waiting on.
         * For timeouts, just loop around.
         */
        if (waitfor == 0)
        {
            continue;
        }

        if (waitfor & CY_OTA_EVENT_SHUTDOWN_NOW)
        {
            /* Pass along to Agent thread */
            cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_SHUTDOWN_NOW, 0);
            result = CY_RSLT_SUCCESS;
            break;
        }

        if (waitfor & CY_OTA_EVENT_DATA_DOWNLOAD_TIMEOUT)
        {
            /* This was generated by a timer in cy_ota_agent.c
             * Pass along to Agent thread.
             */
            cy_log_msg(CYLF_OTA, CY_LOG_WARNING, "MQTT: Download Timeout\n");
            result = CY_RSLT_OTA_NO_UPDATE_AVAILABLE;
            break;
        }

        if (waitfor & CY_OTA_EVENT_STORAGE_ERROR)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() Storage write error\n", __func__);
            result = CY_RSLT_OTA_ERROR_WRITE_STORAGE;
            break;
        }

        if (waitfor & CY_OTA_EVENT_APP_STOPPED_OTA)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() App told us to stop\n", __func__);
            result = CY_RSLT_OTA_ERROR_APP_RETURNED_STOP;
            break;
        }

        if (waitfor & CY_OTA_EVENT_GOT_DATA)
        {
            if (ctx->packet_timeout_sec > 0 )
            {
                /* got some data - restart the download interval timer */
                cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() RESTART PACKET TIMER %ld secs\n", __func__, ctx->packet_timeout_sec);
                cy_ota_start_mqtt_timer(ctx, ctx->packet_timeout_sec, CY_OTA_EVENT_PACKET_TIMEOUT);
            }

            if (ctx->total_bytes_written >= ctx->total_image_size)
            {
                /* stop timer asap so we don't get a timeout */
                cy_ota_stop_mqtt_timer(ctx);

                cy_log_msg(CYLF_OTA, CY_LOG_INFO, "Done writing all data! %ld of %ld\n", ctx->total_bytes_written, ctx->total_image_size);
                cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_DATA_DONE, 0);
            }

#ifndef CY_MQTT_GET_ALL_DATA_WITH_ONE_CALL
            /* This code is only used if we are going to ask the MQTT broker
             * separately for each chunk of data.
             *
             * Request next chunk */
            int32_t chunk_size = (int32_t)CY_OTA_CHUNK_SIZE;
            if (chunk_size > (ctx->total_image_size - ctx->total_bytes_written) )
            {
                chunk_size = -1;    /* get the rest of the data */
            }
            result = cy_ota_mqtt_create_json_request( ctx, CY_OTA_DOWNLOAD_CHUNK_REQUEST,
                    ctx->parsed_job.file, ctx->total_bytes_written, chunk_size);
            if (result != CY_RSLT_SUCCESS)
            {
                cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() cy_ota_mqtt_create_json_request() for Data failed\n", __func__);
                goto cleanup_and_exit;
            }

            result = cy_ota_mqtt_publish_request(ctx, (char *)SUBSCRIBER_PUBLISH_TOPIC, ctx->mqtt.json_doc);
            if (result != CY_RSLT_SUCCESS)
            {
                cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() cy_ota_mqtt_publish_request() for Data failed\n", __func__);
                goto cleanup_and_exit;
            }
#endif
            continue;
        }

        if (waitfor & CY_OTA_EVENT_PACKET_TIMEOUT)
        {
            /* We set a timer and if packets take too long, we will assume the broker forgot about us.
             * Set with CY_OTA_PACKET_INTERVAL_SECS.
             */
            if (ctx->num_packets_received > ctx->last_num_packets_received)
            {
                /* If we received packets since the last time we were here, just continue.
                 * This thread may be held off for a while, and we don't want a false failure.
                 */
                cy_log_msg(CYLF_OTA, CY_LOG_WARNING, "%s() RESTART PACKET TIMER %ld secs\n", __func__, ctx->packet_timeout_sec);
                cy_ota_start_mqtt_timer(ctx, ctx->packet_timeout_sec, CY_OTA_EVENT_PACKET_TIMEOUT);

                /* update our variable */
                ctx->last_num_packets_received = ctx->num_packets_received;
                continue;
            }
            cy_log_msg(CYLF_OTA, CY_LOG_WARNING, "OTA Timeout waiting for a packet (%d seconds), fail\n", ctx->packet_timeout_sec);
            cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_DATA_FAIL, 0);
        }

        if (waitfor & CY_OTA_EVENT_DATA_DONE)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "Got all the data !\n");
            result = CY_RSLT_SUCCESS;
            break;
        }

        if (waitfor & CY_OTA_EVENT_INVALID_VERSION)
        {
            result = CY_RSLT_OTA_ERROR_INVALID_VERSION;
            break;
        }

        if (waitfor & CY_OTA_EVENT_DATA_FAIL)
        {
            result = CY_RSLT_OTA_ERROR_GET_DATA;
            break;
        }

        if (waitfor & CY_OTA_EVENT_DROPPED_US)
        {
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG, "%s() MQTT Broker disconnected state:%d %s\n", __func__, ctx->curr_state, cy_ota_get_state_string(ctx->curr_state));
            result = CY_RSLT_OTA_ERROR_SERVER_DROPPED;
            break;
        }
    }   /* While 1 */

    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s() MQTT DONE result: 0x%lx\n", __func__, result);

    for (i = 0; i < ctx->total_packets; i++)
    {
        if (ctx->mqtt.received_packets[i] == 0 )
        {
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG1, "PACKET %d missing!\n", i);
        }
        else if (ctx->mqtt.received_packets[i] > 1 )
        {
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG1, "PACKET %d Duplicate!\n", i);
        }
        else
        {
            cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "PACKET %d Normal!\n", i);
        }
    }

 cleanup_and_exit:
    if (ctx->mqtt.mqtt_timer_inited)
    {
        /* we completed the download, stop the timer */
        cy_ota_stop_mqtt_timer(ctx);

        cy_rtos_deinit_timer(&ctx->mqtt.mqtt_timer);
    }
    ctx->mqtt.mqtt_timer_inited = false;

    ctx->sub_callback_mutex_inited = 0;
    cy_rtos_deinit_mutex(&ctx->sub_callback_mutex);

    return result;
}

/**
 * @brief Disconnect from MQTT broker
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_mqtt_disconnect(cy_ota_context_t *ctx)
{
    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%s()\n", __func__);
    CY_OTA_CONTEXT_ASSERT(ctx);

    if (ctx->mqtt.connection_established == true)
    {
	/* Remove the topic filters subscriptions */
		cy_ota_modify_subscriptions(ctx,
									CY_OTA_MQTT_UNSUBSCRIBE,
									ctx->network_params.mqtt.numTopicFilters,
									ctx->network_params.mqtt.pTopicFilters);

		/* unsubscribe from unique topic (if we used one) */
		if (ctx->mqtt.unique_topic_subscribed == true)
		{
			if (strlen(ctx->mqtt.unique_topic) > 0)
			{
			    char        *unique_topic[1];
		        unique_topic[0] = ctx->mqtt.unique_topic;
				cy_ota_modify_subscriptions(ctx,
											CY_OTA_MQTT_UNSUBSCRIBE,
											1,
											(const char **)unique_topic);
			}
			ctx->mqtt.unique_topic_subscribed = false;
		}
    }

    if (ctx->mqtt.connection_from_app == false)
    {

        if (ctx->mqtt.connection_established == true)
        {
            ctx->mqtt.connection_established = false;
            cy_mqtt_disconnect( ctx->mqtt.mqtt_connection );
            cy_mqtt_delete( ctx->mqtt.mqtt_connection );
            ctx->mqtt.mqtt_connection = NULL;
        }
    }

    return CY_RSLT_SUCCESS;
}

cy_rslt_t cy_ota_mqtt_report_result(cy_ota_context_t *ctx, cy_rslt_t last_error)
{
    cy_ota_callback_results_t   cb_result;
    cy_rslt_t                   result = CY_RSLT_SUCCESS;

    sprintf(ctx->mqtt.json_doc, CY_OTA_MQTT_RESULT_JSON,
            ( (last_error == CY_RSLT_SUCCESS) ? CY_OTA_RESULT_SUCCESS : CY_OTA_RESULT_FAILURE),
            ctx->mqtt.unique_topic);

    cy_log_msg(CYLF_OTA, CY_LOG_DEBUG2, "%d : %s() CALLING CB STATE_CHANGE %s stop_OTA_session:%d\n", __LINE__, __func__,
            cy_ota_get_state_string(ctx->curr_state), ctx->stop_OTA_session);

    cb_result = cy_ota_internal_call_cb(ctx, CY_OTA_REASON_STATE_CHANGE, ctx->curr_state);
    switch( cb_result )
    {
    default:
    case CY_OTA_CB_RSLT_OTA_CONTINUE:
        result = cy_ota_mqtt_publish_request(ctx, (char *)SUBSCRIBER_PUBLISH_TOPIC, ctx->mqtt.json_doc);
        break;
    case CY_OTA_CB_RSLT_OTA_STOP:
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() App returned OTA Stop for STATE_CHANGE for SEND_RESULT\n", __func__);
        result = CY_RSLT_OTA_ERROR_APP_RETURNED_STOP;
        break;
    case CY_OTA_CB_RSLT_APP_SUCCESS:
        cy_log_msg(CYLF_OTA, CY_LOG_INFO, "%s() App returned APP_SUCCESS for STATE_CHANGE for SEND_RESULT\n", __func__);
        break;
    case CY_OTA_CB_RSLT_APP_FAILED:
        cy_log_msg(CYLF_OTA, CY_LOG_ERR, "%s() App returned APP_FAILED for STATE_CHANGE for SEND_RESULT\n", __func__);
        result = CY_RSLT_OTA_ERROR_SENDING_RESULT;
        break;
    case CY_OTA_CB_NUM_RESULTS:
        result = CY_RSLT_OTA_ERROR_SENDING_RESULT;
        break;
    }

    return result;
}

#endif  /* COMPONENT_OTA_MQTT */
