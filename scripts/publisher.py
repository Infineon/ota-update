import json
import paho.mqtt.client as mqtt
import os
import random
import signal
import struct
import sys
import threading
import time
import traceback
import re

random.seed()

#
#   This is the MQTT Publisher for OTA Update device updating.
#   It is intended to be used with an MQTT Broker that does NOT
#   have the ability to save OTA Images. This allows Devices
#   to connect at any time and request an update.
#
#   There are 2 "flows" for updating OTA Images
#
# Direct Flow:
#   - Publisher startup and Device startup can happen in any order.
#   - Publisher is listening for a request from the Device on PUBLISHER_DIRECT_TOPIC.
#   - Device creates and subscribes to a Unique Topic
#   - Sends update to Device on Unique Topic sent by device
#
#
# Job Flow:
#   - Publisher startup and Device startup can happen in any order.
#   - Publisher is listening for a request from the Device on PUBLISHER_JOB_REQUEST_TOPIC.
#   - Device creates and subscribes to a Unique Topic
#   - Device sends request to Publisher on PUBLISHER_JOB_REQUEST_TOPIC with Unique Topic
#
#
#   Publisher startup:
#   - Publisher connects to MQTT Broker.
#   - Publisher subscribes to PUBLISHER_JOB_REQUEST_TOPIC and PUBLISHER_DIRECT_TOPIC.
#
#   Publisher receives message on PUBLISHER_JOB_REQUEST_TOPIC:
#   - If message is UPDATE_AVAILABILITY
#       - Message contains a "unique topic", Manuf, Product, Board, Version, etc
#       Publisher checks if update is available based on message content
#       If Publisher has database for all the devices,
#           Publisher can check if Device is due an update.
#       If Update is Available,
#           Publisher creates a thread.
#           Publisher thread subscribes to unique_topic_name
#           Publisher thread publishes Job Document to unique_topic_name
#               - data in the Job Document contains information about the OTA Image
#                 available for downloading (version, board name, etc.).
#   - If message is DOWNLOAD_REQUEST
#           Publisher publishes the OTA Image on the new thread to the unique_topic_name.
#
#
#   Device startup:
#   - Device connects to MQTT Broker.
#   - Device creates UPDATE_AVAILABILITY message
#       - contains Manuf, product, board, version, etc.
#       - creates and includes a unique_topic_name
#   - Device subscribes to unique_topic_name
#   - Device sends UPDATE_AVAILABILITY to PUBLISHER_JOB_REQUEST_TOPIC.
#   - If there is no response on unique_topic_name,
#       Device will periodically re-send UPDATE_AVAILABILITY to PUBLISHER_JOB_REQUEST_TOPIC.
#
#   Device receives Job message on unique_topic_name:
#   - Job message contains version, board name, etc.
#   - If Device wants the update:
#       - Device sends a DOWNLOAD_REQUEST on the unique_topic_name
#
#   Device receives data chunks on unique_topic_name:
#   - Device stores OTA Image to FLASH
#   - When download is complete
#       - Device sends DOWNLOAD_RESULT to unique_topic_name.
#
#   Device receives DOWNLOAD_RESULT_RESPONSE on unique_topic_name
#       - Device sets flag for MCUBoot
#   - On Device reset, MCUBoot does copy from Secondary to Primary
#
# Update complete!
#
# Customizations:
# ---------------
#
#   Topic Strings:
#   NOTE: You can change the topic string to fit your needs. This must match in Device code as well!
#   NOTE: This Publisher is an example, and does not check for manuf. name, product, version, etc.
#
#   PUBLISHER_JOB_REQUEST_TOPIC  - Topic name for Publisher to listen for Device Update Available message.
#
#   "Update Availability" Message - Device sends to Publisher
#       sent on topic PUBLISHER_JOB_REQUEST_TOPIC
#
#    {
#       "Message":"Update Availability",
#       "Manufacturer": "<company name>",
#       "ManufacturerID": "<company ID>",
#       "ProductID": "<product ID>",
#       "SerialNumber": "<serial number>",
#       "BoardName": "CY8CPROTO_062_4343W",
#       "Version": "<software version>",
#       "UniqueTopicName": "<my unique topic>",
#       ... other info as desired ...
#   }
#
#   Job Message "No Update Available" - Publisher sends to Device
#       sent on topic <my unique topic>
#   '{
#       "Message": "No Update Available"
#   }'
#
#
# NOTE: Publisher can load a pre-defined JSON file or be more sophisticated and
#       create the JSON file on the fly.
#       For this example, we use the pre-made JSON file JSON_JOB_MESSAGE_FILE
#
#   Job "Update Available" Message for data on MQTT Broker - Publisher sends to Device
#       on topic <my unique topic>
#
#   '{
#       "Message": "Update Available",
#       "Manufacturer": "<company name>",
#       "ManufacturerID": "<company ID>",
#       "ProductID": "<product ID>",
#       "SerialNumber": "<serial number>",
#       "BoardName": "CY8CPROTO_062_4343W",
#       "Version": "<software version>",
#       "Connection": "MQTT",
#       "Broker": "<broker name>",
#       "Port": "1883",
#       "UniqueTopicName": "<my unique topic>",
#       ... other info as desired ...
#   }'
#
#   Job "Update Available" Message for data on HTTP Server - Publisher sends to Device
#       sent on topic <my unique topic>
#
#   '{
#       "Message": "Update Available",
#       "Manufacturer": "<company name>",
#       "ManufacturerID": "<company ID>",
#       "ProductID": "<product ID>",
#       "SerialNumber": "<serial number>",
#       "BoardName": "CY8CPROTO_062_4343W",
#       "Version": "<software version>",
#       "Connection": "HTTPS",
#       "Server": "10.0.0.88",
#       "Port": "80",
#       "FileName": "<filename on server>",
#       "UniqueTopicName": "<my unique topic>",
#       ... other info as desired ...
#   }'
#
#   "RequestUpdateMessage" - Device sends to Publisher
#       sent on topic <my unique topic>
#
#    {
#       "Message":"Request Update",
#       "Manufacturer": "<company name>",
#       "ManufacturerID": "<company ID>",
#       "ProductID": "<product ID>",
#       "SerialNumber": "<serial number>",
#       "BoardName": "CY8CPROTO_062_4343W",
#       "Version": "<software version>",
#       "UniqueTopicName": "<my unique topic>",
#       ... other info as desired ...
#   }
#
# To request a chunk of data
#
#   {
#       "Message": "Request Data Chunk",
#       "Manufacturer":"Express Widgits Corporation",
#       "ManufacturerId":"EWCO",
#       "Product":"Easy Widgit",
#       "SerialNumber":"ABC213450001",
#       "Board":"CY8CPROTO_062_4343W",
#       "Version":"1.2.0",
#       "Offset":"0",
#       "Length":"4096"
#   }
#
#==============================================================================
# Debugging help
#   To turn on logging, Set DEBUG_LOG to 1 (or use command line arg "-l")
#==============================================================================
DEBUG_LOG = 0
DEBUG_LOG_STRING = "0"

