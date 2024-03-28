/*
 * Copyright 2024, Cypress Semiconductor Corporation (an Infineon company) or
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

/*
 *  Cypress OTA Agent network abstraction for HTTP
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "cy_ota_api.h"

#ifdef COMPONENT_OTA_HTTP

#include "cy_ota_internal.h"
#include "cy_http_client_api.h"

#include "cyabs_rtos.h"
#include "cy_ota_log.h"


/***********************************************************************
 *
 * HTTP network Functions
 *
 **********************************************************************/


/***********************************************************************
 *
 * defines & enums
 *
 **********************************************************************/

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

/***********************************************************************
 *
 * Data & Variables
 *
 **********************************************************************/

#define HTTP_HEADER_STR                 "HTTP/"
#define HTTP_HEADERS_BODY_SEPARATOR     "\r\n\r\n"

#define HTTP_HEADER_CONTENT_TYPE        "Content-Type"      /* for JOB, it is HTTP_HEADER_CONTENT_TYPE_JOB_VALUE */
#define HTTP_HEADER_CONTENT_LENGTH      "Content-Length"    /* This is different for each Job, don't enforce size */

#define HTTP_HEADER_ACCEPT_RANGE        "Accept-Ranges"     /* We are only looking for bytes of data */
#define HTTP_HEADER_CONTENT_RANGE       "Content-Range"     /* The range will change - look for response values */

/* For the Job Document, we want to see these values */
#define HTTP_HEADER_CONTENT_ACCEPT_RANGE_VALUE      "bytes"
#define HTTP_HEADER_CONTENT_TYPE_JOB_VALUE          "application/json"
#define HTTP_HEADER_CONTENT_TYPE_DATA_VALUE         "text/plain"
#define HTTP_HEADER_CONTENT_RANGE_VALUE             "bytes"

/* For the Data, something a bit different */
#define CY_HTTP_MAX_HEADERS         10
#define CY_HTTP_HEADER_VALUE_LEN    32

static cy_http_client_header_t cy_ota_http_job_headers[] =
{
#ifdef CY_OTA_LIB_DEBUG_LOGS /* Define for debugging */
    { HTTP_HEADER_CONTENT_TYPE, sizeof(HTTP_HEADER_CONTENT_TYPE) - 1,                 // Do all servers send w/same type?
            HTTP_HEADER_CONTENT_TYPE_JOB_VALUE, sizeof(HTTP_HEADER_CONTENT_TYPE_JOB_VALUE) - 1 },
#endif
    { HTTP_HEADER_ACCEPT_RANGE, sizeof(HTTP_HEADER_ACCEPT_RANGE) - 1,
            HTTP_HEADER_CONTENT_ACCEPT_RANGE_VALUE, sizeof(HTTP_HEADER_CONTENT_ACCEPT_RANGE_VALUE) - 1 },
};
#define CY_NUM_JOB_HEADERS ( sizeof(cy_ota_http_job_headers) / sizeof(cy_http_client_header_t) )

static cy_http_client_header_t cy_ota_http_data_headers[] =
{
#ifdef CY_OTA_LIB_DEBUG_LOGS /* Define for debugging */
    { HTTP_HEADER_CONTENT_TYPE, sizeof(HTTP_HEADER_CONTENT_TYPE) - 1,                 // Do all servers send w/same type?
            HTTP_HEADER_CONTENT_TYPE_DATA_VALUE, sizeof(HTTP_HEADER_CONTENT_TYPE_DATA_VALUE) - 1 },
#endif
    { HTTP_HEADER_ACCEPT_RANGE, sizeof(HTTP_HEADER_ACCEPT_RANGE) - 1,
            HTTP_HEADER_CONTENT_ACCEPT_RANGE_VALUE, sizeof(HTTP_HEADER_CONTENT_ACCEPT_RANGE_VALUE) - 1 },

#ifdef CY_OTA_LIB_DEBUG_LOGS /* Define for debugging */
    { HTTP_HEADER_CONTENT_RANGE, sizeof(HTTP_HEADER_CONTENT_RANGE) - 1,
        HTTP_HEADER_CONTENT_RANGE_VALUE, sizeof(HTTP_HEADER_CONTENT_RANGE_VALUE) - 1 },
#endif
};
#define CY_NUM_DATA_HEADERS ( sizeof(cy_ota_http_data_headers) / sizeof(cy_http_client_header_t) )

static char cy_ota_http_read_values[CY_HTTP_MAX_HEADERS][CY_HTTP_HEADER_VALUE_LEN];

static cy_http_client_header_t cy_ota_http_read_headers[] =
{
    { HTTP_HEADER_CONTENT_TYPE, sizeof(HTTP_HEADER_CONTENT_TYPE) - 1,
      cy_ota_http_read_values[0], CY_HTTP_HEADER_VALUE_LEN },

    { HTTP_HEADER_CONTENT_LENGTH, sizeof(HTTP_HEADER_CONTENT_LENGTH) - 1,
      cy_ota_http_read_values[1], CY_HTTP_HEADER_VALUE_LEN },

    { HTTP_HEADER_CONTENT_RANGE, sizeof(HTTP_HEADER_CONTENT_RANGE) - 1,
      cy_ota_http_read_values[2], CY_HTTP_HEADER_VALUE_LEN },

    { HTTP_HEADER_ACCEPT_RANGE, sizeof(HTTP_HEADER_ACCEPT_RANGE) - 1,
      cy_ota_http_read_values[3], CY_HTTP_HEADER_VALUE_LEN },
};
#define CY_NUM_READ_HEADERS ( sizeof(cy_ota_http_read_headers) / sizeof(cy_http_client_header_t) )

static cy_http_client_header_t cy_ota_http_result_headers[] =
{
    { HTTP_HEADER_CONTENT_TYPE, sizeof(HTTP_HEADER_CONTENT_TYPE) - 1,
            HTTP_HEADER_CONTENT_TYPE_JOB_VALUE, sizeof(HTTP_HEADER_CONTENT_TYPE_JOB_VALUE) - 1 },

};
#define CY_NUM_RESULT_HEADERS ( sizeof(cy_ota_http_result_headers) / sizeof(cy_http_client_header_t) )

/***********************************************************************
 *
 * Forward declarations
 *
 **********************************************************************/

/***********************************************************************
 *
 * Functions
 *
 **********************************************************************/

