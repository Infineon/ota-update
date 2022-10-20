# OTA Makefile Settings for a User Application

## 1. Introduction

This guide shows Makefile variables and build DEFINES used when building an application with ota-update library support.


## 2. Standard Application Makefile Defines


| Makefile Variable | Example | Description |
| ----------------- | ------- | ----------- |
| TARGET?=\<target_name\> | TARGET?=CY8CPROTO-062-4343W | Target Kit for build |
| CORE?=\<core_type\> | CORE?=CM4 | Core CPU for Application<br>(CM33, CM4) |
| APPNAME?=\<application_name\> | APPNAME=ota-test | Name of Application |
| TOOLCHAIN?=\<toolchain\> | TOOLCHAIN?=GCC_ARM | GCC_ARM (default)<br>ARM<br> IAR |
| CONFIG?=\<build_level\> | CONFIG?=Debug | Build level<br>- Debug<br>- Release |

## 3. OTA Specific Makefile Defines

| Makefile addition | Required | Default if not defined | Description |
| ----------------- | -------- | ---------------------- | ----------- |
| OTA_FLASH_MAP=<flash_map.json> | Yes | Error | JSON file passed to flashmap.py that generates flashmap.mk. The JSON file defines:<br>- Internal / external flash usage<br>- Flash area location and sizes<br>- Number of images / slots<br>- XIP (from external flash) if defined |
| OTA_HTTP_SUPPORT=<0,1> | No | Depends on Target Support | Include OTA support for update transfers using HTTP directly to a server.<br>App can use Job or Direct update flow Pull Model. |
| OTA_MQTT_SUPPORT=<0,1> | No | Depends on Target Support | Include OTA support for MQTT Broker transfers over HTTP. Implies HTTP library support<br>App can use Job or Direct update flow Pull Model. |
| OTA_BT_SUPPORT=<0,1> | No | Depends on Target Support | Include BT Support for transfers over BT/BLE/BLESS<br>Push Model. |
| OTA_BT_SECURE=<0,1> | No | Requires BT Support | Include BT TLS secure connection support |
| OTA_APP_VERSION_MAJOR=\<application_version\> | No | 0 | Application Major version number  X.y.z|
| OTA_APP_VERSION_MINOR=\<application_version\> | No | 9 | Application Minor version number  x.Y.z|
| OTA_APP_VERSION_BUILD=\<application_version\> | No | 0 | Application Build version number x.y.Z |
| OTA_TEST_APP_VERSION_IN_TAR=\<0,1\> | No | 0 | Set to 1 to enable checking application version in TAR file in OTA library when updating using a TAR file. |
| OTA_HTTP_USER_AGENT_VALUE=\<string\> | No | "\"ota-http-client\"" | Passed to HTTP library as:<br> DEFINES += HTTP_USER_AGENT_VALUE=$(OTA_TEST_APP_VERSION_IN_TAR)<br>Used in HTTP connection. |
| OTA_HTTP_MAX_HDR_SIZE=\<size\> | No | 2048 | Passed to HTTP library as:<br>DEFINES += HTTP_MAX_RESPONSE_HEADERS_SIZE_BYTES=$(OTA_HTTP_MAX_HDR_SIZE) |
| OTA_HTTP_USE_CUSTOM_CONFIG=\<0,1\> | No | 0 | Passed to HTTP library as:<br>   ifneq ($(OTA_HTTP_USE_CUSTOM_CONFIG),0)<br>DEFINES += HTTP_DO_NOT_USE_CUSTOM_CONFIG<br>endif |
| OTA_MQTT_COMPANY_TOPIC_PREPEND=\<string\> | No | "\"OTAUpdate\"" | Configurable part of MQTT topic name. Must be enclosed in <slash-quote> to compile correctly. |
| OTA_PLATFORM=<platform_type> | Yes | Error | <platform_type> must be one of:<br>CYW20829 - ex: CYW920829M2EVB-01<br>PSOC_062_512K - ex: CY8CPROTO-062S3-4343W<br>PSOC_062_1M - ex: CY8CKIT-062-BLE<br>PSOC_062_2M    - ex: CY8CPROTO-062-4343W<br>PSOC_063_1M - ex: CY8CPROTO-063-BLE<br>PSOC_064_2M    - ex: CY8CKIT-064B0S2-4343W |
| CY_XIP_SMIF_MODE_CHANGE=\<0,1\> | No | 0 | For PSoC™ 62 devices, this is required to be 1.<br>For CYW20829 devices this is required to be 0. |
| CYHAL_DISABLE_WEAK_FUNC_IMPL=<0,1> | No | 0 | Setting to 1 allows for RAM functions necessary to use SMIF functionality while executing from XIP (external flash) to be used properly on PSoCtm targets. <br>For PSoC™ 62 devices, this is required to be 1.<br>For CYW20829 devices this is required to be 0.|

