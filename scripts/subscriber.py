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

random.seed()

#
#   This is the MQTT test Subsciber for OTA Update device updating.
#   It is for use with the MQTT test Publisher for OTA device updating.
#   This was created to test the protocol & Publisher script.

#==============================================================================
# Debugging help
#   To turn on logging, Set DEBUG_LOG to 1 (or use "-l" on command line)
#==============================================================================
DEBUG_LOG = 0
DEBUG_LOG_STRING = "0"

# Customizations:
KIT = "CY8CPROTO_062_4343W"

# Subscriptions
COMPANY_TOPIC_PREPEND = "OTAUpdate"
PUBLISHER_LISTEN_TOPIC = "publish_notify"
PUBLISHER_DIRECT_TOPIC = "OTAImage"

# These are created at runtime so that KIT can be replaced

# SUBSCRIBER_PUBLISH_TOPIC = COMPANY_TOPIC_PREPEND  + "/" + KIT + "/" + PUBLISHER_LISTEN_TOPIC    # Device sends messages to Publisher
# PUBLISHER_DIRECT_REQUEST_TOPIC = COMPANY_TOPIC_PREPEND + "/" + KIT + "/" + PUBLISHER_DIRECT_TOPIC            # Device asks for Download Directly (no Job)

BAD_JSON_DOC = "MALFORMED JSON DOCUMENT"            # Bad incoming message
UPDATE_AVAILABLE_REQUEST = "Update Availability"    # Device requests if there is an Update avaialble
NO_AVAILABLE_REPONSE = "No Update Available"        # Publisher sends back to Device when no update available
AVAILABLE_REPONSE = "Update Available"              # Publisher sends back to Device when update is available
SEND_UPDATE_REQUEST = "Request Update"              # Device requests Publisher send the OTA Image
SEND_DIRECT_UPDATE = "Send Direct Update"           # Device sends Update Direct request
SEND_CHUNK_REQUEST = "Request Data Chunk"           # Device sends Single Chunk Request
REPORTING_RESULT_SUCCESS = "Success"                # Device sends the OTA result Success
REPORTING_RESULT_FAILURE = "Failure"                # Device sends the OTA result Failure
RESULT_REPONSE = "Result Received"                  # Publisher sends back to Device so Device knows Publisher received the results

MSG_TYPE_ERROR = -1
MSG_TYPE_UPDATE_AVAILABLE = 1       # Publisher sent AVAILABLE_REPONSE
MSG_TYPE_NO_UPDATE_AVAILABLE = 2    # Publisher sent NO_AVAILABLE_REPONSE
MSG_TYPE_CHUNK = 3                  # Publisher sent OTA Image chunk
MSG_TYPE_RESULT_RESPONSE = 4        # Publisher sent RESULT_REPONSE

SUBSCRIBER_RETRY_REQ_WAIT = 20

#
# DOWNLOAD_REQUEST includes info from NOTIFICATION_MESSAGE so that Publisher
#   can match the proper download if there are more than 1 updates available
#   for more than 1 kit.
DOWNLOAD_REQUEST = "Request Download"

# DOWNLOAD_COMPLETE includes "unique topic" so Publisher can take appropriate action
DOWNLOAD_COMPLETE = "Download Complete:<unique topic>"

SUBSCRIBER_MQTT_CLIENT_ID = "OTASubscriber"

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

SUBSCRIBER_PUBLISH_QOS = 1
SUBSCRIBER_SUBSCRIBE_QOS = 1

# use Job flow, override on command line
USE_DIRECT_FLOW = False

# Message Template filename
JSON_MESSAGE_TEMPLATE = "ota_message.json"

# Full file path to the firmware image
OTA_IMAGE_FILE = "ota-update.out.bin"

# Paho MQTT client settings
MQTT_KEEP_ALIVE = 60 # in seconds
CHUNK_SIZE = (4 * 1024)

# OTA header information
HEADER_SIZE = 32 # in bytes
HEADER_MAGIC = "OTAImage"
IMAGE_TYPE = 0
VERSION_MAJOR = 1
VERSION_MINOR = 1
VERSION_BUILD = 0

