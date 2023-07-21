################################################################################
# \file mcuboot_flashmap.mk
# \version 1.0
#
# \brief
# Makefile containing the MCUboot flashmap dependencies.
# Will be included in the Top level application through target_ota.mk
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
#################################################################################

###################################################################################################
# OTA SUPPORT
###################################################################################################
# This is the size of the header that imgtool will pre-pend to application
# Must match with MCUBoot
# Do not change
MCUBOOT_HEADER_SIZE=0x400

# Default Bootstrap size for MCUBootloader
APP_BOOTSTRAP_SIZE=0x2400

# Internal and external erased flash values
CY_INTERNAL_FLASH_ERASE_VALUE=0x00
CY_EXTERNAL_FLASH_ERASE_VALUE=0xFF

###################################################################################################
# OTA Flashmap JSON file Usage
###################################################################################################
#
# flashmap.py creates flashmap.mk, a number of defines which are then included in the build.
# It is important to include the defines here (and not just in cy_flash_map.c) as they are used in POSTBUILD processing
#
# Look in configs/flashmap for an appropriate JSON file
#
# CYW920829
#    Single image JSON files, always external flash
#        configs/flashmap/cyw20829_ext_overwrite_single.json
#        configs/flashmap/cyw20829_ext_swap_single.json
# CY8CPROTO-062S3-4343W
#    Single image JSON files, internal flash
#        configs/flashmap/psoc62s3_int_overwrite_single.json
#        configs/flashmap/psoc62s3_int_swap_single.json
#    Single image JSON files, XIP from external flash
#        configs/flashmap/psoc62s3_xip_swap_single.json
# CY8CKIT-062-BLE
#    Single image JSON files, internal flash
#        configs/flashmap/psoc62_1m_int_swap_single.json
#    Single image JSON files, XIP from external flash
#        configs/flashmap/psoc62_1m_xip_swap_single.json
# Other PSoC6 targets
#    Single image JSON files, internal flash only
#        configs/flashmap/psoc62_int_overwrite_single.json
#        configs/flashmap/psoc62_int_swap_single.json
#    Single image JSON files, internal + external flash
#        configs/flashmap/psoc62_ext_overwrite_single.json
#        configs/flashmap/psoc62_ext_swap_single.json
#    Single image JSON files, XIP from external flash
#        configs/flashmap/psoc62_xip_swap_single.json
#
# CY8CPROTO-063-BLE
#    Single image JSON files, internal flash
#        configs/flashmap/psoc63_1m_int_swap_single.json
#    Single image JSON files, XIP from external flash
#        configs/flashmap/psoc63_1m_xip_swap_single.json
#
# CY8CKIT-064B0S2-4343W
#    DOES NOT USE JSON FILE at this time
#
# Example output that is piped into flashmap.mk for an XIP application:
#
# AUTO-GENERATED FILE, DO NOT EDIT. ALL CHANGES WILL BE LOST!
# FLASH_AREA_BOOTLOADER_DEV_ID :=FLASH_DEVICE_INTERNAL_FLASH
# FLASH_AREA_BOOTLOADER_START :=0x000000
# FLASH_AREA_BOOTLOADER_SIZE :=0x018000
# FLASH_AREA_IMG_1_PRIMARY_DEV_ID :=FLASH_DEVICE_EXTERNAL_FLASH(CY_BOOT_EXTERNAL_DEVICE_INDEX)
# FLASH_AREA_IMG_1_PRIMARY_START :=0x000000
# FLASH_AREA_IMG_1_PRIMARY_SIZE :=0x140200
# FLASH_AREA_IMG_1_SECONDARY_DEV_ID :=FLASH_DEVICE_EXTERNAL_FLASH(CY_BOOT_EXTERNAL_DEVICE_INDEX)
# FLASH_AREA_IMG_1_SECONDARY_START :=0x180000
# FLASH_AREA_IMG_1_SECONDARY_SIZE :=0x140200
# FLASH_AREA_IMAGE_SWAP_STATUS_DEV_ID :=FLASH_DEVICE_INTERNAL_FLASH
# FLASH_AREA_IMAGE_SWAP_STATUS_START :=0x018000
# FLASH_AREA_IMAGE_SWAP_STATUS_SIZE :=0x003c00
# FLASH_AREA_IMAGE_SCRATCH_DEV_ID :=FLASH_DEVICE_EXTERNAL_FLASH(CY_BOOT_EXTERNAL_DEVICE_INDEX)
# FLASH_AREA_IMAGE_SCRATCH_START :=0x440000
# FLASH_AREA_IMAGE_SCRATCH_SIZE :=0x080000
# MCUBOOT_IMAGE_NUMBER := 1
# MCUBOOT_MAX_IMG_SECTORS := 32
# USE_EXTERNAL_FLASH := 1
# USE_XIP := 1
#
# Always update the flash area definitions.
# Note: The build system will not re-build all files as the defines have not changed since last build.
#$(shell rm -f flashmap.mk)

ifeq ($(OTA_PLATFORM),PSOC_064_2M)
# CY8CKIT-064B0S2-4343W
#    Single image JSON File, int + ext flash
#     flashmap.py does not support this platform at this time
$(info flashmap.py does not support PSOC_064_2M - We write out directly here!)            #IMP_CHECK