static void cy_ota_http_timer_callback(cy_timer_callback_arg_t arg)
{
    cy_ota_context_t *ctx = (cy_ota_context_t *)arg;
    CY_OTA_CONTEXT_ASSERT(ctx);

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() new event:%d\n", __func__, ctx->http.http_timer_event);
    /* yes, we set the ota_event as the http get() function uses the same event var */
    cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)ctx->http.http_timer_event, 0);
}

static cy_rslt_t cy_ota_stop_http_timer(cy_ota_context_t *ctx)
{
    CY_OTA_CONTEXT_ASSERT(ctx);
    return cy_rtos_stop_timer(&ctx->http.http_timer);
}

static cy_rslt_t cy_ota_start_http_timer(cy_ota_context_t *ctx, uint32_t secs, ota_events_t event)
{
    cy_rslt_t result;
    uint32_t    num_ms = SECS_TO_MILLISECS(secs);

    CY_OTA_CONTEXT_ASSERT(ctx);

    cy_ota_stop_http_timer(ctx);
    ctx->http.http_timer_event = event;
    result = cy_rtos_start_timer(&ctx->http.http_timer, num_ms);
    return result;
}

/*************************************************************/
static cy_rslt_t cy_ota_http_init_headers(cy_ota_context_t *ctx,
                                            cy_http_client_header_t **send_headers, uint16_t *num_send_headers,
                                            cy_http_client_header_t **read_headers, uint16_t *num_read_headers)
{
    CY_OTA_CONTEXT_ASSERT(ctx);

    if( (send_headers == NULL) || (num_send_headers == NULL) ||
         (read_headers == NULL) || (num_read_headers == NULL) )

    {
        return CY_RSLT_OTA_ERROR_BADARG;
    }

    if(ctx->curr_state == CY_OTA_STATE_JOB_DOWNLOAD)
    {
        /* For Jobs Headers */
        *send_headers = cy_ota_http_job_headers;
        *num_send_headers = CY_NUM_JOB_HEADERS;
    }
    else if(ctx->curr_state == CY_OTA_STATE_DATA_DOWNLOAD)
    {
        /* For Data Headers */
        *send_headers = cy_ota_http_data_headers;
        *num_send_headers = CY_NUM_DATA_HEADERS;
    }
    else if(ctx->curr_state == CY_OTA_STATE_RESULT_SEND)
    {
        /* For Data Headers */
        *send_headers = cy_ota_http_result_headers;
        *num_send_headers = CY_NUM_RESULT_HEADERS;
    }
    else
    {
        return CY_RSLT_OTA_ERROR_GENERAL;
    }

    *read_headers = cy_ota_http_read_headers;
    *num_read_headers = CY_NUM_READ_HEADERS;


    return CY_RSLT_SUCCESS;
}

/****************************************************************/

/* This section is only used for debugging, helps determine HTTP header information */
#ifdef CY_OTA_LIB_DEBUG_LOGS
typedef enum
{
    HTTP_CONTINUE                        = 100,
    HTTP_SWITCHING_PROTOCOLS             = 101,
    HTTP_RESPONSE_OK                     = 200,
    HTTP_CREATED                         = 201,
    HTTP_ACCEPTED                        = 202,
    HTTP_NONAUTHORITATIVE                = 203,
    HTTP_NO_CONTENT                      = 204,
    HTTP_RESET_CONTENT                   = 205,
    HTTP_PARTIAL_CONTENT                 = 206,
    HTTP_MULTIPLE_CHOICES                = 300,
    HTTP_MOVED_PERMANENTLY               = 301,
    HTTP_FOUND                           = 302,
    HTTP_SEE_OTHER                       = 303,
    HTTP_NOT_MODIFIED                    = 304,
    HTTP_USEPROXY                        = 305,
    HTTP_TEMPORARY_REDIRECT              = 307,
    HTTP_BAD_REQUEST                     = 400,
    HTTP_UNAUTHORIZED                    = 401,
    HTTP_PAYMENT_REQUIRED                = 402,
    HTTP_FORBIDDEN                       = 403,
    HTTP_NOT_FOUND                       = 404,
    HTTP_METHOD_NOT_ALLOWED              = 405,
    HTTP_NOT_ACCEPTABLE                  = 406,
    HTTP_PROXY_AUTHENTICATION_REQUIRED   = 407,
    HTTP_REQUEST_TIMEOUT                 = 408,
    HTTP_CONFLICT                        = 409,
    HTTP_GONE                            = 410,
    HTTP_LENGTH_REQUIRED                 = 411,
    HTTP_PRECONDITION_FAILED             = 412,
    HTTP_REQUESTENTITYTOOLARGE           = 413,
    HTTP_REQUESTURITOOLONG               = 414,
    HTTP_UNSUPPORTEDMEDIATYPE            = 415,
    HTTP_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
    HTTP_EXPECTATION_FAILED              = 417,
    HTTP_INTERNAL_SERVER_ERROR           = 500,
    HTTP_NOT_IMPLEMENTED                 = 501,
    HTTP_BAD_GATEWAY                     = 502,
    HTTP_SERVICE_UNAVAILABLE             = 503,
    HTTP_GATEWAY_TIMEOUT                 = 504,
    HTTP_VERSION_NOT_SUPPORTED           = 505,
} http_status_code_t;

/**
 * Length limited version of strstr. Ported from wiced_lib.c
 *
 * @param s[in]             : The string to be searched.
 * @param s_len[in]         : The length of the string to be searched.
 * @param substr[in]        : The string to be found.
 * @param substr_len[in]    : The length of the string to be found.
 *
 * @return    pointer to the found string if search successful, otherwise NULL
 */
static char* strnstrn(const char *s, uint16_t s_len, const char *substr, uint16_t substr_len)
{
    for(; s_len >= substr_len; s++, s_len--)
    {
        if(strncmp(s, substr, substr_len) == 0)
        {
            return (char*)s;
        }
    }

    return NULL;
}