MAGIC_POS = 0
DATA_START_POS = 1
TOTAL_FILE_SIZE_POS = 6
FILE_OFFSET_POS = 7
PAYLOAD_SIZE_POS = 8
TOTAL_PAYLOADS_POS = 9
PAYLOAD_INDEX_POS = 10

ca_certs = "no ca_certs"
certfile = "no certfile"
keyfile = "no keyfile"

# If we are requesting a chunk at a time
request_file_in_chunks = False
file_in_chunks_chunk_size = CHUNK_SIZE

# Define a class to encapsulate some variables

class MQTTSubscriber(mqtt.Client):
   def __init__(self,cname,**kwargs):
      super(MQTTSubscriber, self).__init__(cname,**kwargs)
      self.connected_flag=False
      self.subscribe_mid=-1
      self.publish_mid=-1
      self.download_complete=False
      # When we request chunks individually
      self.request_file_in_chunks = False
      self.received_a_chunk=0
      self.chunks_received=0
      self.current_offset = 0
      self.chunk_size = 0
      self.file_size = 0
      self.last_message = ""


# Handle ctrl-c to end program w/threading
# store the original SIGINT handler
# original_SIGINT = signal.getsignal(signal.SIGINT)
# original_SIGBREAK = signal.getsignal(signal.SIGBREAK)
terminate = False

def signal_handling(signum,frame):
   global terminate
   terminate = True

signal.signal(signal.SIGINT,signal_handling)
signal.signal(signal.SIGBREAK,signal_handling)

def on_sub_log(client, userdata, level, buf):
    print("Subscriber log: ",buf)

# =======================================================================================================
#
# Subscriber Variables & Functions
#
# =======================================================================================================

# default value for ota image - make it unique!
unique_topic_name = ""

sub_total_payloads = 0

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
#   on publish callback
# -----------------------------------------------------------
def on_publish(client, userdata, mid):
    client.publish_mid = mid

# -----------------------------------------------------------
#   parse_incoming_message()
#       Parse the message from the Device
#       We receive the message string
#   Parse for various info
#   message_type - AVAILABLE_REPONSE            - Publisher responding that there is an update available
# -----------------------------------------------------------
def parse_incoming_message(message_string):
    # Parse JSON doc

    try:
        actual_message_start = message_string.lstrip()
        # print("\r\nABOUT To PARSE:\r\n '" + message_string + "'\r\n")
        request_json = json.loads(message_string)
        request = request_json["Message"]
        unique_topic_name = request_json["UniqueTopicName"]
    except Exception as e:
        print("Exception Occurred during json parse ... Exiting...")
        print(str(e) + os.linesep)
        traceback.print_exc()
        exit(0)

    if request == NO_AVAILABLE_REPONSE:
        # NO Update Available !
        return request,MSG_TYPE_NO_UPDATE_AVAILABLE,unique_topic_name

    if request == AVAILABLE_REPONSE:
        # Update IS Available !
        return request,MSG_TYPE_UPDATE_AVAILABLE,unique_topic_name

    if request == RESULT_REPONSE:
        # Publisher acknowledges our result
        return request,MSG_TYPE_RESULT_RESPONSE,unique_topic_name

    print("parse_incoming_message(" + message_string + ") FAILED")
    return BAD_JSON_DOC,MSG_TYPE_ERROR,BAD_JSON_DOC

