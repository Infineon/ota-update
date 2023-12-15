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
# Copyright 2018-2022 Cypress Semiconductor Corporation
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
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
ACTUAL_TARGET=$(subst APP_,,$(TARGET))

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
APP_BUILD_VERSION=$(APP_VERSION_MAJOR).$(APP_VERSION_MINOR).$(APP_VERSION_BUILD)
DEFINES+=APP_BUILD_VERSION=$(APP_BUILD_VERSION)

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
# Enable for BT Secure Support
OTA_BT_SECURE?=0

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
    ifeq ($(OTA_BT_SECURE),1)
        COMPONENTS+=OTA_BLUETOOTH_SECURE
    else
        COMPONENTS+=OTA_BLUETOOTH_NON_SECURE
    endif # OTA_BT_SECURE

    ifeq ($(OTA_BT_ONLY),1)
        DEFINES+=OTA_BT_ONLY
    endif

    ifeq ($(OTA_BUILD_VERBOSE),1)
        $(info Makefile: OTA_BT_SUPPORT  =1  OTA_BT_ONLY=$(OTA_BT_ONLY)  OTA_BT_SECURE=$(OTA_BT_SECURE))
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
        CY_IGNORE+=$(SEARCH_mbedtls)/3rdparty
        CY_IGNORE+=$(SEARCH_mbedtls)/configs
        CY_IGNORE+=$(SEARCH_mbedtls)/programs
        CY_IGNORE+=$(SEARCH_mbedtls)/scripts
        CY_IGNORE+=$(SEARCH_mbedtls)/tests
        CY_IGNORE+=$(SEARCH_mbedtls)/doxygen
        CY_IGNORE+=$(SEARCH_mbedtls)/library/net_sockets.c
    endif # OTA_BT_ONLY

    ifeq ($(OTA_BT_ONLY),1)
        ifneq ($(OTA_BT_SECURE),1)
            ifeq ($(OTA_BUILD_VERBOSE),1)
                $(info Makefile: OTA_BT_SUPPORT=1 OTA_BT_SECURE=0     IGNORE secure libraries)
            endif
        endif # OTA_BT_SECURE
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