#==============================================================================
# Defines
#==============================================================================

KIT = "CY8CPROTO_062_4343W"

SEND_IMAGE_MQTT_CLIENT_ID = "OTASend"

# Subscriptions
COMPANY_TOPIC_PREPEND  = "OTAUpdate"
PUBLISHER_LISTEN_TOPIC = "publish_notify"
PUBLISHER_DIRECT_TOPIC = "OTAImage"

# These are created at runtime so that KIT can be replaced
PUBLISHER_JOB_REQUEST_TOPIC = ""
PUBLISHER_DIRECT_REQUEST_TOPIC = ""


BAD_JSON_DOC = "MALFORMED JSON DOCUMENT"            # Bad incoming message
UPDATE_AVAILABLE_REQUEST = "Update Availability"    # Device requests if there is an Update avaialble
SEND_UPDATE_REQUEST = "Request Update"              # Device requests Publisher send the OTA Image
SEND_DIRECT_UPDATE = "Send Direct Update"           # Device sent Update Direct request
SEND_CHUNK = "Request Data Chunk"                   # Device sent Request for a chunk of the data file
REPORTING_RESULT_SUCCESS = "Success"                # Device sends the OTA result Success
REPORTING_RESULT_FAILURE = "Failure"                # Device sends the OTA result Failure

MSG_TYPE_ERROR = -1
MSG_TYPE_UPDATE_AVAILABLE = 1       # Device sent UPDATE_AVAILABLE_REQUEST
MSG_TYPE_SEND_UPDATE = 2            # Device sent SEND_UPDATE_REQUEST
MSG_TYPE_RESULT_SUCCESS = 3         # Device sent REPORTING_RESULT_SUCCESS
MSG_TYPE_RESULT_FAILURE = 4         # Device sent REPORTING_RESULT_FAILURE
MSG_TYPE_SEND_DIRECT = 5            # Device sent SEND_DIRECT_UPDATE
MSG_TYPE_SEND_CHUNK = 6             # Device sent SEND_CHUNK

NO_AVAILABLE_REPONSE = "No Update Available"    # Publisher sends back to Device when no update available
AVAILABLE_REPONSE = "Update Available"          # Publisher sends back to Device when update is available
RESULT_REPONSE = "Result Received"              # Publisher sends back to Device so Device knows Publisher received the results