# Define FLASHMAP_PYTHON_SCRIPT as nothing so it is not executed after start.mk at end of Makefile
FLASHMAP_PYTHON_SCRIPT=

$(shell echo "# AUTO-GENERATED FILE, DO NOT EDIT. ALL CHANGES WILL BE LOST!" > flashmap.mk)
$(shell echo "FLASH_AREA_BOOTLOADER_DEV_ID :=FLASH_DEVICE_INTERNAL_FLASH" >> flashmap.mk)
$(shell echo "FLASH_AREA_BOOTLOADER_START :=0x1e0000" >> flashmap.mk)
$(shell echo "FLASH_AREA_BOOTLOADER_SIZE :=0x018000" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMG_1_PRIMARY_DEV_ID :=FLASH_DEVICE_INTERNAL_FLASH" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMG_1_PRIMARY_START :=0x000000" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMG_1_PRIMARY_SIZE :=0x1c8000" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMG_1_SECONDARY_DEV_ID :=FLASH_DEVICE_EXTERNAL_FLASH(CY_BOOT_EXTERNAL_DEVICE_INDEX)" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMG_1_SECONDARY_START :=0x038400" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMG_1_SECONDARY_SIZE :=0x1c8000" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMAGE_SWAP_STATUS_DEV_ID :=FLASH_DEVICE_INTERNAL_FLASH" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMAGE_SWAP_STATUS_START :=0x1cc000" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMAGE_SWAP_STATUS_SIZE :=0x006c00" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMAGE_SCRATCH_DEV_ID :=FLASH_DEVICE_EXTERNAL_FLASH(CY_BOOT_EXTERNAL_DEVICE_INDEX)" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMAGE_SCRATCH_START :=0x280000" >> flashmap.mk)
$(shell echo "FLASH_AREA_IMAGE_SCRATCH_SIZE :=0x0c0000" >> flashmap.mk)
$(shell echo "MCUBOOT_IMAGE_NUMBER := 1" >> flashmap.mk)
$(shell echo "MCUBOOT_MAX_IMG_SECTORS := 3648" >> flashmap.mk)
$(shell echo "USE_EXTERNAL_FLASH := 1" >> flashmap.mk)

else
# non-PSOC_064_2M here
FLASHMAP_PYTHON_SCRIPT=flashmap.py

ifneq ($(findstring $(MAKECMDGOALS), build program all),)
ifeq ($(CY_PYTHON_PATH),)
CY_PYTHON_PATH=$(shell which python)
$(info setting CY_PYTHON_PATH = $(CY_PYTHON_PATH))
endif # checking for python path

ifneq ($(FLASHMAP_PYTHON_SCRIPT),)
$(info : OTA_FLASH_MAP = $(OTA_FLASH_MAP))

$(info "flashmap.py $(CY_PYTHON_PATH) $(RELATIVE_FILE1_FILE2)/../scripts/$(FLASHMAP_PYTHON_SCRIPT) -p $(FLASHMAP_PLATFORM) -i $(OTA_FLASH_MAP) > flashmap.mk")
$(shell $(CY_PYTHON_PATH) $(RELATIVE_FILE1_FILE2)/../scripts/$(FLASHMAP_PYTHON_SCRIPT) -p $(FLASHMAP_PLATFORM) -i $(OTA_FLASH_MAP) > flashmap.mk)
flash_map_status=$(shell if [ -s "flashmap.mk" ]; then echo "success"; fi )
ifeq ($(flash_map_status),)
$(info "")
$(error Failed to create flashmap.mk !)
$(info "")
endif # check for flashmap.mk created
endif # check for flashmap name
endif # check for build command

endif # PSOC_064_2M check

flash_map_mk_exists=$(shell if [ -s "flashmap.mk" ]; then echo "success"; fi )
ifneq ($(flash_map_mk_exists),)
$(info include flashmap.mk)
include ./flashmap.mk
endif # flash_map_mk_exists

############################
# IF FLASH_MAP sets USE_XIP,
#    we are executing code
#    from external flash
############################

ifeq ($(USE_XIP),1)
ifeq ($(OTA_BUILD_VERBOSE),1)
$(info Makefile: USE_XIP=1)
endif

CY_RUN_CODE_FROM_XIP=1

# For IAR XIP builds, we need to move some code to RAM            #IMP_CHECK
ifeq ($(TOOLCHAIN),IAR)
ifeq ($(OTA_BUILD_VERBOSE),1)
$(info Makefile: ADD CY_INIT_CODECOPY_ENABLE for IAR XIP)
endif
    DEFINES+=CY_INIT_CODECOPY_ENABLE
endif

# When running from external flash
# That we need to turn off XIP and enter critical section when accessing SMIF
#  NOTE: CYW920829 does not need this. Generalize for now,
#        we may need to change this criteria for future devices
ifneq ($(OTA_PLATFORM),CYW20829)
ifeq ($(OTA_BUILD_VERBOSE),1)
$(info Makefile: OTA_PLATFORM = CYW20829)
endif
CY_XIP_SMIF_MODE_CHANGE=1

# Since we are running hybrid (some in RAM, some in External FLash),    #IMP_CHECK
#   we need to override the WEAK functions in CYHAL
DEFINES+=CYHAL_DISABLE_WEAK_FUNC_IMPL=1
endif # Not CYW20829
endif # USE_XIP

# TODO: Can we base this on the flash area at RUNTIME rather than defining it at build time?
#       Flash areas / images are allowed to be placed willy-nilly
ifeq ($(FLASH_AREA_IMG_1_SECONDARY_DEV_ID),FLASH_DEVICE_INTERNAL_FLASH)
    FLASH_ERASE_SECONDARY_SLOT_VALUE= $(CY_INTERNAL_FLASH_ERASE_VALUE)
else
    FLASH_ERASE_SECONDARY_SLOT_VALUE= $(CY_EXTERNAL_FLASH_ERASE_VALUE)
endif # SECONDARY_DEV_ID

# Application MUST provide a flash map
ifneq ($(MAKECMDGOALS),getlibs)
ifeq ($(OTA_FLASH_MAP),)
$(info "")
$(error Application makefile must define OTA_FLASH_MAP. For more info, see <ota-update>/configs/flashmap/MCUBoot_Build_Commands.md)
$(info "")
endif
endif

###################################
# The Defines for the flash map
###################################
DEFINES+=\
    MCUBOOT_MAX_IMG_SECTORS=$(MCUBOOT_MAX_IMG_SECTORS)\
    MCUBOOT_IMAGE_NUMBER=$(MCUBOOT_IMAGE_NUMBER)\
    FLASH_AREA_BOOTLOADER_DEV_ID="$(FLASH_AREA_BOOTLOADER_DEV_ID)"\
    FLASH_AREA_BOOTLOADER_START=$(FLASH_AREA_BOOTLOADER_START)\
    FLASH_AREA_BOOTLOADER_SIZE=$(FLASH_AREA_BOOTLOADER_SIZE)\
    FLASH_AREA_IMG_1_PRIMARY_DEV_ID="$(FLASH_AREA_IMG_1_PRIMARY_DEV_ID)"\
    FLASH_AREA_IMG_1_PRIMARY_START=$(FLASH_AREA_IMG_1_PRIMARY_START) \
    FLASH_AREA_IMG_1_PRIMARY_SIZE=$(FLASH_AREA_IMG_1_PRIMARY_SIZE) \
    FLASH_AREA_IMG_1_SECONDARY_DEV_ID="$(FLASH_AREA_IMG_1_SECONDARY_DEV_ID)"\
    FLASH_AREA_IMG_1_SECONDARY_START=$(FLASH_AREA_IMG_1_SECONDARY_START) \
    FLASH_AREA_IMG_1_SECONDARY_SIZE=$(FLASH_AREA_IMG_1_SECONDARY_SIZE)

ifneq ($(FLASH_AREA_IMAGE_SWAP_STATUS_DEV_ID),)
    DEFINES+=\
        FLASH_AREA_IMAGE_SWAP_STATUS_DEV_ID="$(FLASH_AREA_IMAGE_SWAP_STATUS_DEV_ID)"\
        FLASH_AREA_IMAGE_SWAP_STATUS_START=$(FLASH_AREA_IMAGE_SWAP_STATUS_START)\
        FLASH_AREA_IMAGE_SWAP_STATUS_SIZE=$(FLASH_AREA_IMAGE_SWAP_STATUS_SIZE)
endif

ifneq ($(FLASH_AREA_IMAGE_SCRATCH_DEV_ID),)
    DEFINES+=\
        FLASH_AREA_IMAGE_SCRATCH_DEV_ID="$(FLASH_AREA_IMAGE_SCRATCH_DEV_ID)"\
        FLASH_AREA_IMAGE_SCRATCH_START=$(FLASH_AREA_IMAGE_SCRATCH_START)\
        FLASH_AREA_IMAGE_SCRATCH_SIZE=$(FLASH_AREA_IMAGE_SCRATCH_SIZE)
endif


ifeq ($(MCUBOOT_IMAGE_NUMBER),2)
    DEFINES+=\
        FLASH_AREA_IMG_2_PRIMARY_DEV_ID=$(FLASH_AREA_IMG_2_PRIMARY_DEV_ID)\
        FLASH_AREA_IMG_2_PRIMARY_START=$(FLASH_AREA_IMG_2_PRIMARY_START) \
        FLASH_AREA_IMG_2_PRIMARY_SIZE=$(FLASH_AREA_IMG_2_PRIMARY_SIZE) \
        FLASH_AREA_IMG_2_SECONDARY_DEV_ID=$(FLASH_AREA_IMG_2_SECONDARY_DEV_ID) \
        FLASH_AREA_IMG_2_SECONDARY_START=$(FLASH_AREA_IMG_2_SECONDARY_START) \
        FLASH_AREA_IMG_2_SECONDARY_SIZE=$(FLASH_AREA_IMG_2_SECONDARY_SIZE)
endif

ifeq ($(USE_EXTERNAL_FLASH),1)
    DEFINES+=OTA_USE_EXTERNAL_FLASH=1
endif

ifeq ($(CY_RUN_CODE_FROM_XIP),1)
    DEFINES+=CY_RUN_CODE_FROM_XIP=1
endif

ifeq ($(CY_XIP_SMIF_MODE_CHANGE),1)
    DEFINES+=CY_XIP_SMIF_MODE_CHANGE=1
endif

##################################
# Additional / custom linker flags.
##################################

#IMP_CHECK : Usage of -Wl LDFlag.

ifeq ($(TOOLCHAIN),GCC_ARM)
    CY_ELF_TO_HEX=$(MTB_TOOLCHAIN_GCC_ARM__OBJCOPY)
    CY_ELF_TO_HEX_OPTIONS="-O ihex"
    CY_ELF_TO_HEX_FILE_ORDER="elf_first"
    LDFLAGS+="-Wl,--defsym,MCUBOOT_HEADER_SIZE=$(MCUBOOT_HEADER_SIZE),--defsym,FLASH_AREA_IMG_1_PRIMARY_START=$(FLASH_AREA_IMG_1_PRIMARY_START),--defsym,FLASH_AREA_IMG_1_PRIMARY_SIZE=$(FLASH_AREA_IMG_1_PRIMARY_SIZE)"
else
ifeq ($(TOOLCHAIN),IAR)
    CY_ELF_TO_HEX="$(CY_COMPILER_IAR_DIR)/bin/ielftool"
    CY_ELF_TO_HEX_OPTIONS="--ihex"
    CY_ELF_TO_HEX_FILE_ORDER="elf_first"
    LDFLAGS+=--config_def MCUBOOT_HEADER_SIZE=$(MCUBOOT_HEADER_SIZE) --config_def FLASH_AREA_IMG_1_PRIMARY_START=$(FLASH_AREA_IMG_1_PRIMARY_START) --config_def FLASH_AREA_IMG_1_PRIMARY_SIZE=$(FLASH_AREA_IMG_1_PRIMARY_SIZE)
else
ifeq ($(TOOLCHAIN),ARM)
    CY_ELF_TO_HEX=$(MTB_TOOLCHAIN_ARM__BASE_DIR)/bin/fromelf.exe
    CY_ELF_TO_HEX_OPTIONS="--i32combined --output"
    CY_ELF_TO_HEX_FILE_ORDER="hex_first"
    LDFLAGS+=--pd=-DMCUBOOT_HEADER_SIZE=$(MCUBOOT_HEADER_SIZE) --pd=-DFLASH_AREA_IMG_1_PRIMARY_START=$(FLASH_AREA_IMG_1_PRIMARY_START) --pd=-DFLASH_AREA_IMG_1_PRIMARY_SIZE=$(FLASH_AREA_IMG_1_PRIMARY_SIZE)
    LDFLAGS+=--diag_suppress=L6314W
else
    $(error Must define toolchain ! GCC_ARM, ARM, or IAR)
endif #ARM
endif #IAR
endif #GCC_ARM

# Find Linker Script using wildcard
# Directory within ota-upgrade library

ifeq ($(OTA_SUPPORT),1)
ifeq ($(OTA_LINKER_FILE), )
$(info ======================================================================= )
$(error Provide OTA linker file for $(TARGET))
else
LINKER_SCRIPT=$(OTA_LINKER_FILE)
endif
endif

###################################################################################################
#
# Build information
#
###################################################################################################

ifneq ($(CY_SECONDSTAGE),)
ifeq ($(OTA_BUILD_VERBOSE),1)
$(info Makefile:)
$(info Makefile: TARGET                         = $(TARGET))
$(info Makefile: OTA_PLATFORM                   = $(OTA_PLATFORM))
$(info Makefile: OTA_FLASH_MAP                  = $(OTA_FLASH_MAP))
$(info Makefile: OTA_HTTP_SUPPORT               = $(OTA_HTTP_SUPPORT))
$(info Makefile: OTA_MQTT_SUPPORT               = $(OTA_MQTT_SUPPORT))
$(info Makefile: OTA_BT_SUPPORT                 = $(OTA_BT_SUPPORT))
$(info Makefile: OTA_BT_SECURE                  = $(OTA_BT_SECURE))
$(info Makefile: OTA_APP_VERSION_MAJOR          = $(OTA_APP_VERSION_MAJOR))
$(info Makefile: OTA_APP_VERSION_MINOR          = $(OTA_APP_VERSION_MINOR))
$(info Makefile: OTA_APP_VERSION_BUILD          = $(OTA_APP_VERSION_BUILD))
$(info Makefile: OTA_TEST_APP_VERSION_IN_TAR    = $(OTA_TEST_APP_VERSION_IN_TAR))
ifneq ($(OTA_HTTP_USER_AGENT_VALUE),)
$(info Makefile: OTA_HTTP_USER_AGENT_VALUE      = $(OTA_HTTP_USER_AGENT_VALUE))
endif
ifneq ($(OTA_HTTP_MAX_HDR_SIZE),)
$(info Makefile: OTA_HTTP_MAX_HDR_SIZE          = $(OTA_HTTP_MAX_HDR_SIZE))
endif
ifneq ($(OTA_HTTP_USE_CUSTOM_CONFIG),)
$(info Makefile: OTA_HTTP_USE_CUSTOM_CONFIG     = $(OTA_HTTP_USE_CUSTOM_CONFIG))
endif
ifneq ($(OTA_MQTT_COMPANY_TOPIC_PREPEND),)
$(info Makefile: OTA_MQTT_COMPANY_TOPIC_PREPEND = $(OTA_MQTT_COMPANY_TOPIC_PREPEND))
endif
$(info Makefile: USE_XIP                      = $(USE_XIP))
$(info Makefile: CY_RUN_CODE_FROM_XIP         = $(CY_RUN_CODE_FROM_XIP))
$(info Makefile: CY_XIP_SMIF_MODE_CHANGE      = $(CY_XIP_SMIF_MODE_CHANGE))
$(info Makefile: USE_EXTERNAL_FLASH           = $(USE_EXTERNAL_FLASH))

endif # OTA_BUILD_VERBOSE=1

ifeq ($(OTA_BUILD_FLASH_VERBOSE),1)
$(info MCUBOOT_MAX_IMG_SECTORS      = $(MCUBOOT_MAX_IMG_SECTORS))
$(info FLASH_AREA_BOOTLOADER_DEV_ID = $(FLASH_AREA_BOOTLOADER_DEV_ID))
$(info FLASH_AREA_BOOTLOADER_START  = $(FLASH_AREA_BOOTLOADER_START))
$(info FLASH_AREA_BOOTLOADER_SIZE   = $(FLASH_AREA_BOOTLOADER_SIZE))
$(info MCUBOOT_IMAGE_NUMBER         = $(MCUBOOT_IMAGE_NUMBER))
$(info FLASH_AREA_IMG_1_PRIMARY_DEV_ID   = $(FLASH_AREA_IMG_1_PRIMARY_DEV_ID))
$(info FLASH_AREA_IMG_1_PRIMARY_START    = $(FLASH_AREA_IMG_1_PRIMARY_START))
$(info FLASH_AREA_IMG_1_PRIMARY_SIZE     = $(FLASH_AREA_IMG_1_PRIMARY_SIZE))
$(info FLASH_AREA_IMG_1_SECONDARY_DEV_ID = $(FLASH_AREA_IMG_1_SECONDARY_DEV_ID))
$(info FLASH_AREA_IMG_1_SECONDARY_START  = $(FLASH_AREA_IMG_1_SECONDARY_START))
$(info FLASH_AREA_IMG_1_SECONDARY_SIZE   = $(FLASH_AREA_IMG_1_SECONDARY_SIZE))
$(info FLASH_AREA_IMAGE_SCRATCH_DEV_ID = $(FLASH_AREA_IMAGE_SCRATCH_DEV_ID))
$(info FLASH_AREA_IMAGE_SCRATCH_START  = $(FLASH_AREA_IMAGE_SCRATCH_START))
$(info FLASH_AREA_IMAGE_SCRATCH_SIZE   = $(FLASH_AREA_IMAGE_SCRATCH_SIZE))
ifeq ($(MCUBOOT_IMAGE_NUMBER),2)
$(info FLASH_AREA_IMG_2_PRIMARY_DEV_ID   = $(FLASH_AREA_IMG_2_PRIMARY_DEV_ID))
$(info FLASH_AREA_IMG_2_PRIMARY_START    = $(FLASH_AREA_IMG_2_PRIMARY_START))
$(info FLASH_AREA_IMG_2_PRIMARY_SIZE     = $(FLASH_AREA_IMG_2_PRIMARY_SIZE))
$(info FLASH_AREA_IMG_2_SECONDARY_DEV_ID = $(FLASH_AREA_IMG_2_SECONDARY_DEV_ID))
$(info FLASH_AREA_IMG_2_SECONDARY_START  = $(FLASH_AREA_IMG_2_SECONDARY_START))
$(info FLASH_AREA_IMG_2_SECONDARY_SIZE   = $(FLASH_AREA_IMG_2_SECONDARY_SIZE))
endif
$(info FLASH_AREA_IMAGE_SWAP_STATUS_DEV_ID = $(FLASH_AREA_IMAGE_SWAP_STATUS_DEV_ID))
$(info FLASH_AREA_IMAGE_SWAP_STATUS_START  = $(FLASH_AREA_IMAGE_SWAP_STATUS_START))
$(info FLASH_AREA_IMAGE_SWAP_STATUS_SIZE   = $(FLASH_AREA_IMAGE_SWAP_STATUS_SIZE))
endif # OTA_BUILD_FLASH_VERBOSE

ifeq ($(OTA_BUILD_COMPONENTS_VERBOSE),1)
$(info COMPONENTS)
$(info COMPONENTS=$(COMPONENTS))
endif
ifeq ($(OTA_BUILD_DEFINES_VERBOSE),1)
$(info DEFINES)
$(info DEFINES=$(DEFINES))
endif
ifeq ($(OTA_BUILD_IGNORE_VERBOSE),1)
$(info CY_IGNORE)
$(info CY_IGNORE=$(CY_IGNORE))
endif # OTA_BUILD_VERBOSE = COMPONENTS
endif    # SECOND STAGE of build


###################################################################################################
# OTA POST BUILD scripting
###################################################################################################

######################################
# Build Location / Output directory
######################################

# output directory for use in the sign_script.bash
ifneq ($(CY_SECONDSTAGE),)
OUTPUT_FILE_PATH=$(CY_BUILD_LOCATION)/$(TARGET)/$(CONFIG)
$(info CY_BUILD_LOCATION $(CY_BUILD_LOCATION))
$(info TARGET            $(TARGET))
$(info CONFIG            $(CONFIG))
$(info OUTPUT_FILE_PATH  $(OUTPUT_FILE_PATH))
$(info CY_COMPILER_GCC_ARM_DIR $(CY_COMPILER_GCC_ARM_DIR))
ifeq ($(CY_COMPILER_GCC_ARM_DIR),)
    CY_COMPILER_GCC_ARM_DIR=$(CY_TOOLS_DIR)/gcc
endif
$(info CY_COMPILER_GCC_ARM_DIR $(CY_COMPILER_GCC_ARM_DIR))

# POSTBUILD script is NOT provided by the Application. Use default OTA POSTBUILD scripts.
ifeq ($(OTA_APP_POSTBUILD),)
$(info "")
$(info Application Makefile has not defined OTA_APP_POSTBUILD. Using OTA library's default POSTBUILD commands.)
$(info "")

# Check for Policy file path for 20829 devices
ifeq ($(OTA_PLATFORM), CYW20829)
ifeq ($(OTA_APP_POLICY_PATH),)
$(info "")
$(info Application makefile has not defined OTA_APP_POLICY_PATH. Using OTA library's default Policy file for 20829 device.)
$(info "")
OTA_APP_POLICY_PATH?=$(RELATIVE_FILE1_FILE2)/../source/bootloader_support/COMPONENT_MCUBOOT/policy/policy_CM33_no_secure.json
$(info OTA_APP_POLICY_PATH = $(OTA_APP_POLICY_PATH) )
endif
endif
endif # SECONDSTAGE

CY_HEX_TO_BIN="$(MTB_TOOLCHAIN_GCC_ARM__OBJCOPY)"
APP_BUILD_VERSION=$(OTA_APP_VERSION_MAJOR).$(OTA_APP_VERSION_MINOR).$(OTA_APP_VERSION_BUILD)

# MCUBoot flash support location
MCUBOOT_DIR=$(RELATIVE_FILE1_FILE2)/../source/bootloader_support/COMPONENT_MCUBOOT
IMG_SIGNING_SCRIPT_TOOL_PATH?=$(MCUBOOT_DIR)/imgtool/imgtool.py
CY_SIGN_KEY_PATH?=$(MCUBOOT_DIR)/keys/cypress-test-ec-p256.pem

IMGTOOL_SCRIPT_NAME:= $(shell echo $(IMG_SIGNING_SCRIPT_TOOL_PATH)|rev|cut -d "/" -f1 |rev )
MCUBOOT_SCRIPT_FILE_DIR := $(shell echo "$(IMG_SIGNING_SCRIPT_TOOL_PATH)" | sed "s@/$(IMGTOOL_SCRIPT_NAME)@@")
MCUBOOT_KEY_FILE := $(shell echo $(CY_SIGN_KEY_PATH)|rev|cut -d "/" -f1 |rev )
MCUBOOT_KEY_DIR := $(shell echo "$(CY_SIGN_KEY_PATH)" | sed "s@/$(MCUBOOT_KEY_FILE)@@")

#--------------------------------------
# CYW20829 POSTBUILD
#--------------------------------------
ifeq ($(OTA_PLATFORM), CYW20829)
$(info Makefile: CYW20829 POSTBUILD )

    MCUBOOT_SCRIPT_FILE_PATH?=$(RELATIVE_FILE1_FILE2)/../scripts/sign_script_20829.bash
    CY_ELF_TO_HEX=$(CY_HEX_TO_BIN)

    CY_TOC2_GENERATOR = $(SEARCH_recipe-make-cat1b)/make/scripts/20829/run_toc2_generator.sh

    _MTB_RECIPE_20829_SREC_CAT_UTIL=$(CY_TOOL_srec_cat_EXE_ABS)

    ifeq ($(TOOLCHAIN),ARM)
        POSTBUILD=$(MTB_TOOLCHAIN_ARM__BASE_DIR)/bin/fromelf $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).elf --bin --output=$(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).bin;
        POSTBUILD+=$(MTB_TOOLS__BASH_CMD) $(MTB_TOOLS__RECIPE_DIR)/make/scripts/20829/flash_postbuild.sh "$(TOOLCHAIN)" "$(MTB_TOOLS__OUTPUT_CONFIG_DIR)" "$(APPNAME)" "$(MTB_TOOLCHAIN_GCC_ARM__BASE_DIR)/bin" "$(_MTB_RECIPE_20829_SREC_CAT_UTIL)" "$(BOOTSTRAP_SIZE)";
    else
    ifeq ($(TOOLCHAIN),IAR)
        ifeq ($(APPTYPE),flash)
            OTA_POSTBUILD_PARAM=--bin-multi
        else
            OTA_POSTBUILD_PARAM=--bin
        endif
        POSTBUILD=$(MTB_TOOLCHAIN_IAR__BASE_DIR)/bin/ielftool $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).elf $(OTA_POSTBUILD_PARAM) $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).bin;
        POSTBUILD+=$(MTB_TOOLCHAIN_IAR__BASE_DIR)/bin/ielfdumparm  -a $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).elf > $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).dis;
        POSTBUILD+=$(MTB_TOOLS__BASH_CMD) $(MTB_TOOLS__RECIPE_DIR)/make/scripts/20829/flash_postbuild.sh "$(TOOLCHAIN)" "$(MTB_TOOLS__OUTPUT_CONFIG_DIR)" "$(APPNAME)" "$(MTB_TOOLCHAIN_GCC_ARM__BASE_DIR)/bin" "$(_MTB_RECIPE_20829_SREC_CAT_UTIL)" "$(BOOTSTRAP_SIZE)";
    else
    ifeq ($(TOOLCHAIN),GCC_ARM)
        POSTBUILD="$(MTB_TOOLCHAIN_GCC_ARM__OBJCOPY)" "$(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).elf" -S -O binary "$(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).bin";
    endif # GCC_ARM
    endif # IAR
    endif # ARM

    CY_ENC_IMG ?= 0
    FLASH_BASE_ADDRESS ?= 0x60000000
    CY_SERVICE_APP_DESCR_ADDR = 0x0