/* This function is not used, but useful for debugging */
cy_rslt_t cy_ota_http_parse_header(uint8_t **ptr, uint16_t *data_len, uint32_t *file_len, http_status_code_t *response_code)
{
    char    *response_status;
    uint8_t *header_end;

    if( (ptr == NULL) || (*ptr == NULL) ||
         (data_len == NULL) || (*data_len == 0) ||
         (file_len == NULL) ||
         (response_code == NULL) )
    {
        return CY_RSLT_OTA_ERROR_BADARG;
    }
    *response_code = HTTP_FORBIDDEN;

    /* example:
        "HTTP/1.1 200 Ok\r\n"
        "Server: mini_httpd/1.23 28Dec2015\r\n"
        "Date: Tue, 03 Mar 2020 18:49:23 GMT\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: 830544\r\n"
        "\r\n\r\n"
     */

    /* sanity check */
    if(*data_len < 12)
    {
        return CY_RSLT_OTA_ERROR_NOT_A_HEADER;
    }

    /* Find the HTTP/x.x part*/
    response_status = strnstrn( (char *)*ptr, *data_len, HTTP_HEADER_STR, sizeof(HTTP_HEADER_STR) - 1);
    if(response_status == NULL)
    {
        return CY_RSLT_OTA_ERROR_NOT_A_HEADER;
    }
    /* skip to next ' ' space character */
    response_status = strchr(response_status, ' ');
    if(response_status == NULL)
    {
        return CY_RSLT_OTA_ERROR_NOT_A_HEADER;
    }
    *response_code = (http_status_code_t)atoi(response_status + 1);

    /* Find Content-Length part*/
    response_status = strnstrn( (char *)*ptr, *data_len, HTTP_HEADER_CONTENT_LENGTH, sizeof(HTTP_HEADER_CONTENT_LENGTH) - 1);
    if(response_status == NULL)
    {
        return CY_RSLT_OTA_ERROR_NOT_A_HEADER;
    }
    response_status = &response_status[sizeof(HTTP_HEADER_CONTENT_LENGTH)];
    *file_len = atoi(response_status);

    /* find end of header */
    header_end = (uint8_t *)strnstrn( (char *)*ptr, *data_len, HTTP_HEADERS_BODY_SEPARATOR, sizeof(HTTP_HEADERS_BODY_SEPARATOR) - 1);
    if(header_end == NULL)
    {
        return CY_RSLT_OTA_ERROR_NOT_A_HEADER;
    }
    header_end = &header_end[sizeof(HTTP_HEADERS_BODY_SEPARATOR) - 1];
    *data_len -= (header_end - *ptr);
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG3, "Move ptr from %p to %p skipping %d new_len:%d first_byte:0x%x\n", *ptr, header_end, (header_end - *ptr), *data_len, *header_end);

    *ptr = header_end;
    return CY_RSLT_SUCCESS;
}
#endif /* For debugging */

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
cy_rslt_t cy_ota_http_validate_network_params(const cy_ota_network_params_t *network_params)
{
    if(network_params == NULL)
    {
        return CY_RSLT_OTA_ERROR_BADARG;
    }

    if( (network_params->http.server.host_name == NULL) ||
         (network_params->http.server.port == 0)  ||
         (network_params->http.file == NULL)  )
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_WARNING, "Missing HTTP args: host:%s:%d file:%s\n",
                    network_params->http.server.host_name , network_params->http.server.port,
                    network_params->http.file);
        return CY_RSLT_OTA_ERROR_BADARG;
    }
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Validated HTTP args: host:%s:%d file:%s\n",
                network_params->http.server.host_name , network_params->http.server.port,
                network_params->http.file);
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
static cy_rslt_t cy_ota_http_write_chunk_to_flash(cy_ota_context_t *ctx, cy_ota_storage_write_info_t *chunk_info)
{
    cy_ota_callback_results_t cb_result;

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "%s()\n", __func__);

    if( (ctx == NULL) || (chunk_info == NULL) )
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() Bad args\n", __func__);
        return CY_RSLT_OTA_ERROR_BADARG;
    }

    ctx->ota_storage_context.num_packets_received++;    /* this is so we don't have a false failure with the per packet timer */
    chunk_info->packet_number = ctx->ota_storage_context.num_packets_received;

    /* store the chunk for temporary use in the callback */
    ctx->storage = chunk_info;
    cb_result = cy_ota_internal_call_cb(ctx, CY_OTA_REASON_STATE_CHANGE, CY_OTA_STATE_STORAGE_WRITE);
    switch( cb_result )
    {
        default:
        /* Fall through */
        case CY_OTA_CB_RSLT_OTA_CONTINUE:
            if(ctx->storage_iface.ota_file_write(&(ctx->ota_storage_context), chunk_info) != CY_RSLT_SUCCESS)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() Write failed\n", __func__);
                cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_DATA_FAIL, 0);
                return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
            }
            break;

        case CY_OTA_CB_RSLT_OTA_STOP:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%d: %s() App returned OTA Stop for STATE_CHANGE for OTA platform storage block write API\n", __LINE__, __func__);
            return CY_RSLT_OTA_ERROR_APP_RETURNED_STOP;

        case CY_OTA_CB_RSLT_APP_SUCCESS:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "%d: %s() App returned APP_SUCCESS for STATE_CHANGE for OTA platform storage block write API\n", __LINE__, __func__);
            break;

        case CY_OTA_CB_RSLT_APP_FAILED:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%d: %s() App returned APP_FAILED for STATE_CHANGE for OTA platform storage block write API\n", __LINE__, __func__);
            return CY_RSLT_OTA_ERROR_WRITE_STORAGE;
    }

    /* update the stats */
    ctx->ota_storage_context.total_bytes_written   += chunk_info->size;
    ctx->ota_storage_context.last_offset            = chunk_info->offset;
    ctx->ota_storage_context.last_size              = chunk_info->size;
    ctx->ota_storage_context.last_packet_received   = chunk_info->packet_number;
    ctx->ota_storage_context.total_packets          = chunk_info->total_packets;

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "Written to offset:%ld  %ld of %ld (%ld remaining)\n",
                   ctx->ota_storage_context.last_offset, ctx->ota_storage_context.total_bytes_written, ctx->ota_storage_context.total_image_size,
                   (ctx->ota_storage_context.total_image_size - ctx->ota_storage_context.total_bytes_written) );

    return CY_RSLT_SUCCESS;
}

/**
 * brief Provide an asynchronous notification of network closing
 *
 * Invoked when the network connection is closed.
 *
 * param[in] handle previously created cy_http_client handle
 */
static void cy_ota_http_disconnect_callback(cy_http_client_t handle, cy_http_client_disconn_type_t type, void *user_data)
{
    /* for compiler warnings */
    (void)handle;
    (void)type;
    (void)user_data;
    /* HTTP is now Synchronous.
     * The get_data loop does not check events anymore.
     * The cy_ota_http_send_get_response() will return an error
     * on a disconnect, so we do not need to do anything here.
     * Keep this callback to give extra debug.
     */
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, " HTTP disconnect callback");
}