# This example uses a pre-made JSON document for the JobMessage
JSON_JOB_MESSAGE_FILE = "ota_update.json"

# Public MQTT Brokers for Testing
# Amazon only works with TLS with proper certificates
# Elipse TLS will work with proper certificates
# Mosquitto works sometimes - it is a test server for the Mosquitto project
#
# Set the Broker using command line arguments "-b amazon"
# AMAZON_BROKER_ADDRESS = "a33jl9z28enc1q-ats.iot.us-west-1.amazonaws.com"
AMAZON_BROKER_ADDRESS = "a33jl9z28enc1q-ats.iot.us-east-1.amazonaws.com"

# Set the Broker using command line arguments "-b eclipse"
ECLIPSE_BROKER_ADDRESS = "mqtt.eclipseprojects.io"

# Set the Broker using command line arguments "-b mosquitto"
MOSQUITTO_BROKER_ADDRESS = "test.mosquitto.org"
# default is mosquitto
BROKER_ADDRESS = MOSQUITTO_BROKER_ADDRESS

TLS_ENABLED = False         # turn on with "tls" argument on command line when invoking this script

# Quality of Service settings
PUBLISHER_PUBLISH_QOS = 1     # AWS broker does not support QOS of 2
PUBLISHER_SUBSCRIBE_QOS = 1   # AWS broker does not support QOS of 2

# Path to the firmware image
OTA_IMAGE_FILE = "../bld/ota-update/CY8CPROTO-062-4343W/Debug/ota-update.bin"

# Paho MQTT client settings
MQTT_KEEP_ALIVE = 60 # in seconds

# Each Chunk of the OTA Image needs a header for the Device to handle the chunk properly
#
# Size of each chunk of data sent to the Device when splitting the OTA Image
CHUNK_SIZE = (4 * 1024)

# OTA header information - MUST match Device structure cy_ota_mqtt_chunk_payload_header_s
#                          defined in ota-update/source/cy_ota_mqtt.c !!
HEADER_SIZE = 32            # Total header size in bytes
HEADER_MAGIC = "OTAImage"   # "Magic" string to identify the header
IMAGE_TYPE = 0
VERSION_MAJOR = 2           # Version of software in OTA Image
VERSION_MINOR = 0
VERSION_BUILD = 0

MAGIC_POS = 0               # offset in header of HEADER_MAGIC
DATA_START_POS = 1          # offset in header of offset in packet where the data chunk starts
TOTAL_FILE_SIZE_POS = 6     # offset in header of the total file size
FILE_OFFSET_POS = 7         # offset in header of the offset within the full file
PAYLOAD_SIZE_POS = 8        # offset in header of the size of the data in this chunk
TOTAL_PAYLOADS_POS = 9      # offset in header of the total number of payloads to send
PAYLOAD_INDEX_POS = 10      # offset in header of the index of THIS payload

ca_certs = "no ca_certs"
certfile = "no certfile"
keyfile = "no keyfile"


# Define a class to encapsulate some variables

class MQTTSender(mqtt.Client):
   def __init__(self,cname,**kwargs):
      super(MQTTSender, self).__init__(cname,**kwargs)
      self.connected_flag=False
      self.publish_mid=-1


class MQTTPublisher(mqtt.Client):
   def __init__(self,cname,**kwargs):
      super(MQTTPublisher, self).__init__(cname,**kwargs)
      self.connected_flag=False
      self.subscribe_mid=-1
      self.publish_mid=-1

#==============================================================================
# Handle ctrl-c to end program gracefully
#==============================================================================
terminate = False

# Signal handling function
def signal_handling(signum,frame):
   global terminate
   terminate = True

# take over the signals (SIGINT - MAC & Linux, SIGBREAK - Windows
signal.signal(signal.SIGINT,signal_handling)
signal.signal(signal.SIGBREAK,signal_handling)

#==============================================================================
# Logging functions
#==============================================================================
def on_pub_log(client, userdata, level, buf):
    print("Publisher: log: ",buf)

def on_send_log(client, userdata, level, buf):
    print("Send Image: log: ",buf)

#==============================================================================
#
# Publisher Functions
#
#==============================================================================

# ---------------------------------------------------------
#   do_chunking()
#       Break the large file into smaller chunks,
#       add header, and publish
#   image_file    - name of OTA Image file to send to Device
# ---------------------------------------------------------