# If POSTBUILD fails, Turn this on to check that all args are present
ifneq ($(CY_SECONDSTAGE),)
ifeq ($(OTA_BUILD_POST_VERBOSE),1)
# Check that all arguments for the POSTBUILD functionality are present
$(info MCUBOOT_SCRIPT_FILE_PATH         =$(MCUBOOT_SCRIPT_FILE_PATH))
$(info APPNAME                          =$(APPNAME))
$(info CY_ELF_TO_HEX                    =$(CY_ELF_TO_HEX))
$(info CY_ELF_TO_HEX_OPTIONS            =$(CY_ELF_TO_HEX_OPTIONS))
$(info CY_ELF_TO_HEX_FILE_ORDER         =$(CY_ELF_TO_HEX_FILE_ORDER))
$(info CY_HEX_TO_BIN                    =$(CY_HEX_TO_BIN))
$(info FLASH_ERASE_SECONDARY_SLOT_VALUE =$(FLASH_ERASE_SECONDARY_SLOT_VALUE))
$(info MCUBOOT_HEADER_SIZE              =$(MCUBOOT_HEADER_SIZE))
$(info APP_BUILD_VERSION                =$(APP_BUILD_VERSION))
$(info FLASH_BASE_ADDRESS               =$(FLASH_BASE_ADDRESS))
$(info FLASH_AREA_IMG_1_PRIMARY_START   =$(FLASH_AREA_IMG_1_PRIMARY_START))
$(info MCUBOOT_KEY_DIR                  =$(MCUBOOT_KEY_DIR))
$(info MCUBOOT_KEY_FILE                 =$(MCUBOOT_KEY_FILE))
$(info CY_TOC2_GENERATOR                =$(CY_TOC2_GENERATOR))
$(info CY_LCS                           =$(CY_LCS))
$(info DEVICE_(MPN_LIST)_SRAM_KB        =$(DEVICE_$(MPN_LIST)_SRAM_KB))
$(info MTB_TOOLS__OUTPUT_CONFIG_DIR     =$(MTB_TOOLS__OUTPUT_CONFIG_DIR))
$(info APPTYPE                          =$(APPTYPE))
$(info current_dir                      =$(current_dir))
$(info SMIF_CRYPTO_CONFIG               =$(SMIF_CRYPTO_CONFIG))
$(info MTB_TOOLCHAIN_GCC_ARM__BASE_DIR  =$(MTB_TOOLCHAIN_GCC_ARM__BASE_DIR))
$(info OTA_APP_POLICY_PATH              =$(OTA_APP_POLICY_PATH))
$(info CY_SLOT_SIZE                     =$(CY_SLOT_SIZE))
$(info CY_ENCRYPTION                    =$(CY_ENCRYPTION))
$(info CY_SERVICE_APP_DESCR_ADDR        =$(CY_SERVICE_APP_DESCR_ADDR))
$(info BOOTSTRAP_SIZE                   =$(BOOTSTRAP_SIZE))
$(info DEVICE_SRAM_SIZE_KB              =$(DEVICE_SRAM_SIZE_KB))
endif # OTA_BUILD_VERBOSE = POST
endif # SECOND STAGE

    # CYW920829 POSTBUILD call
    POSTBUILD+=$(MCUBOOT_SCRIPT_FILE_PATH) $(MTB_TOOLCHAIN_GCC_ARM__BASE_DIR) $(CY_PYTHON_PATH) $(MTB_TOOLS__OUTPUT_CONFIG_DIR) $(APPNAME) \
        $(CY_ELF_TO_HEX) $(CY_ELF_TO_HEX_OPTIONS) $(CY_ELF_TO_HEX_FILE_ORDER) $(CY_HEX_TO_BIN) \
        $(FLASH_ERASE_SECONDARY_SLOT_VALUE) $(MCUBOOT_HEADER_SIZE) $(APP_BUILD_VERSION) $(FLASH_BASE_ADDRESS) \
        $(FLASH_AREA_IMG_1_PRIMARY_START) $(FLASH_AREA_IMG_1_PRIMARY_SIZE) $(FLASH_AREA_IMG_1_SECONDARY_START) \
        $(MCUBOOT_KEY_DIR) $(MCUBOOT_KEY_FILE)\
        $(CY_TOC2_GENERATOR) $(CY_LCS) $(MTB_TOOLS__OUTPUT_CONFIG_DIR) $(APPNAME) $(APPTYPE) $(current_dir) $(SMIF_CRYPTO_CONFIG) $(MTB_TOOLCHAIN_GCC_ARM__BASE_DIR) $(OTA_APP_POLICY_PATH) $(FLASH_AREA_IMG_1_PRIMARY_SIZE) $(CY_ENC_IMG) $(CY_SERVICE_APP_DESCR_ADDR) $(BOOTSTRAP_SIZE) $(DEVICE_$(MPN_LIST)_SRAM_KB);
    POSTBUILD+=rm -rf $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).bin;

