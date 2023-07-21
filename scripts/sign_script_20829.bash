#!/bin/bash
#
# This file is used by make to create the build commands to sign an OTA image
#
# Modify at your peril !
#
# break immediately on errors
set -e

#
# Arguments
# We have a lot
#
GCC_ARM_TOOLCHAIN_PATH=$1
shift
CY_PYTHON_PATH=$1
shift
CY_OUTPUT_PATH=$1
shift
CY_OUTPUT_NAME=$1
shift
CY_ELF_TO_HEX=$1
shift
CY_ELF_TO_HEX_OPTIONS=$1
shift
CY_ELF_TO_HEX_FILE_ORDER=$1
shift
CY_HEX_TO_BIN=$1
shift
FLASH_ERASE_VALUE=$1
shift
MCUBOOT_HEADER_SIZE=$1
shift
APP_BUILD_VERSION=$1
shift
FLASH_BASE_ADDRESS=$1
shift
FLASH_AREA_IMG_1_PRIMARY_START=$1
shift
FLASH_AREA_IMG_1_PRIMARY_SIZE=$1
shift
FLASH_AREA_IMG_1_SECONDARY_START=$1
shift
MCUBOOT_KEY_DIR=$1
shift
MCUBOOT_KEY_FILE=$1
shift
CY_TOC2_GENERATOR=$1
shift
CY_LCS=$1
shift
MTB_TOOLS__OUTPUT_CONFIG_DIR=$1
shift
APPNAME=$1
shift
APPTYPE=$1
shift
current_dir=$1
shift
SMIF_CRYPTO_CONFIG=$1
shift
MTB_TOOLCHAIN_GCC_ARM__BASE_DIR=$1
shift
OTA_APP_POLICY_PATH=$1
shift
CY_SLOT_SIZE=$1
shift
CY_ENCRYPTION=$1
shift
CY_SERVICE_APP_DESCR_ADDR=$1
shift
BOOTSTRAP_SIZE=$1
shift
DEVICE_SRAM_SIZE_KB=$1

# Export these values for python3 click module
export LC_ALL=C.UTF-8
export LANG=C.UTF-8

CY_OUTPUT_ELF=$CY_OUTPUT_PATH/$CY_OUTPUT_NAME.elf
CY_OUTPUT_BIN=$CY_OUTPUT_PATH/$CY_OUTPUT_NAME.bin
CY_OUTPUT_FINAL_BIN=$CY_OUTPUT_PATH/$CY_OUTPUT_NAME.final.bin
CY_OUTPUT_HEX=$CY_OUTPUT_PATH/$CY_OUTPUT_NAME.unsigned.hex
CY_OUTPUT_FINAL_HEX=$CY_OUTPUT_PATH/$CY_OUTPUT_NAME.hex
# Copy final to .final.hex for MTB
CY_OUTPUT_FINAL_FINAL_HEX=$CY_OUTPUT_PATH/$CY_OUTPUT_NAME.final.hex
CY_OUTPUT_FILE_WILD=$CY_OUTPUT_PATH/$CY_OUTPUT_NAME.*
CY_OUTPUT_FILE_NAME_BIN=$CY_OUTPUT_NAME.bin
CY_OUTPUT_FILE_NAME_TAR=$CY_OUTPUT_NAME.tar
CY_COMPONENTS_JSON_NAME=components.json
#
# For elf -> hex conversion
#
if [ "$CY_ELF_TO_HEX_FILE_ORDER" == "elf_first" ]
then
    CY_ELF_TO_HEX_FILE_1=$CY_OUTPUT_ELF
    CY_ELF_TO_HEX_FILE_2=$CY_OUTPUT_HEX
else
    CY_ELF_TO_HEX_FILE_1=$CY_OUTPUT_HEX
    CY_ELF_TO_HEX_FILE_2=$CY_OUTPUT_ELF
fi

# Add base address to offset for PRIMARY slot
FLASH_AREA_IMG_1_PRIMARY_START_ABS=`printf "0x%x\n" $(($FLASH_BASE_ADDRESS + $FLASH_AREA_IMG_1_PRIMARY_START))`

