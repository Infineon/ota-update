################################################################################
# \file target_ota.mk
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
$(info TARGET = $(TARGET))
$(info ACTUAL = $(ACTUAL_TARGET))

################################################################################
################################################################################
# Basic OTA configuration
################################################################################
################################################################################

# Bootloader Support
BOOTLOADER?=MCUBOOT

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
OTA_HTTP_SUPPORT?=0

# Use this to change the http library agent string
# OTA_HTTP_USER_AGENT_VALUE?="my_string"

# Use this to change the http max header size
# (default is 2048)
# OTA_HTTP_MAX_HDR_SIZE?=

# Use this to change to a custom configuration for the http library
OTA_HTTP_USE_CUSTOM_CONFIG?=0

###################################################
# Enable for MQTT OTA Support
# Implies using http and mqtt libraries
OTA_MQTT_SUPPORT?=0

# Use this to change the pre-pend value for the MQTT topic
# OTA_MQTT_COMPANY_TOPIC_PREPEND?=my_prepend

###################################################
# Enable BT support
OTA_BT_SUPPORT?=0

# Enable for BT Secure Support
OTA_BT_SECURE?=0

# Default is "Do not test the APP_VERSION when downloading a TAR archive for updating"
# Enable to test APP_VERSION in TAR archive
OTA_TEST_APP_VERSION_IN_TAR?=0


################################################################################
# Check for Transport "Only" after TARGET section
#   as some platforms specify ONLY
################################################################################

# If HTTP only, turn off other transports
ifeq ($(OTA_HTTP_ONLY),1)
OTA_HTTP_SUPPORT=1
OTA_MQTT_SUPPORT=0
OTA_MQTT_ONLY=0
OTA_BT_SUPPORT=0
OTA_BT_ONLY=0
else
OTA_HTTP_ONLY=0
endif

# If MQTT only, turn off other transports
ifeq ($(OTA_MQTT_ONLY),1)
OTA_MQTT_SUPPORT=1
OTA_HTTP_SUPPORT=0
OTA_HTTP_ONLY=0
OTA_BT_SUPPORT=0
OTA_BT_ONLY=0
else
OTA_MQTT_ONLY=0
endif

# If BT only, turn off other transports
ifeq ($(OTA_BT_ONLY),1)
OTA_BT_SUPPORT=1
OTA_HTTP_SUPPORT=0
OTA_HTTP_ONLY=0
OTA_MQTT_SUPPORT=0
OTA_MQTT_ONLY=0
else
OTA_BT_ONLY=0
endif

ifeq ($(OTA_HTTP_SUPPORT),0)
ifeq ($(OTA_MQTT_SUPPORT),0)
ifeq ($(OTA_BT_SUPPORT),0)
$(error Must enable one of OTA_HTTP_SUPPORT: $(OTA_HTTP_SUPPORT),  OTA_MQTT_SUPPORT: $(OTA_MQTT_SUPPORT),  OTA_BT_SUPPORT:$(OTA_BT_SUPPORT) )
endif
endif
endif

COMPONENTS+=$(BOOTLOADER)

################################################################################
# Target specific COMPONENTS, DEFINES and Makefile defines
################################################################################

#----------------------------------
# CYW920829M2EVK-02
#----------------------------------
ifeq ($(ACTUAL_TARGET), CYW920829M2EVK-02)
$(info Makefile: CYW920829M2EVK-02)
    OTA_PLATFORM=CYW20829
endif

ifeq ($(OTA_PLATFORM), CYW20829)
$(info Makefile: CYW20829)
    # Default to XIP Swap
    OTA_FLASH_MAP?=$(RELATIVE_FILE1_FILE2)/../configs/flashmap/cyw20829_xip_swap_single.json
endif

#----------------------------------
# CY8CKIT-062S2-43012
#----------------------------------
ifeq ($(ACTUAL_TARGET), CY8CKIT-062S2-43012)
$(info Makefile: CY8CKIT-062S2-43012)
# 2M internal flash
# 43012
# Available builds:
#   - HTTP, MQTT, and / or BT
#   - Flash location
#     - Internal Only Flash
#     - Internal + External Flash

    # Default to Internal + External Flash, Swap, Single Image
    OTA_FLASH_MAP?=$(RELATIVE_FILE1_FILE2)/../configs/flashmap/psoc62_2m_ext_swap_single.json

    OTA_PLATFORM=PSOC_062_2M
endif

