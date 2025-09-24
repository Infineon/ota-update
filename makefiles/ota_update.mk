################################################################################
# \file ota_update.mk
# \version 1.0
#
# \brief
# Makefile containing the Target dependencies for ota-update.
# Must be included in the Top-level application's Makefile.
#
################################################################################
# \copyright
# Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
# an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
#
# This software, including source code, documentation and related
# materials ("Software") is owned by Cypress Semiconductor Corporation
# or one of its affiliates ("Cypress") and is protected by and subject to
# worldwide patent protection (United States and foreign),
# United States copyright laws and international treaty provisions.
# Therefore, you may use this Software only as provided in the license
# agreement accompanying the software package from which you
# obtained this Software ("EULA").
# If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
# non-transferable license to copy, modify, and compile the Software
# source code solely for use in connection with Cypress's
# integrated circuit products.  Any reproduction, modification, translation,
# compilation, or representation of this Software except as specified
# above is prohibited without the express written permission of Cypress.
#
# Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
# reserves the right to make changes to the Software without notice. Cypress
# does not assume any liability arising out of the application or use of the
# Software or any product or circuit described in the Software. Cypress does
# not authorize its products for use in any products where a malfunction or
# failure of the Cypress product may reasonably be expected to result in
# significant property damage, injury or death ("High Risk Product"). By
# including Cypress's product in a High Risk Product, the manufacturer
# of such system or application assumes all risk of such use and in doing
# so agrees to indemnify Cypress against all liability.
################################################################################

# Find the Relative path between the OTA Makefiles and the Application's Build Directory
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)

    ota_path := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
    ota_dir:=$(shell cd $(shell dirname $(ota_path)); pwd)
    current_dir := $(shell PWD)

    # current_dir = $(shell pwd -P)
    RELATIVE_FILE1_FILE2:=$(shell perl -MFile::Spec::Functions=abs2rel -E 'say abs2rel(shift, shift)' $(ota_dir) $(current_dir))
else
    ota_dir := $(realpath $(patsubst %/,%,$(dir $(realpath $(lastword $(MAKEFILE_LIST))))))
    current_dir = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
    RELATIVE_FILE1_FILE2:=$(shell realpath --relative-to $(current_dir) $(ota_dir))
endif

# $(info setting ota_dir = $(ota_dir))
# $(info setting  current_dir= $(current_dir))
# $(info setting RELATIVE_FILE1_FILE2 = $(RELATIVE_FILE1_FILE2))

# Remove "APP_" from TARGET for this OTA internal Makefile
ACTUAL_TARGET?=$(subst APP_,,$(TARGET))

################################################################################
################################################################################
# Basic OTA configuration
################################################################################
################################################################################

# Bootloader Support
CY_BOOTLOADER?=MCUBOOT

# Initialize Protocol Support
OTA_HTTP_SUPPORT?=0
OTA_MQTT_SUPPORT?=0
OTA_BT_SUPPORT?=0

# Define App version

CY_COMPANY_ID?=0x1234
CY_PRODUCT_ID?=0x5678

APP_VERSION_REVISION?=0
APP_VERSION_SLOT?=0

ifeq ($(CY_MCUBOOT_OTA_IMAGE_VERIFICATION),0)
    # Define App version
    APP_BUILD_VERSION=$(APP_VERSION_MAJOR).$(APP_VERSION_MINOR).$(APP_VERSION_BUILD)
    DEFINES+=APP_BUILD_VERSION=$(APP_BUILD_VERSION)
else
    APP_BUILD_VERSION=0
endif

################################################################################
#
# OTA Build debugging
#
# OTA_BUILD_VERBOSE=1              Output info about Defines
# OTA_BUILD_FLASH_VERBOSE=1        Output info about Flash layout
# OTA_BUILD_COMPONENTS_VERBOSE=1   Output info about COMPONENTS, DEFINES, CY_IGNORE
# OTA_BUILD_DEFINES_VERBOSE=1      Output info about DEFINES
# OTA_BUILD_IGNORE_VERBOSE=1       Output info about CY_IGNORE
# OTA_BUILD_POST_VERBOSE=1         Output info about POSTBUILD values
#
#
###############################################################################
# Set to 1 to get more OTA Makefile / Build information for debugging build issues
OTA_BUILD_VERBOSE?=0

#
# OTA can support HTTP, MQTT and BT Transports, these can be added independently
#     Some platforms can only support BT transport, see TARGET sections below

###################################################
# Enable for HTTP OTA Support
# Implies using http library
# Use this to change the http library agent string
# OTA_HTTP_USER_AGENT_VALUE?="my_string"

# Use this to change the http max header size
# (default is 2048)
# OTA_HTTP_MAX_HDR_SIZE?=

# Use this to change to a custom configuration for the http library
OTA_HTTP_USE_CUSTOM_CONFIG?=0

###################################################
# Enable for MQTT OTA Support
# Use this to change the pre-pend value for the MQTT topic
# OTA_MQTT_COMPANY_TOPIC_PREPEND?=my_prepend

###################################################
# Enable BT support

# Default is "Do not test the APP_VERSION when downloading a TAR archive for updating"
# Enable to test APP_VERSION in TAR archive
CY_TEST_APP_VERSION_IN_TAR?=0

# Check atleast one protocol is enabled or not.
ifeq ($(OTA_HTTP_SUPPORT),0)
    ifeq ($(OTA_MQTT_SUPPORT),0)
        ifeq ($(OTA_BT_SUPPORT),0)
            $(error Must enable one of OTA_HTTP_SUPPORT: $(OTA_HTTP_SUPPORT),  OTA_MQTT_SUPPORT: $(OTA_MQTT_SUPPORT),  OTA_BT_SUPPORT:$(OTA_BT_SUPPORT) )
        endif
    endif
