
OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)
ENTRY(Reset_Handler)


STACK_SIZE = 0x1000;


/*
   Arguments for OTA using MCUBoot -- will get from passed in Makefile:
   --defsym,MCUBOOT_HEADER_SIZE=XXXX
   --defsym,FLASH_AREA_IMG_1_PRIMARY_START=XXXX
   --defsym,FLASH_AREA_IMG_1_PRIMARY_SIZE=XXXX
*/

EXTERN(Reset_Handler)

MEMORY
{
    ram (rwx)      : ORIGIN = 0x08002000, LENGTH = 0x3D800
    flash (rx)     : ORIGIN = 0x18000000 + FLASH_AREA_IMG_1_PRIMARY_START, LENGTH = 0x8000000
    em_eeprom (rx) : ORIGIN = 0x14000000, LENGTH = 0x8000
    xip (rx)       : ORIGIN = 0x18000000, LENGTH = 0x8000000
}


GROUP(libgcc.a libc.a libm.a libnosys.a)
SECTIONS
{
    .text ORIGIN(flash) + MCUBOOT_HEADER_SIZE :
    {
        . = ALIGN(4);
        __Vectors = . ;
        KEEP(*(.vectors))
        . = ALIGN(4);
        __Vectors_End = .;
        __Vectors_Size = __Vectors_End - __Vectors;
        __end__ = .;

        . = ALIGN(4);

        /* Exclude external flash access code from running in external flash - added as RAM functions in appTextRam */
        EXCLUDE_FILE(
                        /* Add Application files entries here.*/

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

        KEEP(*(.init))
        KEEP(*(.fini))

        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        KEEP(*(.eh_frame*))
    } > flash

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > flash

    __exidx_start = .;

    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > flash
    __exidx_end = .;

    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;


        LONG (__Vectors)
        LONG (__ram_vectors_start__)
        LONG (__Vectors_End - __Vectors)


        LONG (__etext)
        LONG (__data_start__)
        LONG (__data_end__ - __data_start__)

        __copy_table_end__ = .;
    } > flash

    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        LONG (__bss_start__)
        LONG (__bss_end__ - __bss_start__)
        __zero_table_end__ = .;
    } > flash

    __etext = . ;

    .ramVectors (NOLOAD) : ALIGN(8)
    {
        __ram_vectors_start__ = .;
        KEEP(*(.ram_vectors))
        __ram_vectors_end__ = .;
    } > ram

    .data __ram_vectors_end__ : AT (__etext)
    {
        . = ALIGN(4);
        __data_start__ = .;

        *(vtable)
        *(.data*)

        . = ALIGN(4);

        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);

        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(4);

        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);

        /* Add Application files entries here.*/

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

        . = ALIGN(4);

        __data_end__ = .;

    } > ram

    .noinit (NOLOAD) : ALIGN(8)
    {
      KEEP(*(.noinit))
    } > ram

    .bss (NOLOAD):
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > ram

    .heap (NOLOAD):
    {
        __HeapBase = .;
        __end__ = .;
        end = __end__;
        KEEP(*(.heap*))
        . = ORIGIN(ram) + LENGTH(ram) - STACK_SIZE;
        __HeapLimit = .;
    } > ram

    .stack_dummy (NOLOAD):
    {
        KEEP(*(.stack*))
    } > ram

    __StackTop = ORIGIN(ram) + LENGTH(ram);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")


    .cy_app_signature ORIGIN(flash) + LENGTH(flash) - 256 :
    {
        KEEP(*(.cy_app_signature))
    } > flash

    .cy_em_eeprom (NOLOAD):
    {
        KEEP(*(.cy_em_eeprom))
    } > em_eeprom

/*
    .cy_xip :
    {
        __cy_xip_start = .;
        KEEP(*(.cy_xip))
        __cy_xip_end = .;
    } > xip
*/

}


/* The following symbols used by the cymcuelftool. */
/* Flash */
__cy_memory_0_start    = 0x10000000;
__cy_memory_0_length   = 0x00200000;
__cy_memory_0_row_size = 0x200;

/* Emulated EEPROM Flash area */
__cy_memory_1_start    = 0x14000000;
__cy_memory_1_length   = 0x8000;
__cy_memory_1_row_size = 0x200;

/* Supervisory Flash */
__cy_memory_2_start    = 0x16000000;
__cy_memory_2_length   = 0x8000;
__cy_memory_2_row_size = 0x200;

/* XIP */
__cy_memory_3_start    = 0x18000000;
__cy_memory_3_length   = 0x08000000;
__cy_memory_3_row_size = 0x00040000;

/* eFuse */
__cy_memory_4_start    = 0x90700000;
__cy_memory_4_length   = 0x100000;
__cy_memory_4_row_size = 1;

/* EOF */