#----------------------------------
# CY8CPROTO-062-4343W
#----------------------------------
ifeq ($(ACTUAL_TARGET), CY8CPROTO-062-4343W)
$(info Makefile: CY8CPROTO-062-4343W)
# 2M internal flash
# 4343W
# Available builds:
#   - HTTP, MQTT, and / or BT
#   - Flash location
#     - Internal Only Flash
#     - Internal + External Flash

    # Default to Internal, Swap, Single Image
    OTA_FLASH_MAP?=$(RELATIVE_FILE1_FILE2)/../configs/flashmap/psoc62_2m_int_swap_single.json

    OTA_PLATFORM=PSOC_062_2M
endif

#----------------------------------
# CY8CPROTO-062S3-4343W
#----------------------------------
ifeq ($(ACTUAL_TARGET), CY8CPROTO-062S3-4343W)
$(info Makefile: CY8CPROTO-062S3-4343W)
# 512k internal flash
# 4343W
# Available builds:
#   - HTTP, MQTT, and / or BT
#   - Flash location
#     - Internal Only Flash
#     - Internal + External Flash
#     - External XIP

    OTA_FLASH_MAP?=$(RELATIVE_FILE1_FILE2)/../configs/flashmap/psoc62_512k_xip_swap_single.json

    OTA_PLATFORM=PSOC_062_512K
endif

#----------------------------------
# CY8CEVAL-062S2-CYW943439M2IPA1
#----------------------------------
ifeq ($(ACTUAL_TARGET), CY8CEVAL-062S2-CYW943439M2IPA1)
$(info Makefile: CY8CEVAL-062S2-CYW943439M2IPA1)
# 2M internal flash
# 43439
# Available builds:
#   - HTTP, MQTT, and / or BT
#   - Flash location
#     - Internal Only Flash
#     - Internal + External Flash

    # Default to Internal + External Flash, Swap, Single Image
    OTA_FLASH_MAP?=$(RELATIVE_FILE1_FILE2)/../configs/flashmap/psoc62_2m_ext_swap_single.json

    OTA_PLATFORM=PSOC_062_2M
endif

#----------------------------------
# CY8CEVAL-062S2-LAI-4373M2
#----------------------------------
ifeq ($(ACTUAL_TARGET), CY8CEVAL-062S2-LAI-4373M2)
$(info Makefile: CY8CEVAL-062S2-LAI-4373M2)
# 2M internal flash
# 4373
# Available builds:
#   - HTTP, MQTT, and / or BT
#   - Flash location
#     - Internal Only Flash
#     - Internal + External Flash

    # Default to Internal + External Flash, Swap, Single Image
    OTA_FLASH_MAP?=$(RELATIVE_FILE1_FILE2)/../configs/flashmap/psoc62_2m_ext_swap_single.json

    OTA_PLATFORM=PSOC_062_2M
endif

#----------------------------------
# CY8CEVAL-062S2-MUR-43439M2
#----------------------------------
ifeq ($(ACTUAL_TARGET), CY8CEVAL-062S2-MUR-43439M2)
$(info Makefile: CY8CEVAL-062S2-MUR-43439M2)
# 2M internal flash
# 43439
# Available builds:
#   - HTTP, MQTT, and / or BT
#   - Flash location
#     - Internal Only Flash
#     - Internal + External Flash

    # Default to Internal + External Flash, Swap, Single Image
    OTA_FLASH_MAP?=$(RELATIVE_FILE1_FILE2)/../configs/flashmap/psoc62_2m_ext_swap_single.json

    OTA_PLATFORM=PSOC_062_2M
endif

#----------------------------------
# CY8CKIT-062-BLE
#----------------------------------
ifeq ($(ACTUAL_TARGET), CY8CKIT-062-BLE)
$(info Makefile: CY8CKIT-062-BLE)
#
#    TODO: [in development, requires MTB 3.0]
#
# 1M internal flash
# Available builds:
#   BLUETOOTH ONLY
#   Internal Flash Only

    # Default to int Swap, BLESS requires cm0p code inclusion
    OTA_FLASH_MAP?=$(RELATIVE_FILE1_FILE2)/../configs/flashmap/psoc62_1m_cm0_int_swap_single.json

    OTA_PLATFORM=PSOC_062_1M
endif

#----------------------------------
# CY8CPROTO-063-BLE
#----------------------------------
ifeq ($(ACTUAL_TARGET), CY8CPROTO-063-BLE)
$(info Makefile: CY8CPROTO-063-BLE)
#
#    TODO: [in development, requires MTB 3.0]
#
# 1M internal flash
# Available builds:
#   BLUETOOTH ONLY
#   Internal Flash Only

    # Default to int Swap, BLESS requires cm0p code inclusion
    OTA_FLASH_MAP?=$(RELATIVE_FILE1_FILE2)/../configs/flashmap/psoc63_1m_cm0_int_swap_single.json

    OTA_PLATFORM=PSOC_063_1M