/**
 * @brief Connect to OTA Update server
 *
 * NOTE: Individual Network Connection type will do whatever is necessary
 *      ex: MQTT
 *          - connect
 *          HTTP
 *          - connect
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_http_connect(cy_ota_context_t *ctx)
{
    cy_rslt_t                    result;
    cy_awsport_ssl_credentials_t *security = NULL;
    cy_awsport_server_info_t     *server_info;

    CY_OTA_CONTEXT_ASSERT(ctx);

    if(ctx->http.connection_established == true)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Already connected\n");
        return CY_RSLT_OTA_ALREADY_CONNECTED;
    }

    if(ctx->http.connection_from_app == true)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "Already connected by application\n");
        ctx->http.connection_established = true;
        return CY_RSLT_OTA_ALREADY_CONNECTED;
    }

    /* determine server info and credential info */
    server_info = &ctx->network_params.http.server;
    security = &ctx->network_params.http.credentials;

    /* If application changed job_doc we need to use the parsed info */
    if( (ctx->curr_state == CY_OTA_STATE_DATA_CONNECT) &&
         (ctx->network_params.use_get_job_flow == CY_OTA_JOB_FLOW)  &&
         (ctx->parsed_job.parse_result == CY_RSLT_OTA_CHANGING_SERVER) )
    {
        server_info = &ctx->parsed_job.broker_server;
        if(ctx->callback_data.credentials != NULL)
        {
            security = ctx->callback_data.credentials;
        }
    }

    /*
     * cy_http_client_create() assumes that you are setting up a TLS connection
     * if credentials are passed in. So we need to make sure that we are only passing
     * in credentials when we really want a TLS connection.
     */

    if( (ctx->network_params.use_get_job_flow == CY_OTA_JOB_FLOW) && (ctx->curr_state == CY_OTA_STATE_DATA_CONNECT) &&
         (ctx->parsed_job.connect_type != CY_OTA_CONNECTION_HTTPS) )
    {
        /* We are using a Job Flow, and we are starting a
         * Data connection after getting a Job Doc.
         * If the connect type is not HTTPS, clear credentials.
         */
        security = NULL;
    }

    if( ( (ctx->network_params.use_get_job_flow == CY_OTA_DIRECT_FLOW) || (ctx->curr_state != CY_OTA_STATE_DATA_CONNECT) ) &&
           (ctx->network_params.initial_connection != CY_OTA_CONNECTION_HTTPS) )
    {
        /* We are using a Direct Flow, and we are not starting a
         * Data connection.
         * If the connect type is not HTTPS, clear credentials.
         */
        security = NULL;
    }

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "call cy_http_client_init()\n");
    result = cy_http_client_init();
    if(result != CY_RSLT_SUCCESS)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_http_client_init() failed %d.\n", __func__, result);
        return CY_RSLT_OTA_ERROR_CONNECT;
    }

    /* create the client connection */
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%s() call cy_http_client_create()!! %s.\n", __func__,
                         (security == NULL) ? "non-TLS" : "TLS");
    result = cy_http_client_create(security,
                                 server_info,
                                 cy_ota_http_disconnect_callback,
                                 ctx,
                                 &ctx->http.connection);

    if(result != CY_RSLT_SUCCESS)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_http_client_create() failed %d.\n", __func__, result);
        cy_http_client_deinit();
        return CY_RSLT_OTA_ERROR_CONNECT;
    }
    result = cy_http_client_connect(ctx->http.connection, CY_OTA_HTTP_TIMEOUT_SEND, CY_OTA_HTTP_TIMEOUT_RECEIVE);
    if(result != CY_RSLT_SUCCESS)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_http_client_connect() failed %d.\n", __func__, result);
        cy_http_client_delete(&ctx->http.connection);
        cy_http_client_deinit();
        return CY_RSLT_OTA_ERROR_CONNECT;
    }

    ctx->http.connection_established = true;

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "HTTP Connection Successful, server:%s:%d  TLS:%s\n",
               (server_info->host_name == NULL) ? "None" : server_info->host_name, server_info->port,
               (security == NULL) ? "No" : "Yes");

    return CY_RSLT_SUCCESS;
}

/**
 * @brief send & get a single request/response
 *
 * @param[in]   ctx         - pointer to OTA agent context @ref cy_ota_context_t
 * @param[in]   request     - pointer to request header struct @ref cy_http_client_request_header_t
 * @param[in]   send_header - pointer to request header struct @ref cy_http_client_header_t
 * @param[in]   num_send_headers - number of headers in the header list
 * @param[in]   read_header - pointer to response header struct @ref cy_http_client_header_t
 * @param[in]   num_read_headers - number of headers in the header list
 * @param[in]   response    - pointer to client response struct @ref cy_http_client_response_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
static cy_rslt_t cy_ota_http_send_get_response(cy_ota_context_t *ctx,
                                                cy_http_client_request_header_t *request,
                                                cy_http_client_header_t         *send_headers,
                                                uint16_t                        num_send_headers,
                                                cy_http_client_header_t         *read_headers,
                                                uint16_t                        num_read_headers,
                                                cy_http_client_response_t       *response)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    uint16_t i;

    result = cy_http_client_write_header(ctx->http.connection, request, send_headers, num_send_headers);
    if(result != CY_RSLT_SUCCESS)
    {
         cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_http_client_write_header() Failed ret:0x%lx\n", __func__, result);
         result = CY_RSLT_OTA_ERROR_GENERAL;
    }
    else
    {
        result = cy_http_client_send(ctx->http.connection, request, NULL, 0, response);
        if( (result == CY_RSLT_HTTP_CLIENT_ERROR_NO_RESPONSE) && (ctx->curr_state == CY_OTA_STATE_RESULT_SEND) )
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "  When sending result (POST), treat NO_RESPONSE as SUCCESS, as server may not handle\n");
            result = CY_RSLT_SUCCESS;
        }
        else if(result != CY_RSLT_SUCCESS)
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_http_client_send() Failed ret:0x%lx\n", __func__, result);
            result = CY_RSLT_OTA_ERROR_GENERAL;
        }
        else
        {
            if(num_read_headers)
            {
                result = cy_http_client_read_header(ctx->http.connection, response, read_headers, num_read_headers);
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "cy_http_client_read_header(): result:0x%lx status:%d\n", result, response->status_code);

#ifdef CY_OTA_LIB_DEBUG_LOGS /* Define for debugging */
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "response->header: count: %ld\n", response->header_count);
                cy_ota_print_data( (const char *)response->header, response->headers_len);
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "response->body:%p sz:%d\n", response->body, response->body_len);
                cy_ota_print_data( (const char *)response->body, response->body_len);