# -----------------------------------------------------------
#   reassemble_image
# -----------------------------------------------------------
def reassemble_image(client, image_file):
    global sub_total_payloads
    num_chunks = sub_total_payloads
    with open(image_file, 'ab') as out_file:
        # print(" Opened .. write the file: " + image_file)
        if (len(sub_mqtt_msgs) != sub_total_payloads) & (client.request_file_in_chunks == False):
            print("ERROR: Number of chunks expected: " + str(sub_total_payloads) + ", Received: " + str(len(sub_mqtt_msgs)))
            return
        else:
            if client.request_file_in_chunks == True:
                num_chunks = 1
            # print("Run through the chunks: " + str(num_chunks))
            for payload_index in range(0,num_chunks):
                # print( "Payload " + str(payload_index) + "...")
                chunk = sub_mqtt_msgs[payload_index]
                payload_len = len(chunk) - HEADER_SIZE
                header = struct.unpack('<8s5H2I3H', bytearray(chunk[0:HEADER_SIZE]))

                # print header info
                if (DEBUG_LOG == True):
                    data_start = header[DATA_START_POS]
                    print("    data start:" + str(data_start))
                    file_size = header[TOTAL_FILE_SIZE_POS]
                    print("     file size:" + str(file_size))
                    off = header[FILE_OFFSET_POS]
                    print("   file offset:" + str(off))
                    payload_size = header[PAYLOAD_SIZE_POS]
                    print("  payload size:" + str(payload_size))
                    payload_index = header[PAYLOAD_INDEX_POS]
                    print(" payload index:" + str(payload_index))
                    total_payload = header[TOTAL_PAYLOADS_POS]
                    print("  num  payload:" + str(total_payload))

                if (header[PAYLOAD_INDEX_POS] != payload_index) & (client.request_file_in_chunks == False):
                    print("ERROR: Chunks are received out of order")
                    return
                elif header[PAYLOAD_SIZE_POS] != payload_len:
                    print("ERROR: Payload size not matching. Size in header: " + str(header[PAYLOAD_SIZE_POS]) +
                        ", Actual size: " + str(payload_len))
                    return
                else:
                    data_start = header[DATA_START_POS]
                    payload = chunk[data_start:len(chunk)]
                    # print( "write chunk " + str(header[FILE_OFFSET_POS]) + " of: " + str(header[FILE_OFFSET_POS]) + " offset: " + str(header[PAYLOAD_INDEX_POS]) + ".")
                    out_file.seek(header[FILE_OFFSET_POS])
                    # print(" pos after seek " + str(out_file.tell()))
                    out_file.write(payload)

            out_file.close()
            file_size = os.stat(image_file).st_size
            if client.request_file_in_chunks == False:
                print("Image file " + image_file + "," + str(file_size) + " done!")


