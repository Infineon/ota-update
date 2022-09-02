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
CY_OUTPUT_PATH=$1
shift
CY_OUTPUT_NAME=$1
shift
CY_PYTHON_PATH=$1
shift
CY_ELF_TO_HEX=$1
shift
CY_ELF_TO_HEX_OPTIONS=$1
shift
CY_ELF_TO_HEX_FILE_ORDER=$1
shift
MCUBOOT_SCRIPT_FILE_DIR=$1
shift
IMGTOOL_SCRIPT_NAME=$1
shift
IMGTOOL_COMMAND_ARG=$1
shift
FLASH_ERASE_VALUE=$1
shift
MCUBOOT_HEADER_SIZE=$1
shift
MCUBOOT_MAX_IMG_SECTORS=$1
shift
APP_BUILD_VERSION=$1
shift
FLASH_AREA_IMG_1_PRIMARY_START=$1
shift
FLASH_AREA_IMG_1_PRIMARY_SIZE=$1
shift
CY_HEX_TO_BIN=$1
shift
CY_SIGNING_KEY_ARG=$1
shift

# When we want to separate the WiFi and BT Firmware, we send more arguments
#        POSTBUILD+=FW_DATBLOCK_SEPARATE_FROM_APPLICATION $(FW_DATA_BLOCK_BUILDER)\
#                       $(FW_DATA_BLOCK_WIFI_FW_SOURCE)  $(FW_DATA_BLOCK_CLM_BLOB_SOURCE) $(FW_DATA_BLOCK_BT_FW_SOURCE)\
#                       $(FW_DATA_BLOCK_OUTPUT_FILE_NAME) $(CY_EXTERNAL_FLASH_ERASE_VALUE) $(CY_EXT_MCUBOOT_HEADER_SIZE)\
#                       $(CY_BOOT_PRIMARY_1_IMAGE_2_START) $(CY_BOOT_PRIMARY_1_IMAGE_2_SIZE)

CY_BUILD_SEPARATE_FW=$1
if [ "$CY_BUILD_SEPARATE_FW" == "FW_DATBLOCK_SEPARATE_FROM_APPLICATION" ]
then
    shift
    FW_DATA_BLOCK_BUILDER=$1
    shift
    FW_DATA_BLOCK_WIFI_FW_SOURCE=$1
    shift
    FW_DATA_BLOCK_CLM_BLOB_SOURCE=$1
    shift
    FW_DATA_BLOCK_BT_FW_SOURCE=$1
    shift
    FW_DATA_BLOCK_OUTPUT_FILE_NAME=$1
    shift
    CY_EXTERNAL_FLASH_ERASE_VALUE=$1
    shift
    CY_EXT_MCUBOOT_HEADER_SIZE=$1
    shift
    CY_BOOT_PRIMARY_1_IMAGE_2_START=$1
    shift
    CY_BOOT_PRIMARY_1_IMAGE_2_SIZE=$1
    shift
    MCUBOOT_MAX_FW_IMG_SECTORS=$1

FW_DATA_BLOCK_OUTPUT_FILE_WILD=$FW_DATA_BLOCK_OUTPUT_FILE_NAME.*
FW_DATA_BLOCK_OUTPUT_FILE_BIN=$FW_DATA_BLOCK_OUTPUT_FILE_NAME".bin"
FW_DATA_BLOCK_OUTPUT_FILE_UNSIGNED_HEX=$FW_DATA_BLOCK_OUTPUT_FILE_NAME".unsigned.hex"
FW_DATA_BLOCK_OUTPUT_FILE_HEX=$FW_DATA_BLOCK_OUTPUT_FILE_NAME".hex"

echo "FW_DATA_BLOCK_BUILDER           = $FW_DATA_BLOCK_BUILDER"
echo "FW_DATA_BLOCK_WIFI_FW_SOURCE    = $FW_DATA_BLOCK_WIFI_FW_SOURCE"
echo "FW_DATA_BLOCK_CLM_BLOB_SOURCE   = $FW_DATA_BLOCK_CLM_BLOB_SOURCE"
echo "FW_DATA_BLOCK_BT_FW_SOURCE      = $FW_DATA_BLOCK_BT_FW_SOURCE"
echo "FW_DATA_BLOCK_OUTPUT_FILE_NAME  = $FW_DATA_BLOCK_OUTPUT_FILE_NAME"
echo "CY_EXTERNAL_FLASH_ERASE_VALUE   = $CY_EXTERNAL_FLASH_ERASE_VALUE"
echo "CY_BOOT_PRIMARY_1_IMAGE_2_START = $CY_BOOT_PRIMARY_1_IMAGE_2_START"
echo "CY_BOOT_PRIMARY_1_IMAGE_2_SIZE  = $CY_BOOT_PRIMARY_1_IMAGE_2_SIZE"