<b>Notes:</b>
- Application Version:
    - Used to verify that the new version is higher than the previous version in the Job Flow.
    - Used in messaging with MQTT Broker, and testing MQTT chunks.

## 4. Additional Makefile statements after paths determination before including start.mk

Creating flashmap.mk is an integral part of building the ota-update library. It informs where to place Primary and Secondary Slots that match MCUBootApp knowledge.
CY8CKIT-064B0S2-4343W ( OTA_PLATFORM=PSOC_064_2M ) is not supported in flashmap.py, and must be defined in the Makefile itself.
Add DEFINES for the build that are based on above mentioned OTA defines.

    ifeq ($(OTA_SUPPORT),1)

    ###################################################################################################
    # OTA Flashmap JSON file Usage
    ###################################################################################################
    #
    # flashmap.py creates flashmap.mk, a number of defines which are then included in the build.
    # The code that uses these defines in in source/port_support/mcuboot/cy_flash_map.c
    # It is important to include the defines here (and not just in cy_flash_map.c) as they are used in POSTBUILD processing
    #
    # Look in configs/flashmap/MCUBoot_Build_Commands.md to choose an appropriate JSON file for you Target and usage
    #
    ifeq ($(OTA_PLATFORM),PSOC_064_2M)
    # CY8CKIT-064B0S2-4343W
    #    Single image JSON File, int + ext flash
    #     flashmap.py does not support this platform at this time
    $(info flashmap.py does not support PSOC_064_2M - We write out directly here!)

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
    $(info "flashmap.py $(CY_PYTHON_PATH) $(SEARCH_ota-update)/scripts/$(FLASHMAP_PYTHON_SCRIPT) -p $(FLASHMAP_PLATFORM) -i $(OTA_FLASH_MAP) > flashmap.mk")
    $(shell $(CY_PYTHON_PATH) $(SEARCH_ota-update)/scripts/$(FLASHMAP_PYTHON_SCRIPT) -p $(FLASHMAP_PLATFORM) -i $(OTA_FLASH_MAP) > flashmap.mk)
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
    # If code resides in external flash, we must support !
    USE_EXTERNAL_FLASH=1

    # For IAR XIP builds, we need to move some code to RAM
    ifeq ($(TOOLCHAIN),IAR)
    ifeq ($(OTA_BUILD_VERBOSE),1)
    $(info Makefile: ADD CY_INIT_CODECOPY_ENABLE for IAR XIP)
    endif
        DEFINES+=CY_INIT_CODECOPY_ENABLE
    endif

    # When running from external flash
    # Signal to /source/port_support/serial_flash/ota_serial_flash.c
    # That we need to turn off XIP and enter critical section when accessing SMIF
    #  NOTE: CYW920829M2EVB-01 or CYW920829-VR do not need this. Generalize for now,
    #        we may need to change this criteria for future devices
    ifneq ($(OTA_PLATFORM),CYW20829)
    ifeq ($(OTA_BUILD_VERBOSE),1)
    $(info Makefile: OTA_PLATFORM = CYW20829 (CYW920829M2EVB-01 or CYW920829-VR))
    endif
    CY_XIP_SMIF_MODE_CHANGE=1
    # Since we are running hybrid (some in RAM, some in External FLash),
    #   we need to override the WEAK functions in CYHAL
    DEFINES+=CYHAL_DISABLE_WEAK_FUNC_IMPL=1
    endif # Not CYW20829
    endif # USE_XIP

    #       Flash areas / images are allowed to be placed willy-nilly
    ifeq ($(FLASH_AREA_IMG_1_SECONDARY_DEV_ID),FLASH_DEVICE_INTERNAL_FLASH)
        FLASH_ERASE_SECONDARY_SLOT_VALUE= $(CY_INTERNAL_FLASH_ERASE_VALUE)
    else
        FLASH_ERASE_SECONDARY_SLOT_VALUE= $(CY_EXTERNAL_FLASH_ERASE_VALUE)
    endif # SECONDARY_DEV_ID

    ###################################
    # Additional Include Directories
    ###################################
    INCLUDES+=$(SEARCH_mqtt)/src\
              $(SEARCH_mqtt)/cyport/include\
              $(SEARCH_wifi-mw-core)/wifi-mw-core/configs\
              $(SEARCH_ota-update)/source/mcuboot/sysflash

    ###################################
    # Misc OTA defines
    ###################################
    DEFINES+=\
        OTA_SUPPORT=1 \
        APP_VERSION_MAJOR=$(OTA_APP_VERSION_MAJOR)\
        APP_VERSION_MINOR=$(OTA_APP_VERSION_MINOR)\
        APP_VERSION_BUILD=$(OTA_APP_VERSION_BUILD)


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

    ifeq ($(USE_EXTERNAL_FLASH),1)
        DEFINES+=OTA_USE_EXTERNAL_FLASH=1
    endif

    ifeq ($(CY_RUN_CODE_FROM_XIP),1)
        DEFINES+=CY_RUN_CODE_FROM_XIP=1
    endif

    ifeq ($(CY_XIP_SMIF_MODE_CHANGE),1)
        DEFINES+=CY_XIP_SMIF_MODE_CHANGE=1
    endif

    # This section needs to be before finding LINKER_SCRIPT_WILDCARD as we need the linker script TOOLCHAIN extension defined
    ifeq ($(TOOLCHAIN),GCC_ARM)
        CY_ELF_TO_HEX=$(MTB_TOOLCHAIN_GCC_ARM__OBJCOPY)
        CY_ELF_TO_HEX_OPTIONS="-O ihex"
        CY_ELF_TO_HEX_FILE_ORDER="elf_first"
        CY_TOOLCHAIN=GCC
        CY_TOOLCHAIN_LS_EXT=ld
        LDFLAGS+="-Wl,--defsym,MCUBOOT_HEADER_SIZE=$(MCUBOOT_HEADER_SIZE),--defsym,FLASH_AREA_IMG_1_PRIMARY_START=$(FLASH_AREA_IMG_1_PRIMARY_START),--defsym,FLASH_AREA_IMG_1_PRIMARY_SIZE=$(FLASH_AREA_IMG_1_PRIMARY_SIZE)"
    else
    ifeq ($(TOOLCHAIN),IAR)
        CY_ELF_TO_HEX="$(CY_COMPILER_IAR_DIR)/bin/ielftool"
        CY_ELF_TO_HEX_OPTIONS="--ihex"
        CY_ELF_TO_HEX_FILE_ORDER="elf_first"
        CY_TOOLCHAIN=$(TOOLCHAIN)
        CY_TOOLCHAIN_LS_EXT=icf
        LDFLAGS+=--config_def MCUBOOT_HEADER_SIZE=$(MCUBOOT_HEADER_SIZE) --config_def FLASH_AREA_IMG_1_PRIMARY_START=$(FLASH_AREA_IMG_1_PRIMARY_START) --config_def FLASH_AREA_IMG_1_PRIMARY_SIZE=$(FLASH_AREA_IMG_1_PRIMARY_SIZE)

    else
    ifeq ($(TOOLCHAIN),ARM)
        CY_ELF_TO_HEX=$(MTB_TOOLCHAIN_ARM__BASE_DIR)/bin/fromelf.exe
        CY_ELF_TO_HEX_OPTIONS="--i32 --output"
        CY_ELF_TO_HEX_FILE_ORDER="hex_first"
        CY_TOOLCHAIN=GCC
        CY_TOOLCHAIN_LS_EXT=sct
        LDFLAGS+=--pd=-DMCUBOOT_HEADER_SIZE=$(MCUBOOT_HEADER_SIZE) --pd=-DFLASH_AREA_IMG_1_PRIMARY_START=$(FLASH_AREA_IMG_1_PRIMARY_START) --pd=-DFLASH_AREA_IMG_1_PRIMARY_SIZE=$(FLASH_AREA_IMG_1_PRIMARY_SIZE)

    else
        $(error Must define toolchain ! GCC_ARM, ARM, or IAR)
    endif #ARM
    endif #IAR
    endif #GCC_ARM

    # Find Linker Script using wildcard
    # Directory within ota-upgrade library
    OTA_LINKER_SCRIPT_BASE_DIR=$(SEARCH_ota-update)/platforms/$(OTA_PLATFORM)/linker_scripts/COMPONENT_$(CORE)/TOOLCHAIN_$(TOOLCHAIN)/ota


    ifneq ($(findstring $(OTA_PLATFORM),PSOC_062_1M PSOC_063_1M),)
        OTA_LINKER_SCRIPT_TYPE=_ota_cm0p_int
    else
        ifeq ($(CY_RUN_CODE_FROM_XIP),1)
            OTA_LINKER_SCRIPT_TYPE=_ota_xip
        else
            OTA_LINKER_SCRIPT_TYPE=_ota_int
        endif
    endif

    LINKER_SCRIPT_WILDCARD=$(OTA_LINKER_SCRIPT_BASE_DIR)/*$(OTA_LINKER_SCRIPT_TYPE).$(CY_TOOLCHAIN_LS_EXT)
    LINKER_SCRIPT=$(wildcard $(LINKER_SCRIPT_WILDCARD))

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
    endif # SECONDSTAGE

    CY_HEX_TO_BIN="$(MTB_TOOLCHAIN_GCC_ARM__OBJCOPY)"
    APP_BUILD_VERSION=$(OTA_APP_VERSION_MAJOR).$(OTA_APP_VERSION_MINOR).$(OTA_APP_VERSION_BUILD)

    # MCUBoot flash support location
    MCUBOOT_DIR=$(SEARCH_ota-update)/source/port_support/mcuboot
    IMGTOOL_SCRIPT_NAME=imgtool/imgtool.py
    MCUBOOT_SCRIPT_FILE_DIR=$(MCUBOOT_DIR)
    MCUBOOT_KEY_DIR=$(MCUBOOT_DIR)/keys
    MCUBOOT_KEY_FILE=cypress-test-ec-p256.pem

    #--------------------------------------
    # CYW20829 POSTBUILD
    #--------------------------------------
    ifeq ($(OTA_PLATFORM), CYW20829)
    $(info Makefile: CYW20829 POSTBUILD (CYW920829M2EVB-01 or CYW920829-VR))
    MCUBOOT_SCRIPT_FILE_PATH?=$(SEARCH_ota-update)/scripts/sign_script_20829.bash
    CY_ELF_TO_HEX=$(CY_HEX_TO_BIN)

    CY_TOC2_GENERATOR = $(SEARCH_recipe-make-cat1b)/make/scripts/20829/run_toc2_generator.sh

    ifeq ($(TOOLCHAIN),ARM)
        POSTBUILD=$(MTB_TOOLCHAIN_ARM__BASE_DIR)/bin/fromelf $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).elf --bin --output=$(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).bin;
        POSTBUILD+=$(MTB_TOOLS__BASH_CMD) $(MTB_TOOLS__RECIPE_DIR)/make/scripts/20829/flash_postbuild.sh "$(TOOLCHAIN)" "$(MTB_TOOLS__OUTPUT_CONFIG_DIR)" "$(APPNAME)" "$(CY_PYTHON_PATH)" "$(_MTB_RECIPE_20829_PYTHON_SCRIPT_PATH)" "$(MTB_TOOLCHAIN_GCC_ARM__BASE_DIR)/bin";
    else
    ifeq ($(TOOLCHAIN),IAR)
        ifeq ($(APPTYPE),flash)
            OTA_POSTBUILD_PARAM=--bin-multi
        else
            OTA_POSTBUILD_PARAM=--bin
        endif
        POSTBUILD=$(MTB_TOOLCHAIN_IAR__BASE_DIR)/bin/ielftool $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).elf $(OTA_POSTBUILD_PARAM) $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).bin;
        POSTBUILD+=$(MTB_TOOLCHAIN_IAR__BASE_DIR)/bin/ielfdumparm  -a $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).elf > $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).dis;
        POSTBUILD+=$(MTB_TOOLS__BASH_CMD) $(MTB_TOOLS__RECIPE_DIR)/make/scripts/20829/flash_postbuild.sh "$(TOOLCHAIN)" "$(MTB_TOOLS__OUTPUT_CONFIG_DIR)" "$(APPNAME)" "$(CY_PYTHON_PATH)" "$(_MTB_RECIPE_20829_PYTHON_SCRIPT_PATH)" "$(MTB_TOOLCHAIN_GCC_ARM__BASE_DIR)/bin";
    else
    ifeq ($(TOOLCHAIN),GCC_ARM)
        POSTBUILD="$(MTB_TOOLCHAIN_GCC_ARM__OBJCOPY)" "$(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).elf" -S -O binary "$(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).bin";
    endif # GCC_ARM
    endif # IAR
    endif # ARM

    # CYW920829M2EVB-01 or CYW920829-VR POSTBUILD call
    POSTBUILD+=$(MCUBOOT_SCRIPT_FILE_PATH) $(MTB_TOOLCHAIN_GCC_ARM__BASE_DIR) $(CY_PYTHON_PATH) $(MTB_TOOLS__OUTPUT_CONFIG_DIR) $(APPNAME) \
        $(CY_ELF_TO_HEX) $(CY_ELF_TO_HEX_OPTIONS) $(CY_ELF_TO_HEX_FILE_ORDER) $(CY_HEX_TO_BIN) \
        $(MCUBOOT_SCRIPT_FILE_DIR) $(IMGTOOL_SCRIPT_NAME) $(FLASH_ERASE_SECONDARY_SLOT_VALUE) $(MCUBOOT_HEADER_SIZE) \
        $(APP_BUILD_VERSION) $(FLASH_BASE_ADDRESS) $(FLASH_AREA_IMG_1_PRIMARY_START) $(FLASH_AREA_IMG_1_PRIMARY_SIZE) $(FLASH_AREA_IMG_1_SECONDARY_START) \
        $(CY_TOC2_GENERATOR) $(CY_LCS) $(APPTYPE) $(MCUBOOT_KEY_DIR) $(MCUBOOT_KEY_FILE) $(SMIF_CRYPTO_CONFIG);
     POSTBUILD+=rm -rf $(MTB_TOOLS__OUTPUT_CONFIG_DIR)/$(APPNAME).bin;

    # end (CYW920829M2EVB-01 or CYW920829-VR) section
    else
    # PSoC 062, PSoC 062s3, PSoC 064B0S2 Here

    SIGN_SCRIPT_FILE_PATH=$(SEARCH_ota-update)/scripts/sign_script.bash
    IMGTOOL_COMMAND_ARG=sign
    CY_SIGNING_KEY_ARG="-k $(MCUBOOT_KEY_DIR)/$(MCUBOOT_KEY_FILE)"

    ifeq ($(ACTUAL_TARGET),CY8CKIT-064B0S2-4343W)
        # values changed for 064B0S2 board
        IMGTOOL_COMMAND_ARG=do_not_sign
    endif

    POSTBUILD=$(SIGN_SCRIPT_FILE_PATH) $(OUTPUT_FILE_PATH) $(APPNAME) $(CY_PYTHON_PATH)\
              $(CY_ELF_TO_HEX) $(CY_ELF_TO_HEX_OPTIONS) $(CY_ELF_TO_HEX_FILE_ORDER)\
              $(MCUBOOT_SCRIPT_FILE_DIR) $(IMGTOOL_SCRIPT_NAME) $(IMGTOOL_COMMAND_ARG) $(FLASH_ERASE_SECONDARY_SLOT_VALUE) $(MCUBOOT_HEADER_SIZE)\
              $(MCUBOOT_MAX_IMG_SECTORS) $(APP_BUILD_VERSION) $(FLASH_AREA_IMG_1_PRIMARY_START) $(FLASH_AREA_IMG_1_PRIMARY_SIZE)\
              $(CY_HEX_TO_BIN) $(CY_SIGNING_KEY_ARG)

    endif # 20829 / PSOc6
    endif # OTA_SUPPORT



## 5. Document History

| Document Version | Description of Change |
| ---------------- | ---------------------------------------------------------- |
| 1.1.0            | Add Makefile additions                |
| 1.0.0            | Initial Document                      |

------

© 2022, Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
This software, associated documentation and materials ("Software") is owned by Cypress Semiconductor Corporation or one of its affiliates ("Cypress") and is protected by and subject to worldwide patent protection (United States and foreign), United States copyright laws and international treaty provisions. Therefore, you may use this Software only as provided in the license agreement accompanying the software package from which you obtained this Software ("EULA"). If no EULA applies, then any reproduction, modification, translation, compilation, or representation of this Software is prohibited without the express written permission of Cypress.
Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress reserves the right to make changes to the Software without notice. Cypress does not assume any liability arising out of the application or use of the Software or any product or circuit described in the Software. Cypress does not authorize its products for use in any products where a malfunction or failure of the Cypress product may reasonably be expected to result in significant property damage, injury or death ("High Risk Product"). By including Cypress's product in a High Risk Product, the manufacturer of such system or application assumes all risk of such use and in doing so agrees to indemnify Cypress against all liability.