#endif
                if(result != CY_RSLT_SUCCESS)
                {
                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_http_client_read_header() Failed ret:0x%lx\n", __func__, result);
                        result = CY_RSLT_OTA_ERROR_GENERAL;
                }
                else if(response->status_code < 100)
                {
                    /* do nothing here */
                    /* TODO: do we need to do anything or will cy_http_client_read_header() take care of it ? */
                    result = CY_RSLT_OTA_ERROR_GENERAL;
                }
                else if(response->status_code < 200)
                {
                    /* 1xx (Informational): The request was received, continuing process */
                    /* TODO: do we need to do anything or will cy_http_client_read_header() take care of it ? */
                    result = CY_RSLT_OTA_ERROR_GENERAL;
                }
                else if(response->status_code < 300)
                {
                    /* 2xx (Successful): The request was successfully received, understood, and accepted */
                    if(ctx->ota_storage_context.total_image_size == 0)
                    {
                        char *full_length = NULL;
                        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "%s() Parsed HTTP headers: %d\n", __func__, response->header_count);
                        for(i = 0; i < num_read_headers; i++)
                        {
                            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "read field[%d] %.*s\n", i, read_headers[i].field_len ,read_headers[i].field );
                            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "read value[%d] %.*s\n", i, read_headers[i].value_len ,read_headers[i].value );
                            if(strcmp(read_headers[i].field, HTTP_HEADER_CONTENT_RANGE) == 0)
                            {
                                if(read_headers[i].value_len == 0)
                                {
                                    /* did not get a "Content-Range" header */
                                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_WARNING, "%s() Content-Range did not have a value!\n", __func__);
                                    result = CY_RSLT_OTA_ERROR_GENERAL;
                                    break;
                                }
                                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "Content-Range value: %.*s\n", read_headers[i].value_len ,read_headers[i].value );

                                /* We expect "bytes 0-xxxx/<full_size>" */
                                full_length = strstr(read_headers[i].value, "/");
                                if(full_length != NULL)
                                {
                                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "full-length 1: %s\n", full_length );
                                    /* skip past the "/" if we found it */
                                    full_length++;
                                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "full-length 2: %s\n", full_length );
                                    ctx->ota_storage_context.total_image_size = atoi(full_length);
                                }
                                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "%s() HTTP File Length: %d\n", __func__, ctx->ota_storage_context.total_image_size);
                            }
                        }
                    }
                    result = CY_RSLT_SUCCESS;
                }
                else if(response->status_code < 400)
                {
                    /* 3xx (Redirection): Further action needs to be taken in order to complete the request */
                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "HTTP response code: %d, redirection - code needed to handle this!\n", response->status_code);
                    result = CY_RSLT_OTA_ERROR_GENERAL;
                }
                else
                {
                    /* 4xx (Client Error): The request contains bad syntax or cannot be fulfilled */
                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "HTTP response code: %d, ERROR!\n", response->status_code);
                    result = CY_RSLT_OTA_ERROR_GENERAL;
                }
            }
        }
    }
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "%s() returning: 0x%lx\n", __func__, result);

    return result;
}

/**
 * @brief get the OTA job
 *
 * NOTE: Individual Network Connection type will do whatever is necessary
 *       HTTP
 *          - pull the data from the server
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_http_get_job(cy_ota_context_t *ctx)
{
    cy_rslt_t       result = CY_RSLT_SUCCESS;
    cy_ota_callback_results_t   cb_result;

    CY_OTA_CONTEXT_ASSERT(ctx);

    /* Form GET request - re-use data buffer to save some RAM */
    memset(ctx->http.file, 0x00, sizeof(ctx->http.file));
    memset(ctx->http.json_doc, 0x00, sizeof(ctx->http.json_doc));
    if(ctx->network_params.use_get_job_flow == CY_OTA_DIRECT_FLOW)
    {
        /* Caller gave us the file name directly - use what is in params */
        strncpy(ctx->http.file, ctx->network_params.http.file, (sizeof(ctx->http.file) - 1) );
        if(strlen(ctx->http.file) < 1)
        {
            size_t copy_max = sizeof(ctx->http.file) - 1;
            if(strlen(CY_OTA_HTTP_DATA_FILE) < copy_max)
            {
                copy_max = strlen(CY_OTA_HTTP_DATA_FILE) + 1; /* include null at end of string */
            }
            strncpy(ctx->http.file, CY_OTA_HTTP_DATA_FILE, copy_max );
        }
        snprintf(ctx->http.json_doc, sizeof(ctx->http.json_doc) - 1, CY_OTA_HTTP_GET_TEMPLATE,
                ctx->http.file, ctx->curr_server->host_name, ctx->curr_server->port);
    }
    else
    {
        /* We got the file name from the Job file.
         * Caller gave us the file name directly - use what is in params
         */
        if(strlen(ctx->http.file) < 1)
        {
            if( strlen(CY_OTA_HTTP_JOB_FILE) > (sizeof(ctx->http.file) - 1))
            {
                    strncpy(ctx->http.file, CY_OTA_HTTP_JOB_FILE, (sizeof(ctx->http.file) - 1) );
            }
            else
            {
                strcpy(ctx->http.file, CY_OTA_HTTP_JOB_FILE);
            }
        }
        snprintf(ctx->http.json_doc, sizeof(ctx->http.json_doc), CY_OTA_HTTP_GET_TEMPLATE,
                ctx->http.file, ctx->curr_server->host_name, ctx->curr_server->port);
    }

