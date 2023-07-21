
# Linker files for OTA update

## 1. Introduction

This guide shows template linker file usage when building an application with ota-update library support. The OTA library provides template OTA linker files for MCUBootloader based OTA application on all supported targets.

## 2. Template linker files for Non XIP Mode

OTA library user expected to <br>
- Copy OTA template linker file based on selected target to application space as per below table.

| Target | Internal<br>Flash size | Template linkers |
|-------------------|---------------------|----------|
| CY8CKIT-062S2-43012<br>CY8CPROTO-062-4343W<br>CY8CEVAL-062S2-LAI-4373M2<br>CY8CEVAL-062S2-MUR-43439M2| 2M | GCC_ARM - cy8c6xxa_cm4_dual_ota_int.ld<br> |
| CY8CKIT-064B0S2-4343W| 2M | GCC_ARM - cyb06xxa_cm4_dual_ota_int.ld |
| CY8CKIT-062-BLE | 1M | GCC_ARM - cy8c6xx7_cm4_dual_ota_cm0p_int.ld |
| CY8CPROTO-063-BLE<br>CYBLE-416045-EVAL | 1M | GCC_ARM - cy8c6xx7_cm4_dual_ota_cm0p_int.ld |
| CY8CPROTO-062S3-4343W  | 512K | GCC_ARM - cy8c6xx5_cm4_dual_ota_int.ld |

<br>

- Provide OTA linker file path in application makefile.<br>
    ```
    # Provide path to the OTA linker script (Must required).
    OTA_LINKER_FILE=< Path to OTA linker file >
    ```
<br>

<b>NOTE:</b> <br>
- Template linker files are provided only for GCC_ARM compiler.<br>
- Template linker files for Non-XIP mode are available [here](./template_linkers).<br>
- Template linker files expects below `LDFLAGS` and these `LDFLAGS` will be added by ota-update library by default.<br>
    ```
    # These LDFLAGS values will be available in generated `flashmap.mk` file.
    MCUBOOT_HEADER_SIZE=XXXX
    FLASH_AREA_IMG_1_PRIMARY_START=XXXX
    FLASH_AREA_IMG_1_PRIMARY_SIZE=XXXX
    ```
<br>

## 3. Template linker files for XIP Mode

OTA library user expected to <br>
- Copy OTA template linker file based on selected target to application space as per below table.

| Target | Internal<br>Flash size | Template linkers |
|-------------------|---------------------|----------|
| CYW920829M2EVK-02 | 0K | GCC_ARM - cyw20829_ns_flash_sahb_ota_xip.ld |
| CY8CPROTO-062S3-4343W  | 512K | GCC_ARM - cy8c6xx5_cm4_dual_ota_xip.ld |

<br>

- Update copied template linker file to make it work in XIP mode. Refer section [Steps for updating XIP linker file](./OTA_MCUBOOT_LINKER_FILES_README.md#4-steps-for-updating-xip-linker-file) for details.
- Provide OTA linker file path in application makefile.<br>
    ```
   # Provide path to the OTA XIP linker script (Must required).
    OTA_LINKER_FILE=< Path to OTA linker file >
    ```
<br>

<b>NOTE:</b><br>
Current version of library supports XIP mode for CY8CPROTO-062S3-4343W and 20829 kits.<br>
Linker file templates for XIP mode are available [here](./template_linkers). By default, pdl and hal files which access XIP memory are placed in RAM sections in these templates.<br>


## 4. Steps for updating XIP linker file
In the below steps, 'app_serial_flash.c' is just reference application file name that implement OTA flash callback APIs and have all the data structure for accessing flash in XIP mode.<br>
Replace the reference file name with the actual application file name.

1. Update memory sections of copied template XIP linker file to exclude application files which implement OTA flash callback APIs from the flash memory section. In the template XIP linker file update EXCLUDE_FILE section as below.
```
       /* Exclude files which access external flash code from running in an external flash */
       EXCLUDE_FILE(
                   /* Add Application files entries here.*/
                    *app_serial_flash.o

                   /* Files, OTA library expects to be in RAM */
                    *cy_syslib_ext.o
                    *lib_a-memset.o
                    *cy_smif.o
                    *cy_smif_memslot.o
                    *cy_smif_psoc6.o
                    *cy_smif_sfdp.o
                    *cyhal_qspi.o
                    *cy_syslib.o
                    *cycfg_qspi_memslot.o
                    *bootutil_misc.o
                   ) *(.text) *(.vfp11)
```

2. Move application files that implement OTA flash callback APIs to the RAM section.
   ```
       .data __ram_vectors_end__ : AT (__etext)
       {
          /* Add Application files entries here.*/
           *app_serial_flash.o(.text* .rodata .rodata* .constdata .constdata* .conststring .conststring*)

          /* Files, OTA library expects to be in RAM */
           *cy_syslib_ext.o(.text* .rodata .rodata* .constdata .constdata* .conststring .conststring*)
           *lib_a-memset.o(.text* .rodata .rodata* .constdata .constdata* .conststring .conststring*)
           *cy_smif.o(.text* .rodata .rodata* .constdata .constdata* .conststring .conststring*)
           *cy_smif_memslot.o(.text* .rodata .rodata* .rodata.CSWTCH.* .constdata .constdata* .conststring .conststring*)
           *cy_smif_psoc6.o(.text* .rodata .rodata* .constdata .constdata* .conststring .conststring*)
           *cy_smif_sfdp.o(.text* .rodata .rodata* .constdata .constdata* .conststring .conststring*)
           *cyhal_qspi.o(.text* .rodata .rodata* .constdata .constdata* .conststring .conststring*)
           *cy_syslib.o(.text* .rodata .rodata* .constdata .constdata* .conststring .conststring*)
           *cycfg_qspi_memslot.o(.text* .rodata .rodata* .constdata .constdata* .conststring .conststring*)
           *bootutil_misc.o(.text* .rodata .rodata* .constdata .constdata* .conststring .conststring*)
       } > ram
   ```

## 5. Additional Information

- [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)

Infineon also provides a wealth of data at www.infineon.com to help you select the right device, and quickly and effectively integrate it into your design.

For PSoC™ 6 MCU devices, see [How to Design with PSoC 6 MCU - KBA223067](https://community.infineon.com/t5/Knowledge-Base-Articles/How-to-Design-with-PSoC-6-MCU-KBA223067/ta-p/248857) in the Infineon community.