# -----------------------------------------------------------
#   subscriber_recv_message
# -----------------------------------------------------------
def subscriber_recv_message(client, userdata, message):
    global job
    global sub_mqtt_msgs
    global sub_total_payloads
    global sub_request_OTA_image
    global unique_topic_name
    # print("message received " ,str(message.payload.decode("utf-8")))
    # print("message topic=",message.topic)
    # print("message qos=",message.qos)
    # print("message retain flag=",message.retain)

    message_string = ""
    request = ""
    message_type = 0
    unique_topic = ""
    try:
        message_string = str(message.payload.decode("utf-8"))
        #print("\nSubscriber: Message: '" + message_string + "'\n")
        # drop anything before start of JSON doc
        pre,delim,message_string = message_string.partition("{")
        message_string = '{' + message_string
        # print("pre:" + pre + "delimiter:" + delim + ">" + message_string)
        request,message_type,unique_topic = parse_incoming_message(message_string)
        print("\nSubscriber: Message received: '" + request + "'\n")
        client.last_message = message_string
    except Exception as e:
        message_string = ""
        message_type = MSG_TYPE_CHUNK

    # Handle bad doc
    if message_type == MSG_TYPE_ERROR:
        print("Bad doc")
        return

    if message_type == MSG_TYPE_NO_UPDATE_AVAILABLE:
        job = ""
        print( "Subscriber: received no updates available.")
        return

    if message_type == MSG_TYPE_RESULT_RESPONSE:
        # Publisher responded to the result we sent!
        return

    if message_type == MSG_TYPE_UPDATE_AVAILABLE:
        if client.request_file_in_chunks == True:
            # ask for file in chunks
            job_dict = json.loads(message_string)
            job_dict["Message"] = SEND_CHUNK_REQUEST
            job_dict["Offset"] = str(client.current_offset)
            job_dict["Size"] = str(client.chunk_size)
            job_string = json.dumps(job_dict)
            # Publish the request
            print( "Subscriber: send Request Update. >" + job_string + "<\n")
            result,messageID = client.publish(SUBSCRIBER_PUBLISH_TOPIC, job_string, SUBSCRIBER_PUBLISH_QOS)
            client.loop(0.1)

        elif USE_DIRECT_FLOW == False:
            if message.topic == unique_topic:
                # An update is available - ask for the download
                job_dict = json.loads(message_string)
                job_dict["Message"] = SEND_UPDATE_REQUEST
                job_string = json.dumps(job_dict)
                # Publish the request
                print( "Subscriber: send Request Update. >" + job_string + "<\n")
                result,messageID = client.publish(SUBSCRIBER_PUBLISH_TOPIC, job_string, SUBSCRIBER_PUBLISH_QOS)
                client.loop(0.1)
            else:
                print( "WHAT UP? >" + message_string + "<\n")
                job_dict = json.loads(message_string)
                job_string = json.dumps(job_dict)
                print( "Subscriber: Job flow, no topic: " + job_string)
                exit(0)

        else:
            # An update is available - ask for the download
            job_dict = json.loads(message_string)
            job_dict["Message"] = SEND_DIRECT_UPDATE
            job_string = json.dumps(job_dict)
            # Publish the request
            print( "Subscriber: send Update DIRECT Request. >" + job_string + "<")
            result,messageID = client.publish(SUBSCRIBER_PUBLISH_TOPIC, job_string, SUBSCRIBER_PUBLISH_QOS)
            client.loop(0.1)
            print( "Subscriber: sent Update DIRECT Request")

        # print("Return from MSG_TYPE_UPDATE_AVAILABLE")
        return


    # when we get a chunk, we do not get the topic
    if message_type == MSG_TYPE_CHUNK:
        header = struct.unpack('<8s5H2I3H', message.payload[0:HEADER_SIZE])
        print(" Got Chunk: " + str(header[PAYLOAD_INDEX_POS]) + " of " + str(header[TOTAL_PAYLOADS_POS]) + " on " + message.topic)

        # print("Header: ", end="")
        # print(header)

        magic_word = str(header[MAGIC_POS], 'ascii')
        if magic_word == HEADER_MAGIC:
            send_result = False

            # Create the msgs list
            # - if this is the first payload of an image
            # - if this is a chunk request
            if (header[PAYLOAD_INDEX_POS] == 0) | (client.request_file_in_chunks == True):
                sub_mqtt_msgs = []

            sub_mqtt_msgs.append(message.payload)

            if client.request_file_in_chunks == True:
                client.received_a_chunk = 1
                client.chunks_received += 1
                client.file_size = header[TOTAL_FILE_SIZE_POS]
                # we received a chunk. If this is the last chunk, we are done
                # if not the last chunk, ask for the next one
                sub_total_payloads = header[TOTAL_PAYLOADS_POS]
                index = header[PAYLOAD_INDEX_POS]
                # print( "Subscriber: saving CHUNK " + str(index) + " of: " + str(sub_total_payloads) +  " to file: " + OTA_IMAGE_FILE )
                reassemble_image(client, OTA_IMAGE_FILE)
                if (index == (sub_total_payloads - 1)):
                    print(" Received last payload!")
                    send_result = True

            elif (header[PAYLOAD_INDEX_POS] == (header[TOTAL_PAYLOADS_POS] - 1)):
                sub_total_payloads = header[TOTAL_PAYLOADS_POS]
                print( "Subscriber: saving " + str(sub_total_payloads) + " to file: " + OTA_IMAGE_FILE )
                reassemble_image(client, OTA_IMAGE_FILE)
                send_result = True

            if (send_result == True):
                # File is completely received
                print("Sending Result\n")
                job_file = open (JSON_MESSAGE_TEMPLATE)
                job_source = job_file.read()
                job_file.close()
                job_dict = json.loads(job_source)
                job_dict["Message"] = REPORTING_RESULT_SUCCESS
                job_dict["UniqueTopicName"] = unique_topic_name
                job_string = json.dumps(job_dict)
                result, messageID = client.publish(SUBSCRIBER_PUBLISH_TOPIC, job_string, SUBSCRIBER_PUBLISH_QOS)
                client.loop(0.1)
                client.download_complete = True

        else:
            print("ERROR: Expected Header Magic: " + HEADER_MAGIC + ", Received: " + header[0])

        # print("return from CHUNK")
        return

    print("UNKNOWN MESSAGE TYPE: " + str(message_type))
    print("message received >" ,str(message.payload.decode("utf-8")) + "<")
    print("message topic=",message.topic)
    return