endif

#----------------------------------
# CYBLE-416045-EVAL
#----------------------------------
ifeq ($(ACTUAL_TARGET), CYBLE-416045-EVAL)
$(info Makefile: CYBLE-416045-EVAL)
#
#    TODO: [in development, requires MTB 3.0]
#
# 1M internal flash
#   BLUETOOTH ONLY
#   Internal Flash Only

    # Default to int Swap, BLESS requires cm0p code inclusion
    OTA_FLASH_MAP?=$(RELATIVE_FILE1_FILE2)/../configs/flashmap/psoc63_1m_cm0_int_swap_single.json

    OTA_PLATFORM=PSOC_063_1M
endif

#----------------------------------
# CY8CKIT-064B0S2-4343W
#----------------------------------
ifeq ($(ACTUAL_TARGET), CY8CKIT-064B0S2-4343W)
$(info Makefile: CY8CKIT-064B0S2-4343W)
# 2M internal flash
# 4343W
# Available builds:
#   HTTP, MQTT, and / or BLUETOOTH
#   Internal + External Flash

    # Default to ext Swap, we need to define for later in the Makefile
    # Although we don't actually use it.
    OTA_FLASH_MAP?=$(RELATIVE_FILE1_FILE2)/../configs/flashmap/psoc64_2m_ext_swap_single.json

    OTA_PLATFORM=PSOC_064_2M

    # Policy name here for reference when provisioning CY8CKIT-064B0S2-4343W
    CY_SECURE_POLICY_NAME = policy_single_CM0_CM4_smif_swap
endif


####################################################################################
# HTTP / MQTT / Bluetooth specific COMPONENTS and DEFINES
####################################################################################

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
    ifneq ($(OTA_HTTP_USE_CUSTOM_CONFIG),0)
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

###########################
# BLUETOOTH support IGNORE
###########################

# Check for BT only Build
ifeq ($(OTA_BT_ONLY),1)
ifeq ($(OTA_BUILD_VERBOSE),1)
$(info Makefile: OTA_BT_ONLY=1         IGNORE WiFi libraries)
endif
    # For no HTTP or MQTT support, we ignore WiFi libs
    # These libs are only needed for WiFi transports
    CY_IGNORE+=$(SEARCH_aws-iot-device-sdk-embedded-C)
    CY_IGNORE+=$(SEARCH_aws-iot-device-sdk-port)
    CY_IGNORE+=$(SEARCH_http-client)
    CY_IGNORE+=$(SEARCH_mqtt)
    CY_IGNORE+=$(SEARCH_wifi-connection-manager)
    CY_IGNORE+=$(SEARCH_wifi-core-freertos-lwip-mbedtls)
    CY_IGNORE+=$(SEARCH_wpa3-external-supplicant)

    CY_IGNORE+=$(SEARCH_lwip)
    CY_IGNORE+=$(SEARCH_lwip-freertos-integration)
    CY_IGNORE+=$(SEARCH_lwip-network-interface-integration)
    CY_IGNORE+=$(SEARCH_whd-bsp-integration)
    CY_IGNORE+=$(SEARCH_wifi-host-driver)

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
        CY_IGNORE+=$(SEARCH_mbedtls)
        CY_IGNORE+=$(SEARCH_secure-sockets)
        CY_IGNORE+=$(SEARCH_cy-mbedtls-acceleration)
        CY_IGNORE+=$(SEARCH_wpa3-external-supplicant)
    endif # OTA_BT_SECURE
endif # OTA_BT_SUPPORT

# Common NO BT support IGNORES
ifneq ($(OTA_BT_SUPPORT),1)
$(info Makefile: *NO* OTA_BT_ONLY IGNORE BT  libraries)
    CY_IGNORE+=$(SEARCH_btstack-integration)
    CY_IGNORE+=$(SEARCH_btstack)
endif

######################
# HTTP support IGNORES
######################