fi


# Export these values for python3 click module
export LC_ALL=C.UTF-8
export LANG=C.UTF-8

CY_OUTPUT_BIN=$CY_OUTPUT_PATH/$CY_OUTPUT_NAME.bin
CY_OUTPUT_ELF=$CY_OUTPUT_PATH/$CY_OUTPUT_NAME.elf
CY_OUTPUT_HEX=$CY_OUTPUT_PATH/$CY_OUTPUT_NAME.unsigned.hex
CY_OUTPUT_SIGNED_HEX=$CY_OUTPUT_PATH/$CY_OUTPUT_NAME.hex
CY_OUTPUT_FILE_WILD=$CY_OUTPUT_NAME.*
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

#
# Leave here for debugging
#echo " CY_OUTPUT_NAME           $CY_OUTPUT_NAME"
#echo " CY_OUTPUT_BIN            $CY_OUTPUT_BIN"
#echo " CY_OUTPUT_ELF            $CY_OUTPUT_ELF"
#echo " CY_OUTPUT_HEX            $CY_OUTPUT_HEX"
#echo " CY_OUTPUT_SIGNED_HEX     $CY_OUTPUT_SIGNED_HEX"
#echo " MCUBOOT_SCRIPT_FILE_DIR  $MCUBOOT_SCRIPT_FILE_DIR"
#echo " IMGTOOL_SCRIPT_NAME      $IMGTOOL_SCRIPT_NAME"
#echo " FLASH_ERASE_VALUE        $FLASH_ERASE_VALUE"
#echo " MCUBOOT_HEADER_SIZE      $MCUBOOT_HEADER_SIZE"
#echo " MCUBOOT_MAX_IMG_SECTORS  $MCUBOOT_MAX_IMG_SECTORS"
#echo " APP_BUILD_VERSION         $APP_BUILD_VERSION"
#echo " FLASH_AREA_IMG_1_PRIMARY_START  $FLASH_AREA_IMG_1_PRIMARY_START"
#echo " FLASH_AREA_IMG_1_PRIMARY_SIZE   $FLASH_AREA_IMG_1_PRIMARY_SIZE"
#echo " SIGNING_KEY_PATH         $SIGNING_KEY_PATH"
#
# For FLASH_ERASE_VALUE
# If value is 0x00, we need to specify "-R 0"
# If value is 0xFF, we do not specify anything!
#
FLASH_ERASE_ARG=
if [ "$FLASH_ERASE_VALUE" == "0x00" ]
then
FLASH_ERASE_ARG="-R 0"
fi

EXT_FLASH_ERASE_ARG=
if [ "$CY_EXTERNAL_FLASH_ERASE_VALUE" == "0x00" ]
then
EXT_FLASH_ERASE_ARG="-R 0"
fi


echo ""
echo "Create  unsigned.hex"
# echo "$CY_ELF_TO_HEX $CY_ELF_TO_HEX_OPTIONS $CY_ELF_TO_HEX_FILE_1 $CY_ELF_TO_HEX_FILE_2"
"$CY_ELF_TO_HEX" $CY_ELF_TO_HEX_OPTIONS $CY_ELF_TO_HEX_FILE_1 $CY_ELF_TO_HEX_FILE_2

# If
if [[ "$IMGTOOL_COMMAND_ARG" != "do_not_sign" ]]; then
    echo "imgtool $IMGTOOL_COMMAND_ARG signed .hex."
    echo "$IMGTOOL_SCRIPT_NAME $IMGTOOL_COMMAND_ARG $FLASH_ERASE_ARG -e little --pad-header --align 8 -H $MCUBOOT_HEADER_SIZE -M $MCUBOOT_MAX_IMG_SECTORS -v $APP_BUILD_VERSION -L $FLASH_AREA_IMG_1_PRIMARY_START -S $FLASH_AREA_IMG_1_PRIMARY_SIZE $CY_SIGNING_KEY_ARG $CY_OUTPUT_HEX $CY_OUTPUT_SIGNED_HEX"
    $CY_PYTHON_PATH $MCUBOOT_SCRIPT_FILE_DIR/$IMGTOOL_SCRIPT_NAME $IMGTOOL_COMMAND_ARG $FLASH_ERASE_ARG -e little --pad-header --align 8 -H $MCUBOOT_HEADER_SIZE -M $MCUBOOT_MAX_IMG_SECTORS -v $APP_BUILD_VERSION -L $FLASH_AREA_IMG_1_PRIMARY_START -S $FLASH_AREA_IMG_1_PRIMARY_SIZE $CY_SIGNING_KEY_ARG $CY_OUTPUT_HEX $CY_OUTPUT_SIGNED_HEX
