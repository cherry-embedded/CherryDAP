.. _tinyuf2:

TinyUF2
==============

概述
------

TinyUF2这个示例提供了一个UF2 bootloader。

* bootloader模式，运行起来之后，将在PC端枚举为一个u盘。这时可以将

构建在UF2容器中的一个应用，通过拖拽将其放入枚举到的u盘中。TinyUF2将把该容器
内的应用烧写进板载的flash中，烧写完成之后，将直接跳转至刚刚写入的应用。

* 启动模式, 可以通过按住TinyUF2按键(请确认具体开发板 {ref}`按键 <board_resource>` 部分描述)同时reset板子，进入bootloader模式，也可以直接跳转到已经通过TinyUF2

写入的flash镜像，若无镜像，将直接退回到bootloader模式。

UF2容器打包
~~~~~~~~~~~~~~~

一个可以被用作TinyUF2引导的应用需要满足以下条件：
1. 该应用应该使用SDK中的flash_uf2或者flash_sdram_uf2链接脚本链接
1. 运行一下命令打包生成UF2容器 （假设应用二进制文件名为hello_world.bin）:


.. code-block:: python

   python3 uf2conv.py -f 0x0A4D5048 -b 0x80020000 -c -o update.uf2 hello_world.bin

该命令把hello_world.bin（从0x80020000启动）打包生成update.uf2这个UF2容器文件。
0x0A4D5048: 先楫半导体UF2的家族ID，需要和uf2conv.py匹配使用

硬件设置
------------

无特殊设置

运行现象
------------

* 在启动模式下，如果有合法镜像已经写入正确位置，它会跳转至该应用，串口终端会输出如下信息：


.. code-block:: console

   TinyUF2
   Jump to application @0x80020004(0x800291b7)

注:
* 0x80020004 是写入的应用的起始地址

* 0x800291b7 是写入的应用起始地址上的指令

* 在bootloader模式下，如果一切正常，串口终端会输出如下信息：


.. code-block:: console

   TinyUF2
   Starting DFU mode

此时可以插入USB线，在PC端将打包的UF2容器文件拖入枚举出的u盘内。烧写完成后，它会直接跳转
到该应用。