# -----------------------------------------------------------
#   get_OTA_image
# -----------------------------------------------------------
def get_OTA_image():
    global terminate
    global unique_topic_name

    time_value = time.monotonic_ns()
    time_string = repr(time_value)
    client_id = SUBSCRIBER_MQTT_CLIENT_ID + str(random.randint(0, 1024*1024*1024))
    client_id = str.ljust(client_id, 24)  # limit to 24 characters
    client_id = str.rstrip(client_id)
    print("Subscriber: Start id: " + client_id)
    sub_client = MQTTSubscriber(client_id)
    if DEBUG_LOG:
        sub_client.on_log = on_sub_log
    sub_client.on_connect = on_connect
    sub_client.on_subscribe = on_subscribe
    sub_client.on_publish = on_publish
    sub_client.on_message = subscriber_recv_message
    if TLS_ENABLED:
        sub_client.tls_set(ca_certs, certfile, keyfile)

    if (request_file_in_chunks == True):
        sub_client.request_file_in_chunks = True
        sub_client.received_a_chunk=0
        sub_client.chunks_received=0
        sub_client.current_offset = 0
        sub_client.chunk_size = file_in_chunks_chunk_size
        sub_client.last_message = ""

    sub_client.connect(BROKER_ADDRESS, BROKER_PORT, MQTT_KEEP_ALIVE)
    while sub_client.connected_flag == False:
        sub_client.loop(0.1)
        time.sleep(0.1)
        if terminate:
            exit(0)

    # Create Unique Topic Name
    print( "Subscriber: Create unique topic to receive the OTA Image" )
    rand_string = str(random.randint(0, 1024*1024*1024))
    unique_topic_name = COMPANY_TOPIC_PREPEND + "/" + KIT + "/subscriber/image" + rand_string

    print("Subscriber: Waiting for message from Publisher on: " + unique_topic_name)
    sub_client.subscribe_mid = -1
    result,messageID = sub_client.subscribe(unique_topic_name, SUBSCRIBER_SUBSCRIBE_QOS)
    sub_client.loop(0.1)

    print("Subscriber: Connected and Subscribed. Sending Request.")

    # load a sample message JSON and update the message type and the unique_topic_name
    job_file = open (JSON_MESSAGE_TEMPLATE)
    job_source = job_file.read()
    job_file.close()
    job_dict = json.loads(job_source)
    job_dict["Message"] = UPDATE_AVAILABLE_REQUEST
    job_dict["UniqueTopicName"] = unique_topic_name
    job_string = json.dumps(job_dict)

    # Publish the request
    sub_client.publish_mid = -1
    result,messageID = sub_client.publish(SUBSCRIBER_PUBLISH_TOPIC, job_string, SUBSCRIBER_PUBLISH_QOS)

    # wait forever
    while True:
        sub_client.loop(0.1)
        time.sleep(0.1)
        if terminate:
            exit(0)
        if (sub_client.request_file_in_chunks == True) & (sub_client.received_a_chunk == 1):
            sub_client.current_offset += sub_client.chunk_size
            sub_client.received_a_chunk = 0
            # if not done, request next chunk
            # print("   ---------- Main --- curr_off: " + str(sub_client.current_offset) + " of " + str(sub_client.file_size))
            if (sub_client.current_offset < sub_client.file_size):
                # request next chunk
                job_dict = json.loads(sub_client.last_message)
                job_dict["Message"] = SEND_CHUNK_REQUEST
                job_dict["Offset"] = str(sub_client.current_offset)
                job_dict["Size"] = str(sub_client.chunk_size)
                job_string = json.dumps(job_dict)
                # Publish the request
                if DEBUG_LOG:
                    print( "Subscriber: send Request Update. >" + job_string + "<\n")
                result,messageID = sub_client.publish(SUBSCRIBER_PUBLISH_TOPIC, job_string, SUBSCRIBER_PUBLISH_QOS)
                sub_client.loop(0.1)

        if sub_client.download_complete:
            sub_client.disconnect()
            return

# -----------------------------------------------------------
#   subscriber_loop
# -----------------------------------------------------------
def subscriber_loop():
    global terminate
    while True:
        get_OTA_image()
        print("waiting 15 seconds, then starting download again")
        i = 0
        while ( i < 150 ):
            time.sleep(0.1)
            i = i + 1
            if terminate:
                exit(0)

        return