#
# Leave here for debugging
echo "--------------------- MCUBOOT SIGN Vars ----------------------------------------------------------------------"
echo "GCC_ARM_TOOLCHAIN_PATH     =$GCC_ARM_TOOLCHAIN_PATH"
echo "CY_PYTHON_PATH             =$CY_PYTHON_PATH"
echo "CY_OUTPUT_PATH             =$CY_OUTPUT_PATH"
echo "CY_OUTPUT_NAME             =$CY_OUTPUT_NAME"
echo "CY_ELF_TO_HEX              =$CY_ELF_TO_HEX"
echo "CY_ELF_TO_HEX_OPTIONS      =$CY_ELF_TO_HEX_OPTIONS"
echo "CY_ELF_TO_HEX_FILE_ORDER   =$CY_ELF_TO_HEX_FILE_ORDER"
echo "CY_HEX_TO_BIN              =$CY_HEX_TO_BIN"
echo "FLASH_ERASE_VALUE          =$FLASH_ERASE_VALUE"
echo "MCUBOOT_HEADER_SIZE        =$MCUBOOT_HEADER_SIZE"
echo "APP_BUILD_VERSION          =$APP_BUILD_VERSION"
echo "FLASH_BASE_ADDRESS                 =$FLASH_BASE_ADDRESS"
echo "FLASH_AREA_IMG_1_PRIMARY_START     =$FLASH_AREA_IMG_1_PRIMARY_START"
echo "FLASH_AREA_IMG_1_PRIMARY_START_ABS =$FLASH_AREA_IMG_1_PRIMARY_START_ABS"
echo "FLASH_AREA_IMG_1_PRIMARY_SIZE      =$FLASH_AREA_IMG_1_PRIMARY_SIZE"
echo "FLASH_AREA_IMG_1_SECONDARY_START   =$FLASH_AREA_IMG_1_SECONDARY_START"
echo "MCUBOOT_KEY_DIR                    =$MCUBOOT_KEY_DIR"
echo "MCUBOOT_KEY_FILE                   =$MCUBOOT_KEY_FILE"
echo "CY_TOC2_GENERATOR                =$CY_TOC2_GENERATOR"
echo "CY_LCS                           =$CY_LCS"
echo "MTB_TOOLS__OUTPUT_CONFIG_DIR     =$MTB_TOOLS__OUTPUT_CONFIG_DIR"
echo "APPNAME                          =$APPNAME"
echo "APPTYPE                          =$APPTYPE"
echo "current_dir                      =$current_dir"
echo "SMIF_CRYPTO_CONFIG               =$SMIF_CRYPTO_CONFIG"
echo "MTB_TOOLCHAIN_GCC_ARM__BASE_DIR  =$MTB_TOOLCHAIN_GCC_ARM__BASE_DIR"
echo "OTA_APP_POLICY_PATH              =$OTA_APP_POLICY_PATH"
echo "CY_SLOT_SIZE                     =$CY_SLOT_SIZE"
echo "CY_ENCRYPTION                    =$CY_ENCRYPTION"
echo "CY_SERVICE_APP_DESCR_ADDR        =$CY_SERVICE_APP_DESCR_ADDR"
echo "BOOTSTRAP_SIZE                   =$BOOTSTRAP_SIZE"
echo "DEVICE_SRAM_SIZE_KB              =$DEVICE_SRAM_SIZE_KB"

# For FLASH_ERASE_VALUE
# If value is 0x00, we need to specify "-R 0"
# If value is 0xFF, we do not specify anything!
#
FLASH_ERASE_ARG=
if [ "$FLASH_ERASE_VALUE" == "0x00" ]
then
FLASH_ERASE_ARG="-R 0"
fi

PRJ_DIR=.

echo ""
echo "[Bin to Hex for debug (unsigned)]"
echo "$CY_HEX_TO_BIN -I binary -O ihex $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.bin $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.unsigned.hex"
$CY_HEX_TO_BIN -I binary -O ihex $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.bin $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.unsigned.hex