fi
#
# Convert signed hex file to Binary
echo "Signed .hex to .bin"
"$CY_HEX_TO_BIN" --input-target=ihex --output-target=binary $CY_OUTPUT_SIGNED_HEX $CY_OUTPUT_BIN

# get size of binary file for components.json
BIN_SIZE=$(ls -g -o $CY_OUTPUT_BIN | awk '{printf $3}')

# Navigate to build directory

pushd $CY_OUTPUT_PATH

# create "components.json" file
echo "{\"numberOfComponents\":\"2\",\"version\":\"$APP_BUILD_VERSION\",\"files\":["                    >  $CY_COMPONENTS_JSON_NAME
echo "{\"fileName\":\"components.json\",\"fileType\": \"component_list\"},"                             >> $CY_COMPONENTS_JSON_NAME
echo "{\"fileName\":\"$CY_OUTPUT_FILE_NAME_BIN\",\"fileType\": \"NSPE\",\"fileSize\":\"$BIN_SIZE\"}]}" >> $CY_COMPONENTS_JSON_NAME

# create tarball for OTA
echo "Create tarball"
tar -cf $CY_OUTPUT_FILE_NAME_TAR $CY_COMPONENTS_JSON_NAME $CY_OUTPUT_FILE_NAME_BIN

#
#
# When we build WiFi & BT FW into separate block
#
#
popd

if [ "$CY_BUILD_SEPARATE_FW" == "FW_DATBLOCK_SEPARATE_FROM_APPLICATION" ]
then
    # Build the FW Data Block
    if [ "$FW_DATA_BLOCK_WIFI_FW_SOURCE" == "NONE" ]
    then
        WIFI_ARGUMENT=" "
    else
        WIFI_ARGUMENT="-wifi_src $FW_DATA_BLOCK_WIFI_FW_SOURCE"
    fi

    if [ "$FW_DATA_BLOCK_CLM_BLOB_SOURCE" == "NONE" ]
    then
        CLM_ARGUMENT=" "
    else
        CLM_ARGUMENT="-clm_src $FW_DATA_BLOCK_CLM_BLOB_SOURCE"
    fi

    if [ "$FW_DATA_BLOCK_BT_FW_SOURCE" == "NONE" ]
    then
        BT_ARGUMENT=" "
    else
        BT_ARGUMENT="-bt_src $FW_DATA_BLOCK_BT_FW_SOURCE"
    fi

    echo ""
    echo "WIFI_ARGUMENT $WIFI_ARGUMENT"
    echo "CLM_ARGUMENT  $CLM_ARGUMENT"
    echo "BT_ARGUMENT   $BT_ARGUMENT"
    echo ""

    echo "$CY_PYTHON_PATH $FW_DATA_BLOCK_BUILDER -l $WIFI_ARGUMENT $CLM_ARGUMENT $BT_ARGUMENT -out_file $FW_DATA_BLOCK_OUTPUT_FILE_BIN"
    $CY_PYTHON_PATH $FW_DATA_BLOCK_BUILDER -l $WIFI_ARGUMENT $CLM_ARGUMENT $BT_ARGUMENT -out_file $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_BIN

    # Convert to Hex
    echo "$CY_PYTHON_PATH $CY_PYTHON_PATH/../Scripts/bin2hex.py $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_BIN $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_UNSIGNED_HEX"
    $CY_PYTHON_PATH $CY_PYTHON_PATH/../Scripts/bin2hex.py $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_BIN $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_UNSIGNED_HEX
    ls -l $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_WILD

    # sign the hex file and place at Primary image 2 start
    echo "$CY_PYTHON_PATH $MCUBOOT_SCRIPT_FILE_DIR/$IMGTOOL_SCRIPT_NAME $IMGTOOL_COMMAND_ARG $EXT_FLASH_ERASE_ARG -e little --pad-header --align 8 -H $CY_EXT_MCUBOOT_HEADER_SIZE -M $MCUBOOT_MAX_FW_IMG_SECTORS -v $APP_BUILD_VERSION -x $CY_BOOT_PRIMARY_1_IMAGE_2_START -S $CY_BOOT_PRIMARY_1_IMAGE_2_SIZE $CY_SIGNING_KEY_ARG $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_UNSIGNED_HEX $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_HEX"
    $CY_PYTHON_PATH $MCUBOOT_SCRIPT_FILE_DIR/$IMGTOOL_SCRIPT_NAME $IMGTOOL_COMMAND_ARG $EXT_FLASH_ERASE_ARG -e little --pad-header --align 8 -H $CY_EXT_MCUBOOT_HEADER_SIZE -M $MCUBOOT_MAX_FW_IMG_SECTORS -v $APP_BUILD_VERSION -x $CY_BOOT_PRIMARY_1_IMAGE_2_START -S $CY_BOOT_PRIMARY_1_IMAGE_2_SIZE $CY_SIGNING_KEY_ARG $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_UNSIGNED_HEX $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_HEX

    # Convert signed hex file to Binary
    echo "Signed $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_HEX $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_BIN"
    "$CY_HEX_TO_BIN" --input-target=ihex --output-target=binary $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_HEX $CY_OUTPUT_PATH/$FW_DATA_BLOCK_OUTPUT_FILE_BIN

    pushd $CY_OUTPUT_PATH

    # Make tar archive
    # get size of binary file for components.json
    FW_DATA_BLOCK_SIZE=$(ls -g -o $FW_DATA_BLOCK_OUTPUT_FILE_BIN | awk '{printf $3}')

    # create "components.json" file
    echo "{\"numberOfComponents\":\"3\",\"version\":\"$APP_BUILD_VERSION\",\"files\":["                    >  $CY_COMPONENTS_JSON_NAME
    echo "{\"fileName\":\"components.json\",\"fileType\": \"component_list\"},"                             >> $CY_COMPONENTS_JSON_NAME
    echo "{\"fileName\":\"$CY_OUTPUT_FILE_NAME_BIN\",\"fileType\": \"NSPE\",\"fileSize\":\"$BIN_SIZE\"}," >> $CY_COMPONENTS_JSON_NAME
    echo "{\"fileName\":\"$FW_DATA_BLOCK_OUTPUT_FILE_BIN\",\"fileType\": \"FWDB\",\"fileSize\":\"$FW_DATA_BLOCK_SIZE\"}]}" >> $CY_COMPONENTS_JSON_NAME

    # create tarball for OTA
    echo "Create tarball"
    tar -cf $CY_OUTPUT_FILE_NAME_TAR $CY_COMPONENTS_JSON_NAME $CY_OUTPUT_FILE_NAME_BIN $FW_DATA_BLOCK_OUTPUT_FILE_BIN

    popd
