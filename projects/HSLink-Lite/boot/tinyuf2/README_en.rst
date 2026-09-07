.. _tinyuf2:

TinyUF2
==============

Overview
--------

tinyuf2 example project provides a UF2 bootloader for HPMicro devices.

* At booloader mode, once it's running, the device will be listed as a USB mass storage device on PC.

An application wrapped in a UF2 container can be drag-n-drop to that USB mass storage device,
then TinyUF2 bootloader will program it to the onboard flash and jump to the application.

* At loading mode, once it's running, it will load the image already programmed by this bootloader and jump to it.

UF2 container
~~~~~~~~~~~~~

Application to be used as TinyUF2 bootable image, it must meet the following conditions:
1. it should be built with flash_uf2 or flash_sdram_uf2 linker script released in SDK.
1. run the following command to create the UF2 container (assume the application name is hello_world.bin):


.. code-block:: python

   python3 uf2conv.py -f 0x0A4D5048 -b 0x80020000 -c -o update.uf2 hello_world.bin

it will generate update.uf2, the UF2 container, from the application image hello_world.bin (starting from 0x80020000, matching its link script)
0x0A4D5048: HPMicro UF2 family ID which should be supported by uf2conv.py

Enter bootloader mode
~~~~~~~~~~~~~~~~~~~~~

Hold TinyUF2 button (please check  :ref:`Button <board_resource>`  section of sepcific board) while presing reset button

Board Setting
-------------

No special settings

Running the example
-------------------

* At loading mode, if there's valid application programmed already, it will jump to that application, and the serial port terminal will output the following information:


.. code-block:: console

   TinyUF2
   Jump to application @0x80020004(0x800291b7)

Note:
* 0x80020004 is the start address of programmed application

* 0x800291b7 is the first word instruction located at start address

* At bootloader mode, if it runs successfully, and the serial port terminal will output the following information:


.. code-block:: console

   TinyUF2
   Starting DFU mode

Then connect USB cable to USB port, a USB mass storage device will be found on PC. Then the UF2 container can be drag-n-dropped into the device.
Once the programming is completed, the application will run immediately.