def do_chunking(image_file, whole_file, file_offset, send_size):
    global terminate
    offset = 0                                      # assume start at 0
    image_size = os.path.getsize(image_file)        # assume full file
    payload_index = 0
    pub_total_payloads = image_size//send_size      # determine # payloads based on file size and chunk size
    if ((image_size % send_size) != 0):
        pub_total_payloads += 1

    if (whole_file == False):
        offset = file_offset
        payload_index = int(file_offset/send_size)
        if DEBUG_LOG:
            print("Send Image CHUNK offset: " + str(file_offset) + "Image size: " + str(image_size) + " idx: " + str(payload_index) )
    else:
        print("Image Size: " + str(image_size) + " Offset: " + str(file_offset) + ", Total Payloads: " + str(pub_total_payloads))

    with open(image_file, 'rb') as image:
        pub_mqtt_msgs = []

        while True:
            if terminate:
                exit(0)

            # read up to offset
            image.seek(offset)
            chunk = image.read(send_size)

            if chunk:
                chunk_size = len(chunk)
                # print("Trying to send chunk_size: " + str(chunk_size))
                packet = bytearray(HEADER_SIZE)

                # MQTT payload (chunk) header format is defined in ota-update/source/cy_ota_mqtt.c
                # typedef struct cy_ota_mqtt_chunk_payload_header_s {
                #     const char      magic[8];                          /* "OTAImage"                                            */
                #     const uint16_t  offset_to_data;                    /* Offset within this payload to start of data           */
                #     const uint16_t  ota_image_type;                    /* 0 = single application OTA Image                      */
                #     const uint16_t  update_version_major;              /* Major version number                                  */
                #     const uint16_t  update_version_minor;              /* Minor version number                                  */
                #     const uint16_t  update_version_build;              /* Build version number                                  */
                #     const uint32_t  total_size;                        /* Total size of OTA Image                               */
                #     const uint32_t  image_offset;                      /* Offset within the final OTA Image of THIS chunk data  */
                #     const uint16_t  data_size;                         /* Size of chunk data in THIS payload                    */
                #     const uint16_t  total_num_payloads;                /* Total number of payloads                              */
                #     const uint16_t  this_payload_index;                /* THIS payload index                                    */
                # } cy_ota_mqtt_chunk_payload_header_t;

                # s - 1 byte character, H - 2 bytes integer, I - 4 bytes integer
                struct.pack_into('<8s5H2I3H', packet, 0, HEADER_MAGIC.encode('ascii'),
                                  HEADER_SIZE, IMAGE_TYPE, VERSION_MAJOR, VERSION_MINOR,
                                  VERSION_BUILD, image_size, offset, chunk_size, pub_total_payloads,
                                  payload_index)

                # print header info
                # if (DEBUG_LOG == True):
                #    header = struct.unpack('<8s5H2I3H', packet[0:HEADER_SIZE])
                #    data_start = header[DATA_START_POS]
                #    print("    data start:" + str(data_start))
                #    file_size = header[TOTAL_FILE_SIZE_POS]
                #    print("     file size:" + str(file_size))
                #    off = header[FILE_OFFSET_POS]
                #    print("   file offset:" + str(off))
                #    payload_size = header[PAYLOAD_SIZE_POS]
                #    print("  payload size:" + str(payload_size))
                #    payload_index = header[PAYLOAD_INDEX_POS]
                #    print(" payload index:" + str(payload_index))
                #    total_payload = header[TOTAL_PAYLOADS_POS]
                #    print("  num  payload:" + str(total_payload))

                packet += chunk
                pub_mqtt_msgs.append(packet)

                offset += chunk_size
                payload_index += 1

            else:
                break

            if (whole_file == False):
                # just one chunk here
                # we have already sent the chunk, return from this routine
                if DEBUG_LOG:
                    print(" CHUNK Queued " + str(payload_index) + " chunks")
                return pub_mqtt_msgs,pub_total_payloads

        return pub_mqtt_msgs,pub_total_payloads

