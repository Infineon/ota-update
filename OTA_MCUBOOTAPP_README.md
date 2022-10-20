
# MCUBootApp Usage Instructions


## 1. Clone MCUBoot

You need to first build and program the bootloader app *MCUBootApp* that is available in the MCUBoot GitHub repo before using an OTA application. MCUBootApp runs on the CM0+ CPU and starts any OTA enabled application on CM4. Clone the MCUBoot repository onto your local machine, **outside of your application directory.**

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
   git checkout v1.8.3-cypress
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
- The **CY8CKIT-064B0S2-4343W** is a special kit that uses cysecureboot. See "cysecuretools for CY8CKIT-064B0S2-4343W" information below.

## 2. UART differences

The UART Tx and Rx pins are common across most TARGET boards. However, there are exceptions. If you see not "printf()" output to the terminal, check the UART settings. The UART #defines can be changed for your device. Edit <mcboot>/boot/cypress/MCUBootApp/config/custom_debug_uart_cfg.h and set the defines as appropriate for your device, and include "USE_CUSTOM_DEBUG_UART=1" to the make command.

## 3. Build MCUBoot

MCUBootApp supports using a JSON document to define the flash regions used for updating the application.

Read **"\<ota-update library\>/configs/flashmap/MCUBoot_Build_Commands.md"** to see the flashmaps and build command to use. This ****must**** be the same flashmap used to buid your OTA application.

<b>Notes:</b>
- The JSON files use ABSOLUTE address, which indicate which flash part is to be used for the particular section. The usage in MCUBootApp and OTA library defines the location by device_id and RELATIVE address. You do not need to do anything, this information is to reduce confusion.

- Watch the MCUBootApp build output. If the python script flashmap.py detects errors in the flash layout, it will report them to stderr and stop the build.

## 4. Program MCUBootApp

-  Connect the board to your PC using the provided USB cable through the USB connector.

 - Program the board using CyProgrammer or the instructions in your Customer Example Application notes.

## 5. cysecuretools for CY8CKIT-064B0S2-4343W

For the CY8CKIT-062B0S2-4343W, we use a procedure called "provisioning" to put the CyBootloader into the device. Please refer to the board instructions for this procedure.

[PSoC™ 64 Secure Boot Wi-Fi BT Pioneer Kit](https://www.cypress.com/documentation/development-kitsboards/psoc-64-secure-boot-wi-fi-bt-pioneer-kit-cy8ckit-064b0s2-4343w) (CY8CKIT-064B0S2-4343W)

## 6. Additional Information

- [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.cypress.com/products/modustoolbox-software-environment)
-  [MCUboot](https://github.com/JuulLabs-OSS/mcuboot/blob/cypress/docs/design.md) documentation

Infineon also provides a wealth of data at www.infineon.com to help you select the right device, and quickly and effectively integrate it into your design.

For PSoC™ 6 MCU devices, see [How to Design with PSoC 6 MCU - KBA223067](https://community.cypress.com/docs/DOC-14644) in the Infineon community.

## 7. Document History

| Document Version | Description of Change                                      |
| ---------------- | ---------------------------------------------------------- |
| 1.0.1            | Add UART information                                       |
| 1.0.0            | New MCUBootApp usage document                                     |

------

© 2022, Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
This software, associated documentation and materials ("Software") is owned by Cypress Semiconductor Corporation or one of its affiliates ("Cypress") and is protected by and subject to worldwide patent protection (United States and foreign), United States copyright laws and international treaty provisions. Therefore, you may use this Software only as provided in the license agreement accompanying the software package from which you obtained this Software ("EULA"). If no EULA applies, then any reproduction, modification, translation, compilation, or representation of this Software is prohibited without the express written permission of Cypress.
Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress reserves the right to make changes to the Software without notice. Cypress does not assume any liability arising out of the application or use of the Software or any product or circuit described in the Software. Cypress does not authorize its products for use in any products where a malfunction or failure of the Cypress product may reasonably be expected to result in significant property damage, injury or death ("High Risk Product"). By including Cypress's product in a High Risk Product, the manufacturer of such system or application assumes all risk of such use and in doing so agrees to indemnify Cypress against all liability.