fi

pushd $CY_OUTPUT_PATH
echo ""
echo "Application Name                         : $CY_OUTPUT_NAME"
echo "Application Version                      : $APP_BUILD_VERSION"
echo "Cypress MCUBoot Header size              : $MCUBOOT_HEADER_SIZE"
if [ "$CY_BUILD_SEPARATE_FW" == "FW_DATBLOCK_SEPARATE_FROM_APPLICATION" ]
then
echo "Primary 1 Slot Image 1 Start             : $FLASH_AREA_IMG_1_PRIMARY_START"
echo "Primary 1 Slot Image 1 Size              : $FLASH_AREA_IMG_1_PRIMARY_SIZE"
echo "Primary 1 Slot Image 2 Start             : $CY_BOOT_PRIMARY_1_IMAGE_2_START"
echo "Primary 1 Slot Image 2 Size              : $CY_BOOT_PRIMARY_1_IMAGE_2_SIZE"
echo "FLASH ERASE Value                        : $FLASH_ERASE_VALUE"
echo "EXT FLASH ERASE Value                    : $CY_EXTERNAL_FLASH_ERASE_VALUE"
echo "CY_EXT_MCUBOOT_HEADER_SIZE               : $CY_EXT_MCUBOOT_HEADER_SIZE"
else
echo "Primary 1 Slot Start                     : $FLASH_AREA_IMG_1_PRIMARY_START"
echo "Primary 1 Slot Size                      : $FLASH_AREA_IMG_1_PRIMARY_SIZE"
echo "FLASH ERASE Value                        : $FLASH_ERASE_VALUE"
fi
echo "Max 512 bytes sectors for Application    : $MCUBOOT_MAX_IMG_SECTORS"
if [ "$SIGNING_KEY_PATH" != "" ]
then
    echo "Signing key: $SIGNING_KEY_PATH"
fi
echo ""

ls -l $CY_OUTPUT_FILE_WILD
if [ "$CY_BUILD_SEPARATE_FW" == "FW_DATBLOCK_SEPARATE_FROM_APPLICATION" ]
then
ls -l $FW_DATA_BLOCK_OUTPUT_FILE_WILD
fi
echo ""

echo "$CY_OUTPUT_FILE_NAME_TAR File List"
# print tar file list
tar -t -f $CY_OUTPUT_FILE_NAME_TAR

echo ""