# =====================================================================
#
# Start of "main"
#
# Look at arguments and find what mode we are in.
#
# Usage:
#   python subscriber.py [tls] [-f filepath]
#
# =====================================================================

if __name__ == "__main__":
    print("Infineon Test MQTT Subscriber.")
    print("   Usage: 'python subscriber.py [tls] [-l] [-b <broker>] [-k <kit>] [-f <filepath>] [-c <chunk_size>]  [-e <topic_suffix>]'")
    print("<broker>       '[a] | [amazon] | [e] | [eclipse] | [m] | [mosquitto]'")
    print("<kit>          '[CY8CKIT_062S2_43012] | [CY8CKIT_064B0S2_4343W] | [CY8CPROTO_062_4343W]'")
    print("<filepath>     The location to store the OTA Image file")
    print("<chunk_size>   The size (in decimal bytes) of the chunks to send - default=4096")
    print("<topic_suffix> This will be added to the beginning of the topic: 'OTAUpdate'<topic_suffix")
    print("Defaults: <non-TLS>")
    print("        : -f " + OTA_IMAGE_FILE)
    print("        : -b mosquitto ")
    print("        : -k " + KIT)
    print("        : -d (use direct flow - default is job flow)")
    print("        : -l turn on extra logging")
    print("        : -c request chunks one at a time rather than whole file")
    last_arg = ""
    for i, arg in enumerate(sys.argv):
        # print(f"Argument {i:>4}: {arg}")
        if arg == "TLS" or arg == "tls":
            TLS_ENABLED = True
        if arg == "-l":
            DEBUG_LOG = 1
            DEBUG_LOG_STRING = "1"
        if arg == "-d":
            USE_DIRECT_FLOW = True
        if last_arg == "-f":
            OTA_IMAGE_FILE = arg
        if last_arg == "-e":
            COMPANY_TOPIC_PREPEND = arg
        if last_arg == "-b":
            if ((arg == "amazon") | (arg == "a")):
                BROKER_ADDRESS = AMAZON_BROKER_ADDRESS
            if ((arg == "eclipse") | (arg == "e")):
                BROKER_ADDRESS = ECLIPSE_BROKER_ADDRESS
            if ((arg == "mosquitto") | (arg == "m")):
                BROKER_ADDRESS = MOSQUITTO_BROKER_ADDRESS
        if last_arg == "-c":
            file_in_chunks_chunk_size = int(arg)    # need to test range?
            request_file_in_chunks = True
        if last_arg == "-k":
            KIT = arg
        last_arg = arg

print("\n")
if TLS_ENABLED:
    print("   Using TLS")
else:
    print("   Using non-TLS")
print("   Using BROKER: " + BROKER_ADDRESS)
print("   Using    KIT: " + KIT)
print("   Using   File: " + OTA_IMAGE_FILE)
print("    extra debug: " + DEBUG_LOG_STRING)
if request_file_in_chunks == True:
    print("   Get file in chunks of size: " + str(file_in_chunks_chunk_size) )

SUBSCRIBER_PUBLISH_TOPIC = COMPANY_TOPIC_PREPEND + "/" + KIT + "/" + PUBLISHER_LISTEN_TOPIC
print("SUBSCRIBER_PUBLISH_TOPIC   : " + SUBSCRIBER_PUBLISH_TOPIC)

PUBLISHER_DIRECT_REQUEST_TOPIC = COMPANY_TOPIC_PREPEND + "/" + KIT + "/" + PUBLISHER_DIRECT_TOPIC
print("PUBLISHER_DIRECT_REQUEST_TOPIC: " + PUBLISHER_DIRECT_REQUEST_TOPIC)
print("\n")

# delete outfile to start clean
try:
    os.remove(OTA_IMAGE_FILE)
except Exception as e:
    # don't error if file does not exist
    print("")

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
    print("Connecting using TLS to '" + COMPANY_TOPIC_PREPEND + ":" + str(BROKER_PORT) + "'" + os.linesep)
else:
    BROKER_PORT = 1883
    print("Unencrypted connection to '" + COMPANY_TOPIC_PREPEND + ":" + str(BROKER_PORT) + "'" + os.linesep)

subscriber_loop()