# end (CYW920829) section
else
    # PSoC 062, PSoC 062s3, PSoC 064B0S2 Here

    SIGN_SCRIPT_FILE_PATH=$(RELATIVE_FILE1_FILE2)/../scripts/sign_script.bash
    IMGTOOL_COMMAND_ARG=sign
    CY_SIGNING_KEY_ARG="-k $(MCUBOOT_KEY_DIR)/$(MCUBOOT_KEY_FILE)"

    ifeq ($(ACTUAL_TARGET),CY8CKIT-064B0S2-4343W)        #IMP_CHECK
        # values changed for 064B0S2 board
        IMGTOOL_COMMAND_ARG=do_not_sign
    endif

# If POSTBUILD fails, Turn this on to check all args are present
ifneq ($(CY_SECONDSTAGE),)
ifeq ($(OTA_BUILD_POST_VERBOSE),1)
$(info SIGN_SCRIPT_FILE_PATH            =$(SIGN_SCRIPT_FILE_PATH))
$(info OUTPUT_FILE_PATH                 =$(OUTPUT_FILE_PATH))
$(info APPNAME                          =$(APPNAME))
$(info CY_ELF_TO_HEX                    =$(CY_ELF_TO_HEX))
$(info CY_ELF_TO_HEX_OPTIONS            =$(CY_ELF_TO_HEX_OPTIONS))
$(info CY_ELF_TO_HEX_FILE_ORDER         =$(CY_ELF_TO_HEX_FILE_ORDER))
$(info MCUBOOT_SCRIPT_FILE_DIR          =$(MCUBOOT_SCRIPT_FILE_DIR))
$(info IMGTOOL_SCRIPT_NAME              =$(IMGTOOL_SCRIPT_NAME))
$(info IMGTOOL_COMMAND_ARG              =$(IMGTOOL_COMMAND_ARG))
$(info FLASH_ERASE_SECONDARY_SLOT_VALUE =$(FLASH_ERASE_SECONDARY_SLOT_VALUE))
$(info MCUBOOT_HEADER_SIZE              =$(MCUBOOT_HEADER_SIZE))
$(info MCUBOOT_MAX_IMG_SECTORS          =$(MCUBOOT_MAX_IMG_SECTORS))
$(info APP_BUILD_VERSION                =$(APP_BUILD_VERSION))
$(info FLASH_AREA_IMG_1_PRIMARY_START   =$(FLASH_AREA_IMG_1_PRIMARY_START))
$(info FLASH_AREA_IMG_1_PRIMARY_SIZE    =$(FLASH_AREA_IMG_1_PRIMARY_SIZE))
$(info CY_HEX_TO_BIN                    =$(CY_HEX_TO_BIN))
$(info CY_SIGNING_KEY_ARG               =$(CY_SIGNING_KEY_ARG))
endif # OTA_BUILD_VERBOSE = POST
endif # SECOND STAGE

    POSTBUILD=$(SIGN_SCRIPT_FILE_PATH) $(OUTPUT_FILE_PATH) $(APPNAME) $(CY_PYTHON_PATH)\
              $(CY_ELF_TO_HEX) $(CY_ELF_TO_HEX_OPTIONS) $(CY_ELF_TO_HEX_FILE_ORDER)\
              $(MCUBOOT_SCRIPT_FILE_DIR) $(IMGTOOL_SCRIPT_NAME) $(IMGTOOL_COMMAND_ARG) $(FLASH_ERASE_SECONDARY_SLOT_VALUE) $(MCUBOOT_HEADER_SIZE)\
              $(MCUBOOT_MAX_IMG_SECTORS) $(APP_BUILD_VERSION) $(FLASH_AREA_IMG_1_PRIMARY_START) $(FLASH_AREA_IMG_1_PRIMARY_SIZE)\
              $(CY_HEX_TO_BIN) $(CY_SIGNING_KEY_ARG)

endif # 20829 / PSOc6
else # POSTBUILD script provided by the Application.
ifneq ($(CY_SECONDSTAGE),)
$(info "")
$(info OTA_APP_POSTBUILD is defined by Application Makefile.)
$(info "")
endif # SECOND STAGE
    POSTBUILD=$(OTA_APP_POSTBUILD)
endif