# -----------------------------------------------------------
#   parse_incoming_request()
#       Parse the message from the Device
#       We receive the message string
#   Parse for various info
#   message_type - UPDATE_AVAILABLE_REQUEST     - Device requesting if Update is Available
#                  SEND_UPDATE_REQUEST          - Device requesting to send the Update
#                  REPORTING_RESULT_SUCCESS     - Device sending result of Success
#                  REPORTING_RESULT_FAILURE     - Device sending result of Failure
# -----------------------------------------------------------
def parse_incoming_request(message_string):
    # Parse JSON doc
    # strip leading whitespace
    message_string = message_string.lstrip()
    if message_string[0] != '{':
        print("Malformed JSON document : '" + message_string + "'\r\n")
        return BAD_JSON_DOC,MSG_TYPE_ERROR,BAD_JSON_DOC

    try:
        if (DEBUG_LOG):
            print("\r\nABOUT To PARSE:\r\n'" + message_string + "'\r\n")
        request_json = json.loads(message_string)
    except Exception as e:
        print("Exception Occurred during json parse ... Exiting...")
        print(str(e) + os.linesep)
        traceback.print_exc()
        exit(0)

    # determine the request message type
    request = request_json["Message"]
    #
    # A direct request does not have a unique topic name
    if request != SEND_DIRECT_UPDATE:
        unique_topic_name = request_json["UniqueTopicName"]
    else:
        unique_topic_name = PUBLISHER_DIRECT_REQUEST_TOPIC

    if request == UPDATE_AVAILABLE_REQUEST:
        # Is Update Available ?
        return request,MSG_TYPE_UPDATE_AVAILABLE,unique_topic_name

    if request == SEND_UPDATE_REQUEST:
        # Device Requests Update
        return request,MSG_TYPE_SEND_UPDATE,unique_topic_name

    if request == REPORTING_RESULT_SUCCESS:
        #
        # Enhancement:
        # Save Success result in a database.
        #
        return request,MSG_TYPE_RESULT_SUCCESS,unique_topic_name

    if request == REPORTING_RESULT_FAILURE:
        #
        # Enhancement:
        # Save Failure result in a database.
        #
        return request,MSG_TYPE_RESULT_FAILURE,unique_topic_name

    if request == SEND_DIRECT_UPDATE:
        #
        # Always send a direct update on PUBLISHER_DIRECT_REQUEST_TOPIC
        #
        return request,MSG_TYPE_SEND_DIRECT,PUBLISHER_DIRECT_REQUEST_TOPIC

    if request == SEND_CHUNK:
        #
        # Send a chunk of data update on PUBLISHER_DIRECT_REQUEST_TOPIC
        #
        if (DEBUG_LOG):
            print(" SUBSCRIBER ASKED FOR A SINGLE CHUNK !!!!!")
        return request,MSG_TYPE_SEND_CHUNK,unique_topic_name

    print("Could not understand the message!")
    return BAD_JSON_DOC,MSG_TYPE_ERROR,BAD_JSON_DOC
# -----------------------------------------------------------
#   on send connection callback
# -----------------------------------------------------------
def on_send_connect(client, userdata, flags, rc):
    client.connected_flag = True

# -----------------------------------------------------------
#   on send publish callback
# -----------------------------------------------------------
def on_send_publish(client, userdata, mid):
    client.publish_mid = mid


# ---------------------------------------------------------
#   send_image_chunk_thread()
#       This is used in a separate thread.
#       Call do_chunking() to send one chunk to the Device.
#   message_string  - The Initial "Update Availability" message
#   unique_topic    - The unique topic to send the OTA Image on.
# ---------------------------------------------------------

def send_image_chunk_thread(message_string, unique_topic):
    global terminate

    # Create unique MQTT ID
    client_id = SEND_IMAGE_MQTT_CLIENT_ID + str(random.randint(0, 1024*1024*1024))
    client_id = str.ljust(client_id, 24)  # limit to 24 characters
    client_id = str.rstrip(client_id)
    if DEBUG_LOG:
        print("Send Image chunks: MQTT Connect on topic: " + unique_topic)

    # Create a new client
    send_client = MQTTSender(client_id)
    if (DEBUG_LOG):
        send_client.on_log = on_send_log

    send_client.on_connect = on_send_connect
    send_client.on_publish = on_send_publish
    if TLS_ENABLED:
        send_client.tls_set(ca_certs, certfile, keyfile)
    send_client.connect(BROKER_ADDRESS, BROKER_PORT, MQTT_KEEP_ALIVE)
    while send_client.connected_flag == False:
        send_client.loop(0.1)
        time.sleep(0.1)
        if terminate:
            exit(0)

    try:
        time_string = time.asctime()
        if (DEBUG_LOG):
            print("Send Chunk Begins..." + time_string + " ")

        job_dict = json.loads(message_string)
        offset = int(job_dict["Offset"])
        size = int(job_dict["Size"])

        pub_mqtt_msgs,pub_total_payloads = do_chunking(OTA_IMAGE_FILE, False, offset, size)

        # print(" Sending Chunk  offset:" + str(offset) + " size:" + str(size) + " packet sz: " + str( len(pub_mqtt_msgs[0])) + " to: " + unique_topic)
        result,messageID = send_client.publish(unique_topic, pub_mqtt_msgs[0], PUBLISHER_PUBLISH_QOS)
        while send_client.publish_mid != messageID:
            send_client.loop(0.1)
            time.sleep(0.1)
            if terminate:
                exit(0)

        time_string = time.asctime()
        if (DEBUG_LOG):
            print("Send Chunk Ends..." + time_string )

    except Exception as e:
        print("Exception Occurred... Exiting...")
        print(str(e) + os.linesep)
        traceback.print_exc()
        exit(0)

    # we're done
    exit(0)