#ifdef CY_OTA_LIB_DEBUG_LOGS /* Define for debugging */
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "HTTP Get Job \n");
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "File before cb: %s\n", ctx->http.file);
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "json_doc before cb: %d:%s\n", strlen(ctx->http.json_doc), ctx->http.json_doc);
#endif

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%d : %s() CALLING CB STATE_CHANGE %s stop_OTA_session:%d\n", __LINE__, __func__,
            cy_ota_get_state_string(ctx->curr_state), ctx->stop_OTA_session);

    cb_result = cy_ota_internal_call_cb(ctx, CY_OTA_REASON_STATE_CHANGE, ctx->curr_state);

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "HTTP Get Job        cb result: 0x%lx\n", cb_result);
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "                File After cb: %s\n", ctx->http.file);
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "            json_doc After cb: %d:%s\n", strlen(ctx->http.json_doc), ctx->http.json_doc);

    switch( cb_result )
    {
        default:
        case CY_OTA_CB_RSLT_OTA_CONTINUE:
            {
                cy_http_client_request_header_t request;                    // move into ctx structure ?
                cy_http_client_header_t         *send_headers = NULL;
                uint16_t                        num_send_headers=0;     /* Number of headers in the list */
                cy_http_client_response_t       response;               // move into ctx structure ?
                cy_http_client_header_t         *read_headers = NULL;
                uint16_t                        num_read_headers=0;     /* Number of headers in the list */

                request.method        = CY_HTTP_CLIENT_METHOD_GET;
                request.resource_path = ctx->http.file;                 /* File to load */
                request.buffer        = (uint8_t*)ctx->http.json_doc;   /* Location to store returned data */
                request.buffer_len    = sizeof(ctx->http.json_doc);     /* size of buffer */
                request.headers_len   = 0;                              /* filled in by cy_http_client_write_header() */
                request.range_start   = 0;                              /* the job is reasonably small, get the whole thing */
                request.range_end     = -1;

                /* fill headers we want to see in the response */
                if(cy_ota_http_init_headers(ctx, &send_headers, &num_send_headers, &read_headers, &num_read_headers) != CY_RSLT_SUCCESS)
                {
                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "cy_ota_http_init_headers() failed for state: %s\n", cy_ota_get_state_string(ctx->curr_state));
                }

                memset(&response, 0x00, sizeof(response));

                result = cy_ota_http_send_get_response(ctx, &request,
                                                        send_headers, num_send_headers,
                                                        read_headers, num_read_headers,
                                                        &response);
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "cy_ota_http_send_get_response() returned: 0x%lx status:%d\n", result, response.status_code);
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "  Buffer:%p   len:%d\n", response.body, response.body_len);
                if(result != CY_RSLT_SUCCESS)
                {
                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "cy_ota_http_send_get_response() returned: 0x%lx\n", result);
                    result = CY_RSLT_OTA_ERROR_GET_JOB;
                    break;
                }
                else
                {
                    uint32_t     copy_len;
                    /* Copy the Job document into the buffer. We will parse the job document in cy_ota_agent.c */
                    memset(ctx->job_doc, 0x00, sizeof(ctx->job_doc) );
                    copy_len = (response.body_len < sizeof(ctx->job_doc)) ? response.body_len : (sizeof(ctx->job_doc) - 1);
                    memcpy(ctx->job_doc, response.body, copy_len); //http.json_do ??

#ifdef CY_OTA_LIB_DEBUG_LOGS /* Define for debugging */
                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "response.body:%p sz:%d\n", response.body, response.body_len);
                    cy_ota_print_data( (const char *)response.body, response.body_len);

                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "JOB DOC     :%p sz:%d\n", ctx->job_doc, copy_len);
                    cy_ota_print_data( ctx->job_doc, copy_len);
#endif
                    result = CY_RSLT_SUCCESS;
                }
            }
            break;

        case CY_OTA_CB_RSLT_OTA_STOP:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() App returned OTA Stop for STATE_CHANGE for JOB_DOWNLOAD\n", __func__);
            result = CY_RSLT_OTA_ERROR_GET_JOB;
            break;

        case CY_OTA_CB_RSLT_APP_SUCCESS:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "%s() App returned APP_SUCCESS for STATE_CHANGE for JOB_DOWNLOAD\n", __func__);
            result = CY_RSLT_SUCCESS;
            break;

        case CY_OTA_CB_RSLT_APP_FAILED:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() App returned APP_FAILED for STATE_CHANGE for JOB_DOWNLOAD\n", __func__);
            result = CY_RSLT_OTA_ERROR_GET_JOB;
            break;
    }

    return result;
}


