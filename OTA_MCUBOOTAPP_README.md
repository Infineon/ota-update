
# MCUBootApp Usage Instructions


## 1. Clone MCUBoot

- You need to first build and program the bootloader app *MCUBootApp* that is available in the MCUBoot GitHub repo before using an OTA application.

    - MCUBootApp runs on the CM0+ CPU and starts any OTA enabled application on CM4 core in case of multicore Psoc6 devices.
    - MCUBootApp runs on CM33 CPU along with the OTA enabled applications in the case of 20829 devices.

- Clone the MCUBoot repository onto your local machine, **outside of your application directory.**

-  Open a CLI terminal and run the following command:

   ```
   git clone https://github.com/JuulLabs-OSS/mcuboot.git
   ```

- Navigate to the cloned *mcuboot* folder:
   ```
   cd mcuboot
   ```

- Change the branch to get the appropriate version:

   ```
   git checkout v1.8.4-cypress
   ```

- Pull in sub-modules to build mcuboot:

   ```
   git submodule update --init --recursive
   ```

- Install the required Python packages mentioned in *mcuboot/scripts/requirements.txt*. Be sure to use the python instance in ModusToolbox/tools_XX/python:
   ```
   cd mcuboot/scripts

   pip install -r requirements.txt
   ```

<b>Note:</b>
- The **CY8CKIT-064B0S2-4343W** is a special kit that uses cysecureboot. See [cysecuretools for PSoC™ 64 and CYW20829 devices](#5-cysecuretools-for-psoc-64-and-cyw20829-devices) information section below.

## 2. UART differences

The UART Tx and Rx pins are common across most TARGET boards. However, there are exceptions. If you see not "printf()" output to the terminal, check the UART settings. The UART #defines can be changed for your device. Edit <mcboot>/boot/cypress/MCUBootApp/config/custom_debug_uart_cfg.h and set the defines as appropriate for your device, and include "USE_CUSTOM_DEBUG_UART=1" to the make command.

## 3. Build MCUBoot

MCUBootApp supports using a JSON document to define the flash regions used for updating the application.

Read **[MCUBoot Build Commands](./configs/flashmap/MCUBoot_Build_Commands.md)** to see the flashmaps and build command to use. This ****must**** be the same flashmap used to buid your OTA application.

<b>Notes:</b>
- The JSON files use ABSOLUTE address, which indicate which flash part is to be used for the particular section. The usage in MCUBootApp and OTA library defines the location by device_id and RELATIVE address. You do not need to do anything, this information is to reduce confusion.

- Watch the MCUBootApp build output. If the python script flashmap.py detects errors in the flash layout, it will report them to stderr and stop the build.

## 4. Program MCUBootApp

- Connect the board to your PC using the provided USB cable through the USB connector.

- From the terminal, execute the make program command to program the application using the default toolchain to the default target.
    ```
    make program TARGET=<BSP> TOOLCHAIN=<toolchain>
    ```

- Alternatively, program the board using CyProgrammer or the instructions in your Customer Example Application notes.

## 5. cysecuretools for PSoC™ 64 and CYW20829 devices

For CY8CKIT-062B0S2-4343W kit and CYW20829 devices (SECURE), we use a procedure called "provisioning" to put the CyBootloader into the device. Please refer to the following cysecuretools documents for this procedure.

[Provisioning Steps for CY8CKIT-064B0S2-4343W](https://github.com/Infineon/cysecuretools/blob/master/docs/README_PSOC64.md)

[Provisioning Steps for CYW20829 devices](https://github.com/Infineon/cysecuretools/blob/master/docs/README_CYW20829.md)

<b>Note:</b>
- For 20829 NORMAL_NO_SECURE mode, MCUBootApp should be built and programmed separately.

- Windows version of ModusToolbox 3.0/3.1 contains a Python package with CySecureTools 4.1.0. But, mcuboot v1.8.4-cypress requires the latest CySecureTools 4.2.0 version of the package for 20829 platforms.

- The following command can be used in the modus-shell terminal to find the current version of CySecureTools.
    ````
    cysecuretools version
    ````

- If the current CySecureTools is not the latest 4.2.0 version, the following command can be used in modus-shell terminal to update it.
    ````
    pip install --upgrade --force-reinstall cysecuretools
    ````