# -----------------------------------------------------------
#   send_image_thread()
#       This is used in a separate thread.
#       Call do_chunking() and send the chunks to the Device.
#   message_string  - The Initial "Update Availability" message
#   unique_topic    - The unique topic to send the OTA Image on.
#
# -----------------------------------------------------------
def send_image_thread(message_string, unique_topic):
    global terminate

    # Create unique MQTT ID
    client_id = SEND_IMAGE_MQTT_CLIENT_ID + str(random.randint(0, 1024*1024*1024))
    client_id = str.ljust(client_id, 24)  # limit to 24 characters
    client_id = str.rstrip(client_id)
    print("Send Image: MQTT Connect on topic: " + unique_topic)

    # Create a new client
    send_client = MQTTSender(client_id)
    if (DEBUG_LOG):
        send_client.on_log = on_send_log

    send_client.on_connect = on_send_connect
    send_client.on_publish = on_send_publish
    if TLS_ENABLED:
        send_client.tls_set(ca_certs, certfile, keyfile)
    send_client.connect(BROKER_ADDRESS, BROKER_PORT, MQTT_KEEP_ALIVE)
    while send_client.connected_flag == False:
        send_client.loop(0.1)
        time.sleep(0.1)
        if terminate:
            exit(0)

    try:
        time_string = time.asctime()
        print("Publishing Begins..." + time_string + " ")
        pub_mqtt_msgs,pub_total_payloads = do_chunking(OTA_IMAGE_FILE, True, 0, CHUNK_SIZE)

        # for chunk in pub_mqtt_msgs:
        for chunk in range(0,pub_total_payloads):
            if terminate:
                exit(0)
            # print(" Sending Chunk " + str(chunk)  + " of " + str(pub_total_payloads) + " to: " + unique_topic)
            result,messageID = send_client.publish(unique_topic, pub_mqtt_msgs[chunk], PUBLISHER_PUBLISH_QOS)
            while send_client.publish_mid != messageID:
                send_client.loop(0.1)
                time.sleep(0.1)
                if terminate:
                    exit(0)

        time_string = time.asctime()
        print("Publishing Ends..." + time_string )

    except Exception as e:
        print("Exception Occurred... Exiting...")
        print(str(e) + os.linesep)
        traceback.print_exc()
        exit(0)

    # we're done
    exit(0)