echo ""
echo "[TOC2_Generate] Execute toc2 generator script for $CY_OUTPUT_NAME"
echo "source $CY_TOC2_GENERATOR $CY_LCS $MTB_TOOLS__OUTPUT_CONFIG_DIR $APPNAME $APPTYPE $current_dir $SMIF_CRYPTO_CONFIG $MTB_TOOLCHAIN_GCC_ARM__BASE_DIR $OTA_APP_POLICY_PATH $CY_SLOT_SIZE $CY_ENCRYPTION $CY_SERVICE_APP_DESCR_ADDR $BOOTSTRAP_SIZE $DEVICE_SRAM_SIZE_KB"
source $CY_TOC2_GENERATOR $CY_LCS $MTB_TOOLS__OUTPUT_CONFIG_DIR $APPNAME $APPTYPE $current_dir $SMIF_CRYPTO_CONFIG $MTB_TOOLCHAIN_GCC_ARM__BASE_DIR $OTA_APP_POLICY_PATH $CY_SLOT_SIZE $CY_ENCRYPTION $CY_SERVICE_APP_DESCR_ADDR $BOOTSTRAP_SIZE $DEVICE_SRAM_SIZE_KB;

echo ""
echo "Checking for TOC2 output file"
ls -l $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.final.bin

#################################################### SIGNING BOOT IMAGE ######################################################
echo ""
echo "[SIGNING BOOT using cysecuretool(4.2)]"
echo "cysecuretools -q -t cyw20829 -p $OTA_APP_POLICY_PATH sign-image -H $MCUBOOT_HEADER_SIZE --align 8 -v $APP_BUILD_VERSION -S $FLASH_AREA_IMG_1_PRIMARY_SIZE --overwrite-only -R 0xff --image $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.final.bin --output $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.signed.bin --key-path $MCUBOOT_KEY_DIR/$MCUBOOT_KEY_FILE --image-id 1"
cysecuretools -q -t cyw20829 -p $OTA_APP_POLICY_PATH sign-image -H $MCUBOOT_HEADER_SIZE --align 1 -v $APP_BUILD_VERSION -S $FLASH_AREA_IMG_1_PRIMARY_SIZE --overwrite-only -R 0xff --image $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.final.bin --output $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.signed.bin --key-path $MCUBOOT_KEY_DIR/$MCUBOOT_KEY_FILE --image-id 1

echo ""
echo "[Bin to Hex for PRIMARY (BOOT) Slot]"
echo "$CY_HEX_TO_BIN --change-address=$FLASH_AREA_IMG_1_PRIMARY_START_ABS -I binary -O ihex $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.signed.bin $CY_OUTPUT_FINAL_HEX"
$CY_HEX_TO_BIN --change-address=$FLASH_AREA_IMG_1_PRIMARY_START_ABS -I binary -O ihex $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.signed.bin $CY_OUTPUT_FINAL_HEX
echo "Copy $CY_OUTPUT_FINAL_HEX to $CY_OUTPUT_FINAL_FINAL_HEX for MTB"
cp $CY_OUTPUT_FINAL_HEX $CY_OUTPUT_FINAL_FINAL_HEX

#################################################### SIGNING UPGRADE IMAGE ######################################################
echo ""
echo "[SIGNING UPGRADE using cysecuretool(4.2) ]"
echo "cysecuretools -q -t cyw20829 -p $OTA_APP_POLICY_PATH sign-image -H $MCUBOOT_HEADER_SIZE --align 8 -v $APP_BUILD_VERSION -S $FLASH_AREA_IMG_1_PRIMARY_SIZE --overwrite-only -R 0xff --image $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.final.bin --output $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.update.signed.bin --key-path $MCUBOOT_KEY_DIR/$MCUBOOT_KEY_FILE --image-id 1"
cysecuretools -q -t cyw20829 -p $OTA_APP_POLICY_PATH sign-image -H $MCUBOOT_HEADER_SIZE --align 1 -v $APP_BUILD_VERSION -S $FLASH_AREA_IMG_1_PRIMARY_SIZE --overwrite-only -R 0xff --image $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.final.bin --output $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.update.signed.bin --key-path $MCUBOOT_KEY_DIR/$MCUBOOT_KEY_FILE --image-id 1

echo ""
echo "[Bin to Hex for SECONDARY (UPDATE) Slot]"
echo "$CY_HEX_TO_BIN --change-address=$FLASH_AREA_IMG_1_SECONDARY_START -I binary -O ihex $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.update.signed.bin $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.update.hex"
$CY_HEX_TO_BIN --change-address=$FLASH_AREA_IMG_1_SECONDARY_START -I binary -O ihex $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.update.signed.bin $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.update.hex

# clean up temp files
rm $CY_OUTPUT_PATH/$CY_OUTPUT_NAME.final.bin

echo ""
ls -l $CY_OUTPUT_FILE_WILD
echo ""