/**
 * @brief get the OTA download
 *
 * NOTE: Individual Network Connection type will do whatever is necessary
 *      ex: MQTT
 *          - subscribe to start data transfer
 *          HTTP
 *          - pull the data from the server
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_http_get_data(cy_ota_context_t *ctx)
{
    /* static so it is not on the stack */
    static cy_ota_storage_write_info_t     http_chunk_info;
    cy_rslt_t       result;
    uint32_t        waitfor_clear;
    uint32_t        range_start;
    uint32_t        range_end;

    cy_ota_callback_results_t   cb_result;

    CY_OTA_CONTEXT_ASSERT(ctx);

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "%s()\n", __func__);

    if(cy_rtos_init_mutex(&ctx->sub_callback_mutex) != CY_RSLT_SUCCESS)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() sub_callback_mutex init failed\n", __func__);
        return CY_RSLT_OTA_ERROR_GET_DATA;
    }
    ctx->sub_callback_mutex_inited = 1;

    /* clear any lingering events */
    waitfor_clear = CY_OTA_EVENT_HTTP_EVENTS;
    result = cy_rtos_waitbits_event(&ctx->ota_event, &waitfor_clear, 1, 0, 1);
    if(waitfor_clear != 0)
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "%s() Clearing waitfor: 0x%lx\n", __func__, waitfor_clear);
    }

    result = cy_rtos_init_timer(&ctx->http.http_timer, CY_TIMER_TYPE_ONCE,
                        cy_ota_http_timer_callback, (cy_timer_callback_arg_t)ctx);
    if(result != CY_RSLT_SUCCESS)
    {
        /* Timer init failed */
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() Timer Create Failed!\n", __func__);
        ctx->sub_callback_mutex_inited = 0;
        cy_rtos_deinit_mutex(&ctx->sub_callback_mutex);
        return CY_RSLT_OTA_ERROR_GET_DATA;
    }

    /* start with first chunk of data */
    range_start = 0;
    range_end = CY_OTA_CHUNK_SIZE - 1;      /* end byte, not length ! */

    /* Form GET request - re-use data buffer to save some RAM */
    memset(ctx->http.file, 0x00, sizeof(ctx->http.file));
    memset(ctx->http.json_doc, 0x00, sizeof(ctx->http.json_doc));
    if(ctx->network_params.use_get_job_flow == CY_OTA_DIRECT_FLOW)
    {
        /* Caller gave us the file name directly - use what is params */
        strncpy(ctx->http.file, ctx->network_params.http.file, (sizeof(ctx->http.file) - 1) );
        snprintf(ctx->http.json_doc, sizeof(ctx->http.json_doc), CY_OTA_HTTP_GET_RANGE_TEMPLATE,
                ctx->http.file, ctx->curr_server->host_name, ctx->curr_server->port,
                (long)range_start, (long)range_end);
    }
    else
    {
        /* We got the file name from the Job file.
         * The Job redirect will have already changed the current server */
        strncpy(ctx->http.file, ctx->parsed_job.file, (sizeof(ctx->http.file) - 1) );
        snprintf(ctx->http.json_doc, sizeof(ctx->http.json_doc), CY_OTA_HTTP_GET_RANGE_TEMPLATE,
                ctx->parsed_job.file, ctx->curr_server->host_name, ctx->curr_server->port,
                (long)range_start, (long)range_end);
    }
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "%d : %s() CALLING CB STATE_CHANGE %s stop_OTA_session:%d\n", __LINE__, __func__,
            cy_ota_get_state_string(ctx->curr_state), ctx->stop_OTA_session);

    cb_result = cy_ota_internal_call_cb(ctx, CY_OTA_REASON_STATE_CHANGE, ctx->curr_state);
    switch( cb_result )
    {
        default:
        case CY_OTA_CB_RSLT_OTA_STOP:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() App returned OTA Stop for STATE_CHANGE for DATA_DOWNLOAD\n", __func__);
            result = CY_RSLT_OTA_ERROR_APP_RETURNED_STOP;
            goto cleanup_and_exit;

        case CY_OTA_CB_RSLT_OTA_CONTINUE:
            /* Processing continues after switch() */
            break;

        case CY_OTA_CB_RSLT_APP_SUCCESS:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "%s() App returned APP_SUCCESS for STATE_CHANGE for DATA_DOWNLOAD\n", __func__);
            result = CY_RSLT_SUCCESS;
            goto cleanup_and_exit;

        case CY_OTA_CB_RSLT_APP_FAILED:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() App returned APP_FAILED for STATE_CHANGE for DATA_DOWNLOAD\n", __func__);
            result = CY_RSLT_OTA_ERROR_GET_DATA;
            goto cleanup_and_exit;
    }

    /* we only get here when there are no errors in our setup above. */
    /* If we bail without doing anything here, then we need to look at getting the file size before
     * getting here.
     */
    while( ( (ctx->ota_storage_context.total_bytes_written == 0) ||
              (ctx->ota_storage_context.total_bytes_written < ctx->ota_storage_context.total_image_size) ) &&
            (range_end > range_start) )
    {
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "while(ctx->ota_storage_context.total_bytes_written (%ld) < (%ld) ctx->total_image_size)\n", ctx->ota_storage_context.total_bytes_written, ctx->ota_storage_context.total_image_size);
        /* Send a request and wait for a response
         * The response call has a timeout value, so when we call waitbits_event below
         * we do not use "forever".
         */
        cy_http_client_request_header_t request;                    // move into ctx structure ?
        cy_http_client_header_t         *send_headers = NULL;
        uint16_t                        num_send_headers=0;    /* Number of headers requested  */
        cy_http_client_header_t         *read_headers = NULL;
        uint16_t                        num_read_headers=0;    /* Number of headers requested  */
        cy_http_client_response_t       response;                   // move into ctx structure ?

        request.method        = CY_HTTP_CLIENT_METHOD_GET;
        request.resource_path = ctx->http.file;             /* Data file name */
        request.buffer        = ctx->chunk_buffer;          /* Location to store returned data */
        request.buffer_len    = sizeof(ctx->chunk_buffer);  /* size of buffer */
        request.headers_len   = 0;                          /* filled in by cy_http_client_write_header() */
        request.range_start   = range_start;                /* start offset for this chunk */
        request.range_end     = range_end;                  /* bytes to transfer this loop */

        /* fill headers we want to see in the response */
        if(cy_ota_http_init_headers(ctx, &send_headers, &num_send_headers, &read_headers, &num_read_headers) != CY_RSLT_SUCCESS)
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "cy_ota_http_init_headers() failed for state: %s\n", cy_ota_get_state_string(ctx->curr_state));
        }

        memset(&response, 0x00, sizeof(response));

        result = cy_ota_http_send_get_response(ctx, &request,
                                                send_headers, num_send_headers,
                                                read_headers, num_read_headers,
                                                &response);

        if(result == CY_RSLT_SUCCESS)
        {
            /* set parameters for writing */
            http_chunk_info.offset     = ctx->ota_storage_context.total_bytes_written;
            http_chunk_info.buffer     = (uint8_t *)response.body;
            http_chunk_info.size       = response.body_len;
            http_chunk_info.total_size = ctx->ota_storage_context.total_image_size;  // is this correct? Is it set?

            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "call cy_ota_http_write_chunk_to_flash(%p %d)\n", http_chunk_info.buffer, http_chunk_info.size);
            result = cy_ota_http_write_chunk_to_flash(ctx, &http_chunk_info);
            if(result == CY_RSLT_OTA_ERROR_APP_RETURNED_STOP)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_WARNING, "%s() cy_ota_storage_write() returned OTA_STOP 0x%lx\n", __func__, result);
            }
            else if(result != CY_RSLT_SUCCESS)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_ota_storage_write() failed 0x%lx\n", __func__, result);
                result = CY_RSLT_OTA_ERROR_WRITE_STORAGE;
                break;  // drop out of while loop
            }
            else
            {
                result = CY_RSLT_SUCCESS;
            }
        }
        else
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "cy_ota_http_send_get_response() ret:0x%lx start:0x%lx =  0x%lx\n", result, request.range_start, range_start);
            result = CY_RSLT_OTA_ERROR_GET_DATA;
            break;  // drop out of while loop
        }


        range_start = range_end + 1;
        range_end += CY_OTA_CHUNK_SIZE;     /* end, not length */
        if(range_end > ctx->ota_storage_context.total_image_size)
        {
            range_end = ctx->ota_storage_context.total_image_size - 1;
        }
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "After :: range_start: 0x%lx  end: 0x%lx total_image_size:0x%lx\n", range_start, range_end, ctx->ota_storage_context.total_image_size);

        /* Check the timing between packets */
        if(ctx->packet_timeout_sec > 0 )
        {
            /* got some data - restart the download interval timer */
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "%s() RESTART PACKET TIMER %ld secs\n", __func__, ctx->packet_timeout_sec);
            cy_ota_start_http_timer(ctx, ctx->packet_timeout_sec, CY_OTA_EVENT_PACKET_TIMEOUT);
        }

        /* Check for finished getting data */
        if( (ctx->ota_storage_context.total_bytes_written > 0) &&
             (ctx->ota_storage_context.total_bytes_written >= ctx->ota_storage_context.total_image_size) )
        {
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "Done writing all data! %ld of %ld\n", ctx->ota_storage_context.total_bytes_written, ctx->ota_storage_context.total_image_size);
            cy_rtos_setbits_event(&ctx->ota_event, (uint32_t)CY_OTA_EVENT_DATA_DONE, 0);
            /* stop timer asap */
            cy_ota_stop_http_timer(ctx);
        }

    }   /* While not done loading */

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "%s() HTTP GET DATA DONE result: 0x%lx\n", __func__, result);