# -----------------------------------------------------------
#   publisher_recv_message()
#       This is the on_message callback for the Publisher
#       We receive the client, userdata, and message from the Device
#   client   - Publisher MQTT client instance
#   userdata -
#   message  - Message sent from the Device
# -----------------------------------------------------------
def publisher_recv_message(client, userdata, message):
    global VERSION_MAJOR
    global VERSION_MINOR
    global VERSION_BUILD
    # print("message received " ,str(message.payload.decode("utf-8")))
    # print("message topic=",message.topic)
    # print("message qos=",message.qos)
    # print("message retain flag=",message.retain)
    # print("\nMessage Payload")
    # for c in message.payload:
    #    print( str(c) + "  " + str(int(c)))

    message_string = str(message.payload.decode("utf-8"))
    request,message_type,unique_topic = parse_incoming_request(message_string)
    # print("\nPublisher: Message received: " + request + " topic: " + unique_topic)

    # Handle bad doc
    if message_type == MSG_TYPE_ERROR:
        print("Bad doc")

    # Handle incoming "Is Update Available?" request
    if message_type == MSG_TYPE_UPDATE_AVAILABLE:
        #
        # Possible Enhancement:
        #   Check parameters from the Device to check if there is an update
        #       for board/product/version/etc.
        #   Open a specific response file or generate the JSON on the fly.
        #
        # Get the Job file (or create a JSON document )
        job_file = open (JSON_JOB_MESSAGE_FILE)
        job_source = job_file.read()
        job_file.close()

        # Add the unique_topic to outgoing job
        try:
            job_dict = json.loads(job_source)
            job_dict["Message"]=AVAILABLE_REPONSE       # use NO_AVAILABLE_REPONSE if no update available
            job_dict["UniqueTopicName"] = unique_topic
            # print ("Got unique topic:" + unique_topic )
            job = json.dumps(job_dict)
        except Exception as e:
            print("Exception Occurred during json parse ... Exiting...")
            print(str(e) + os.linesep)
            traceback.print_exc()
            exit(0)

        # Get version # from the JOB_MESSAGE_FILE for sending chunks later
        update_app_version = job_dict["Version"]
        print("JOB DOC VERSION: " + update_app_version)
        VERSION_MAJOR_STR, VERSION_MINOR_STR, VERSION_BUILD_STR = re.split("\.", update_app_version)
        VERSION_MAJOR = int(VERSION_MAJOR_STR)
        VERSION_MINOR = int(VERSION_MINOR_STR)
        VERSION_BUILD = int(VERSION_BUILD_STR)

        print("Publisher: Sending on topic:" + unique_topic + "Job Doc: >" + job + "<")
        client.publish(unique_topic, job, PUBLISHER_PUBLISH_QOS)
        return


    # Handle incoming "Send Update" request
    if message_type == MSG_TYPE_SEND_UPDATE:
        #
        # Possible Enhancement:
        #   Check that the json_document matches the one sent for MSG_TYPE_UPDATE_AVAILABLE
        #   Determine the OTA Image file to send to the Device.
        #   Keep track that Device update was sent so response can be tested
        #
        # Create a new thread to send the data. This will allow for multiple, overlapping requests.
        send_thread = threading.Thread(None, send_image_thread, None, args=(message_string, unique_topic))
        send_thread.start()
        return

    # Handle incoming "Direct Update" request
    if message_type == MSG_TYPE_SEND_DIRECT:
        #
        # Possible Enhancement:
        #   Determine the OTA Image file to send to the Device.

        print( "Publisher: Send Direct OTA on topic:" + unique_topic )

        # Create a new thread to send the data. This will allow for multiple, overlapping requests.
        send_thread = threading.Thread(None, send_image_thread, None, args=(message_string, unique_topic))
        send_thread.start()
        return


    # Handle incoming "Request Data Chunk" request
    if message_type == MSG_TYPE_SEND_CHUNK:
        #
        # Possible Enhancement:
        #   Determine the OTA Image file to send to the Device.

        # print( "Publisher: Send Chunk of OTA Image on topic:" + unique_topic )

        # Create a new thread to send the data. This will allow for multiple, overlapping requests.
        print("Publisher: Start Sending CHUNK Thread")
        send_thread = threading.Thread(None, send_image_chunk_thread, None, args=(message_string, unique_topic))
        send_thread.start()
        return

    # Handle incoming "result" notification
    if (message_type == MSG_TYPE_RESULT_SUCCESS) | (message_type == MSG_TYPE_RESULT_FAILURE):
        #
        # Possible Enhancement:
        #   Match the Device that requested the updte
        #   Save the Device update report to a database
        #
        # Add the unique_topic to outgoing job
        # Get the Job file (or create a JSON document )
        job_file = open (JSON_JOB_MESSAGE_FILE)
        job_source = job_file.read()
        job_file.close()
        try:
            job_dict = json.loads(job_source)
            job_dict["Message"]=RESULT_REPONSE
            job_dict["UniqueTopicName"] = unique_topic
            job = json.dumps(job_dict)
        except Exception as e:
            print("Exception Occurred during json parse ... Exiting...")
            print(str(e) + os.linesep)
            traceback.print_exc()
            exit(0)
        print("Publisher sending result response: " + job )
        client.publish(unique_topic, job, PUBLISHER_PUBLISH_QOS)
        return

# -----------------------------------------------------------
#   on connection callback
# -----------------------------------------------------------
def on_connect(client, userdata, flags, rc):
    client.connected_flag = True

# -----------------------------------------------------------
#   on subscribe callback
# -----------------------------------------------------------
def on_subscribe(client, userdata, mid, granted_qos):
    client.subscribe_mid = mid

# -----------------------------------------------------------
#   publisher_loop
# -----------------------------------------------------------
def publisher_loop():
    global terminate
    client_id = SEND_IMAGE_MQTT_CLIENT_ID + str(random.randint(0, 1024*1024*1024))
    client_id = str.ljust(client_id, 24)  # limit to 24 characters
    client_id = str.rstrip(client_id)
    print("Publisher: MQTT Connect with id: " + client_id)
    pub_client = MQTTPublisher(client_id)
    pub_client.on_message = publisher_recv_message
    if (DEBUG_LOG):
        pub_client.on_log = on_pub_log

    pub_client.on_connect = on_connect
    pub_client.on_subscribe = on_subscribe
    if TLS_ENABLED:
        pub_client.tls_set(ca_certs, certfile, keyfile)

    pub_client.connect(BROKER_ADDRESS, BROKER_PORT, MQTT_KEEP_ALIVE)
    while pub_client.connected_flag == False:
        pub_client.loop(0.1)
        time.sleep(0.1)
        if terminate:
            exit(0)

    print("Publisher: Waiting for Job request on: '" + PUBLISHER_JOB_REQUEST_TOPIC + "'" )
    result,messageID = pub_client.subscribe(PUBLISHER_JOB_REQUEST_TOPIC, PUBLISHER_SUBSCRIBE_QOS)
    while pub_client.subscribe_mid != messageID:
        pub_client.loop(0.1)
        time.sleep(0.1)
        if terminate:
            exit(0)

    print("Publisher: Connected and Subscribed. Waiting for Requests.")
    # Loop forever
    while True:
        pub_client.loop(0.1)
        time.sleep(0.1)
        if terminate:
            exit(0)