endif

COMPONENTS+=$(CY_BOOTLOADER)

####################################################################################
# HTTP / MQTT / Bluetooth specific COMPONENTS and DEFINES
####################################################################################
ifeq ($(OTA_MQTT_SUPPORT),0)
    ifeq ($(OTA_HTTP_SUPPORT),0)
        ifeq ($(OTA_BT_SUPPORT),1)
            OTA_BT_ONLY=1
        endif
    endif
endif

ifeq ($(OTA_HTTP_SUPPORT),1)
    COMPONENTS+=OTA_HTTP

    # If not defined, set to "\"ota-http-client\"" (default)
    ifeq ($(OTA_HTTP_USER_AGENT_VALUE),)
        OTA_HTTP_USER_AGENT_VALUE="\"ota-http-client\""
    endif
    # Set user agent name in all request headers with the specified name
    DEFINES += HTTP_USER_AGENT_VALUE=$(OTA_HTTP_USER_AGENT_VALUE)

    # If not defined, set to 2048 (default)
    ifeq ($(OTA_HTTP_MAX_HDR_SIZE),)
        OTA_HTTP_MAX_HDR_SIZE=2048
    endif
    # Configure response header maximum length with the specified value - HTTP
    DEFINES += HTTP_MAX_RESPONSE_HEADERS_SIZE_BYTES=$(OTA_HTTP_MAX_HDR_SIZE)

    # If not defined, set to 0 (default)
    ifeq ($(OTA_HTTP_USE_CUSTOM_CONFIG),)
        OTA_HTTP_USE_CUSTOM_CONFIG=0
    endif
    # Disable custom config header file
    ifeq ($(OTA_HTTP_USE_CUSTOM_CONFIG),0)
        DEFINES += HTTP_DO_NOT_USE_CUSTOM_CONFIG
    endif
    ifeq ($(OTA_BUILD_VERBOSE),1)
        $(info Makefile: OTA_HTTP_SUPPORT=1  OTA_HTTP_ONLY=$(OTA_HTTP_ONLY))
    endif
endif # OTA_HTTP_SUPPORT

ifeq ($(OTA_MQTT_SUPPORT),1)
    COMPONENTS+=OTA_MQTT

    # If MQTT Topic defined, set to over-ride value in cy_ota_config.h
    ifneq ($(OTA_MQTT_COMPANY_TOPIC_PREPEND),)
        COMPANY_TOPIC_PREPEND=\"$(OTA_MQTT_COMPANY_TOPIC_PREPEND)\"
        DEFINES+=COMPANY_TOPIC_PREPEND=$(COMPANY_TOPIC_PREPEND)
    endif
    ifeq ($(OTA_BUILD_VERBOSE),1)
        $(info Makefile: OTA_MQTT_SUPPORT=1  OTA_MQTT_ONLY=$(OTA_MQTT_ONLY)  COMPANY_TOPIC_PREPEND=$(COMPANY_TOPIC_PREPEND))
    endif
endif # OTA_MQTT_SUPPORT

ifeq ($(OTA_BT_SUPPORT),1)
    COMPONENTS+=OTA_BLUETOOTH
    COMPONENTS+=OTA_BLUETOOTH_NON_SECURE
    ifeq ($(OTA_BT_ONLY),1)
        DEFINES+=OTA_BT_ONLY
    endif

    ifeq ($(OTA_BUILD_VERBOSE),1)
        $(info Makefile: OTA_BT_SUPPORT  =1  OTA_BT_ONLY=$(OTA_BT_ONLY))
    endif
endif # OTA_BT_SUPPORT

###################################################################################################
# Ignore various directories based on BLE, WIFI, MQTT support
# This OTA Test Application supports multiple transport combinations,
#   so we must ignore various libraries in order to bypass any
#   multiple declarations / definitions / functions
###################################################################################################
#
# Ignore during build, but allow getlibs to pull in all the libraries
#
ifneq ($(MAKECMDGOALS),getlibs)
    # Check for BT only Build
    ifeq ($(OTA_BT_ONLY),1)
        ifeq ($(OTA_BUILD_VERBOSE),1)
            $(info Makefile: OTA_BT_ONLY=1         IGNORE WiFi libraries)
        endif
    endif # OTA_BT_ONLY

    CY_IGNORE+=$(SEARCH_aws-iot-device-sdk-port)/source/ota
    CY_IGNORE+=$(SEARCH_aws-iot-device-sdk-embedded-C)/libraries/aws/ota-for-aws-iot-embedded-sdk
endif    # not getlibs

###################################################################################################
#
# OTA Support Specifics
#
###################################################################################################

##############################
# OTA internal extras for debugging
##############################

# Enable for debugging http / ssl
ifeq ($(OTA_HTTP_DEBUG),1)
    DEFINES += ENABLE_HTTP_CLIENT_LOGS ENABLE_SECURE_SOCKETS_LOGS ENABLE_AWS_PORT_LOGS
endif

# Enable for debugging MQTT / ssl
ifeq ($(OTA_MQTT_DEBUG),1)
    DEFINES += ENABLE_MQTT_LOGS ENABLE_SECURE_SOCKETS_LOGS ENABLE_AWS_PORT_LOGS
endif

# Enable for BT Module debugging
ifeq ($(OTA_BT_DEBUG),1)
    DEFINES+= ENABLE_FW_LOGS
    DEFINES+= CYBT_PLATFORM_TRACE_ENABLE=1
endif

ifeq ($(DISABLE_MBEDTLS_ACCELERATION),1)
    DEFINES+=DISABLE_MBEDTLS_ACCELERATION
endif

###################################
# Additional Include Directories
###################################
INCLUDES+=$(SEARCH_mqtt)/src\
          $(SEARCH_mqtt)/cyport/include\