cleanup_and_exit:
    ctx->sub_callback_mutex_inited = 0;
    cy_rtos_deinit_mutex(&ctx->sub_callback_mutex);

    /* we completed the download, stop the timer */
    cy_ota_stop_http_timer(ctx);
    cy_rtos_deinit_timer(&ctx->http.http_timer);

    return result;
}

/**
 * @brief Disconnect from HTTP server
 *
 * @param[in]   ctx - pointer to OTA agent context @ref cy_ota_context_t
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_OTA_ERROR_GENERAL
 */
cy_rslt_t cy_ota_http_disconnect(cy_ota_context_t *ctx)
{
    CY_OTA_CONTEXT_ASSERT(ctx);

    /* Only disconnect if the Application did not pass in the connection */
    if(ctx->http.connection_from_app == false)
    {
        if(ctx->http.connection_established == true)
        {
            cy_http_client_t  old_conn;
            old_conn = ctx->http.connection;
            ctx->http.connection = NULL;

            /* Only disconnect if we had connected before */
            if(old_conn != NULL)
            {
                cy_http_client_disconnect(old_conn);
                cy_http_client_delete(old_conn);
            }
        }
        ctx->http.connection_established = false;
    }
    return CY_RSLT_SUCCESS;
}

cy_rslt_t cy_ota_http_report_result(cy_ota_context_t *ctx, cy_rslt_t last_error)
{
    cy_rslt_t                   result = CY_RSLT_SUCCESS;
    cy_ota_callback_results_t   cb_result;
    uint32_t                    buff_len;

    /* Create Result JSON Doc */
    sprintf(ctx->http.json_doc, CY_OTA_HTTP_RESULT_JSON,
            ( (last_error == CY_RSLT_SUCCESS) ? CY_OTA_RESULT_SUCCESS : CY_OTA_RESULT_FAILURE),
            ctx->http.file);

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "%d : %s() CALLING CB STATE_CHANGE %s stop_OTA_session:%d\n", __LINE__, __func__,
            cy_ota_get_state_string(ctx->curr_state), ctx->stop_OTA_session);

    cb_result = cy_ota_internal_call_cb(ctx, CY_OTA_REASON_STATE_CHANGE, ctx->curr_state);
    buff_len = strlen(ctx->http.json_doc);

    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "HTTP POST result     File After cb: %s\n", ctx->http.file);
    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "HTTP POST result json_doc After cb: %ld:%s\n",buff_len, ctx->http.json_doc);

    /* Form POST message for HTTP Server */

    /* Create Post Header */
    sprintf((char *)ctx->data_buffer, CY_OTA_HTTP_POST_TEMPLATE,
            ( (last_error == CY_RSLT_SUCCESS) ? CY_OTA_RESULT_SUCCESS : CY_OTA_RESULT_FAILURE),
            (long)buff_len, ctx->http.json_doc);
    buff_len = strlen((char *)ctx->data_buffer);

    switch( cb_result )
    {
        default:
        case CY_OTA_CB_RSLT_OTA_CONTINUE:
            {
                cy_http_client_request_header_t request;                    // move into ctx structure ?
                cy_http_client_header_t         *send_headers = NULL;
                uint16_t                        num_send_headers=0;        /* Number of headers in the list */
                cy_http_client_header_t         *read_headers = NULL;
                uint16_t                        num_read_headers=0;        /* Number of headers in the list */
                cy_http_client_response_t       response;                   // move into ctx structure ?

                request.method        = CY_HTTP_CLIENT_METHOD_POST;
                request.resource_path = ctx->http.file;                 /* OTA_HTTP_JOB_FILE ?? */
                request.buffer        = (uint8_t *)ctx->http.json_doc;  /* Location to store returned data */
                request.buffer_len    = sizeof(ctx->http.json_doc);     /* size of buffer */
                request.headers_len   = 0;                              /* filled in by cy_http_client_write_header() */
                request.range_start   = 0;                              /* the job is reasonably small, get the whole thing */
                request.range_end     = -1;

                /* fill headers we want to see in the response */
                if(cy_ota_http_init_headers(ctx, &send_headers, &num_send_headers, &read_headers, &num_read_headers) != CY_RSLT_SUCCESS)
                {
                    cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "cy_ota_http_init_headers() failed for state: %s\n", cy_ota_get_state_string(ctx->curr_state));
                }

                /* Some HTTP server may not support POST command, So just POST result and No need to read header response. */
                read_headers = NULL;
                num_read_headers = 0;

                memset(&response, 0x00, sizeof(response));
                result = cy_ota_http_send_get_response(ctx, &request,
                                                        send_headers, num_send_headers,
                                                        read_headers, num_read_headers,
                                                        &response);
            }
            if(result != CY_RSLT_SUCCESS)
            {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() cy_ota_http_send_get_response(len:0x%x) failed 0x%lx\n", __func__, buff_len, result);
                result = CY_RSLT_OTA_ERROR_SENDING_RESULT;
            }
            break;

        case CY_OTA_CB_RSLT_OTA_STOP:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() App returned OTA Stop for STATE_CHANGE for SEND_RESULT\n", __func__);
            result = CY_RSLT_OTA_ERROR_APP_RETURNED_STOP;
            break;

        case CY_OTA_CB_RSLT_APP_SUCCESS:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "%s() App returned APP_SUCCESS for STATE_CHANGE for SEND_RESULT\n", __func__);
            break;

        case CY_OTA_CB_RSLT_APP_FAILED:
            cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "%s() App returned APP_FAILED for STATE_CHANGE for SEND_RESULT\n", __func__);
            result = CY_RSLT_OTA_ERROR_SENDING_RESULT;
            break;
    }

    return result;
}

#endif /* COMPONENT_OTA_HTTP    */