# =====================================================================
#
# Start of "main"
#
# Look at arguments and find what mode we are in.
#
# Possible arguments TLS setting for MQTT connection (default is non-TLS)
#
# Usage:
#   python publisher.py [TLS]
#
# =====================================================================

if __name__ == "__main__":
    print("Infineon Test MQTT Publisher.")
    print("   Usage: 'python publisher.py [tls] [-l] [-b <broker>] [-k <kit>] [-f <filepath>] [-c <company_topic>]'")
    print("[tls]              Use TLS for connection")
    print("-l                 Turn on extra logging")
    print("-b <broker>        '[a] | [amazon] | [e] | [eclipse] | [m] | [mosquitto]'")
    print("-k <kit>           '[CY8CKIT_062S2_43012] | [CY8CKIT_064B0S2_4343W] | [CY8CPROTO_062_4343W]'")
    print("-f <filepath>      The location of the OTA Image file to server to the device")
    print("-c <company_topic> This will be the beginning of the topic: <company_topic>/")
    print("Defaults: non-TLS")
    print("        : -f " + OTA_IMAGE_FILE)
    print("        : -b mosquitto ")
    print("        : -k " + KIT)
    print("        : -c " + COMPANY_TOPIC_PREPEND)
    last_arg = ""
    for i, arg in enumerate(sys.argv):
        # print(f"Argument {i:>4}: {arg}")
        if arg == "TLS" or arg == "tls":
            TLS_ENABLED = True
        if arg == "-l":
            DEBUG_LOG = 1
            DEBUG_LOG_STRING = "1"
        if last_arg == "-f":
            OTA_IMAGE_FILE = arg
        if last_arg == "-c":
            COMPANY_TOPIC_PREPEND = arg
        if last_arg == "-b":
            if ((arg == "amazon") | (arg == "a")):
                BROKER_ADDRESS = AMAZON_BROKER_ADDRESS
            if ((arg == "eclipse") | (arg == "e")):
                BROKER_ADDRESS = ECLIPSE_BROKER_ADDRESS
            if ((arg == "mosquitto") | (arg == "m")):
                BROKER_ADDRESS = MOSQUITTO_BROKER_ADDRESS
        if last_arg == "-k":
            KIT = arg
        last_arg = arg

print("\n")
print("Values for this run:\n")
if TLS_ENABLED:
    print("   Using TLS")
else:
    print("   Using non-TLS")
print("   Using BROKER: " + BROKER_ADDRESS)
print("   Using    KIT: " + KIT)
print("   Using   File: " + OTA_IMAGE_FILE)
print("   extra debug : " + DEBUG_LOG_STRING)
print(" company topic : " + COMPANY_TOPIC_PREPEND)

PUBLISHER_JOB_REQUEST_TOPIC = COMPANY_TOPIC_PREPEND + "/" + KIT + "/" + PUBLISHER_LISTEN_TOPIC
print("PUBLISHER_JOB_REQUEST_TOPIC   : " + PUBLISHER_JOB_REQUEST_TOPIC)

PUBLISHER_DIRECT_REQUEST_TOPIC = COMPANY_TOPIC_PREPEND + "/" + KIT + "/" + PUBLISHER_DIRECT_TOPIC
print("PUBLISHER_DIRECT_REQUEST_TOPIC: " + PUBLISHER_DIRECT_REQUEST_TOPIC)
print("\n")

#
# set TLS broker and certs based on args
#
# Set the TLS port, certs, and key
#
if TLS_ENABLED:
    BROKER_PORT = 8883
    if BROKER_ADDRESS == MOSQUITTO_BROKER_ADDRESS:
        BROKER_PORT = 8884
        ca_certs = "mosquitto.org.crt"
        certfile = "mosquitto_client.crt"
        keyfile  = "mosquitto_client.key"
    elif BROKER_ADDRESS == ECLIPSE_BROKER_ADDRESS:
        ca_certs = "eclipse_ca.crt"
        certfile = "eclipse_client.crt"
        keyfile  = "eclipse_client.pem"
    else:
        ca_certs = "amazon_ca.crt"
        certfile = "amazon_client.crt"
        keyfile  = "amazon_private_key.pem"
    print("Connecting using TLS to '" + BROKER_ADDRESS + ":" + str(BROKER_PORT) + "'" + os.linesep)
else:
    BROKER_PORT = 1883
    print("Unencrypted connection to '" + BROKER_ADDRESS + ":" + str(BROKER_PORT) + "'" + os.linesep)

publisher_loop()