ifeq ($(OTA_HTTP_SUPPORT),1)
ifneq ($(OTA_MQTT_SUPPORT),1)
ifeq ($(OTA_BUILD_VERBOSE),1)
$(info Makefile: OTA_HTTP_SUPPORT  ONLY - IGNORE MQTT library)
endif
    # If we ignore the MQTT library, we fail the build:			#IMP_CHECK
    # In file included from ../mtb_shared/aws-iot-device-sdk-embedded-C/202103.00/libraries/standard/coreMQTT/source/core_mqtt.c:30:
    # ../mtb_shared/aws-iot-device-sdk-embedded-C/202103.00/libraries/standard/coreMQTT/source/include/core_mqtt.h:35:14: fatal error: core_mqtt_config.h: No such file or directory
    # 35 |     #include "core_mqtt_config.h"
    # CY_IGNORE+=$(SEARCH_mqtt)
endif # NO OTA_MQTT_SUPPORT
endif # OTA_HTTP_SUPPORT

###################
# MQTT support IGNORES
###################

ifeq ($(OTA_MQTT_SUPPORT),1)
ifneq ($(OTA_HTTP_SUPPORT),1)
ifeq ($(OTA_BUILD_VERBOSE),1)
$(info Makefile: OTA_MQTT_SUPPORT   IGNORE HTTP library)
endif
    CY_IGNORE+=$(SEARCH_http-client)
endif
endif # OTA_MQTT_SUPPORT

################################
# IGNORES for specific platforms
################################

# Ignore libraries that 20829 does not use
ifeq ($(OTA_PLATFORM), CYW20829)
ifeq ($(OTA_BUILD_VERBOSE),1)
$(info Makefile: CYW920829 IGNORE capsense, emeeprom, serial-flash libraries)
endif
# Ignore other libraries that CYW920829 will not use
    CY_IGNORE+=$(SEARCH_capsense)
    CY_IGNORE+=$(SEARCH_emeeprom)
endif
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

# Enable for debugging http / ssl
ifeq ($(OTA_MQTT_DEBUG),1)
    DEFINES += ENABLE_MQTT_LOGS ENABLE_SECURE_SOCKETS_LOGS
endif

# Enable for BT Module debugging
ifeq ($(OTA_BT_DEBUG),1)
    DEFINES+= ENABLE_FW_LOGS
    DEFINES+= CYBT_PLATFORM_TRACE_ENABLE=1
endif

ifeq ($(DISABLE_MBEDTLS_ACCELERATION),1)
    DEFINES+=DISABLE_MBEDTLS_ACCELERATION
endif

#############################
# Check for Required defines
#############################

# MUST provide platform type
# Must be one of these names
OTA_PSOC_062_SUPPORTED_PLATFORMS="PSOC_062_512K PSOC_062_1M PSOC_062_2M "
OTA_PSOC_063_SUPPORTED_PLATFORMS="PSOC_063_1M "
OTA_PSOC_064_SUPPORTED_PLATFORMS="PSOC_064_2M "
OTA_PSOC_06X_SUPPORTED_PLATFORMS=$(OTA_PSOC_062_SUPPORTED_PLATFORMS)$(OTA_PSOC_063_SUPPORTED_PLATFORMS)$(OTA_PSOC_064_SUPPORTED_PLATFORMS)
OTA_OTHER_SUPPORTED_PLATFORMS="CYW20829 "
OTA_SUPPORTED_PLATFORMS=$(OTA_PSOC_06X_SUPPORTED_PLATFORMS)$(OTA_OTHER_SUPPORTED_PLATFORMS)

# Add OTA_PLATFORM in DEFINES for platform-specific code
DEFINES+=$(OTA_PLATFORM)
# for use when running flashmap.py
FLASHMAP_PLATFORM=$(OTA_PLATFORM)

# For generalized PSOC based specifics
ifneq ($(findstring $(OTA_PLATFORM), $(OTA_PSOC_06X_SUPPORTED_PLATFORMS)),)
# ifeq ($(OTA_BUILD_VERBOSE),2)
$(info FOUND $(OTA_PLATFORM) Use COMPONENT_OTA_PSOC_062)
# endif
    COMPONENTS+=OTA_PSOC_062
endif

###################################
# Additional Include Directories
###################################
INCLUDES+=$(SEARCH_mqtt)/src\
          $(SEARCH_mqtt)/cyport/include\

###################################
# Misc OTA defines
###################################
DEFINES+=\
    OTA_SUPPORT=1 \
    APP_VERSION_MAJOR=$(OTA_APP_VERSION_MAJOR)\
    APP_VERSION_MINOR=$(OTA_APP_VERSION_MINOR)\
    APP_VERSION_BUILD=$(OTA_APP_VERSION_BUILD)

include $(RELATIVE_FILE1_FILE2)/mcuboot_flashmap.mk
