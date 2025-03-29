---
title: HSLink Pro
---

HSLink Pro 是一款使用 HPM5301 芯片的一个 CherryDAP 实现。其中主要代码参考了 [hpm5301evklite](./HPM5301EVKLite) 工程。

![](image/2024-09-23-22-25-12.png)

## 引脚定义

HSLink Pro 的引脚定义满足 [20-pin J-Link Connector](https://wiki.segger.com/20-pin_J-Link_Connector)定义。目前的引脚定义分配如下：

![](image/2024-09-23-23-02-55.png)

| 引脚 | 名称 | 描述 |
| --- | --- | --- |
| 1 | Vref | 参考电压输入 |
| 2 | Tvcc | 目标板供电可调电源输出 |
| 3 | TRST | JTAG 复位信号，通常连接到目标 CPU 的 nTRST。该引脚通常在目标上被拉高，以避免在没有连接时意外复位 |
| 4 | GND | 地 |
| 5 | TDI | JTAG TDI |
| 6 | GND | 地 |
| 7 | TMS/SWDIO | 在JTAG模式为TMS，在SWD模式下则为SWDIO |
| 8 | GND | 地 |
| 9 | TCK/SWCLK | 在JTAG模式为TCK，在SWD模式下则为SWCLK |
| 10 | GND | 地 |
| 11 | NC | 未连接 |
| 12 | GND | 地 |
| 13 | TDO | JTAG TDO |
| 14 | UART_DTR | CDC的DTR信号输出，可用于ESP32等MCU串口自动下载 |
| 15 | SRST | 目标板复位信号，低电平有效 |
| 16 | UART_RTS | CDC的RTS信号输出，可用于ESP32等MCU串口自动下载 |
| 17 | NC | 未连接 |
| 18 | UART_TX | 串口TX信号 |
| 19 | +5V | +5V 电源输出 |
| 20 | UART_RX | 串口RX信号 |

## 支持特性

目前 HSLink Pro 支持的特性有：

- [x] 提供最高80MHz的 SWD 和 JTAG 速率
- [x] 电平转换功能，适配任意电平的目标板
- [x] 同时支持 SWD 和 JTAG 协议
- [x] 支持为目标板提供最高 1A 负载的供电能力
- [x] 支持虚拟串口功能
- [x] 支持串口的 DTR/RTS 控制，可为 ESP32 等MCU提供自动下载
- [x] 支持SWD模式下对 Arm 芯片进行写`SYSRESETREQ` 和 `VECTRESET` 软复位
- [x]  支持通过上位机进行持久化配置
- [x]  支持通过上位机进行固件升级
- [x] 可通过上位机设置是否开启速度Boost
- [x] 可通过上位机设置是否开启电平转换
- [x] 可通过上位机设置是否固定输出电源
- [x] 可通过上位机设置是否开启软复位
- [x] 支持更多复位方式，例如POR

上位机下载地址：

<https://github.com/HSLink/HSLinkUpper/releases>

## HSLink Pro 升级流程

### 一般升级流程

正常连接电脑，启动完成后长按`BL`按钮2秒以上即可进入升级模式，此时电脑上会出现一个`CHERRYUF2`的移动存储设备，将升级的`.uf2`文件拖入其中即可。最新的固件可以在QQ交流群群文件或者[GitHub releases](https://github.com/cherry-embedded/CherryDAP/releases)中下载。

需要注意的是，如果是在2.3.4及之前版本升级到更新版本，需要先升级`中间包`，再升级其他版本的固件。

### V2.3.4及之前版本升级流程

对于跨越2.4.0的更新，升级到更新版本需要更新Bootloader，必须先更新`中间包`将bootloader升级之后，才能更新到其他更新版本的固件。中间包下载地址: <https://github.com/cherry-embedded/CherryDAP/releases/download/HSLinkPro-2.4.0/HSLink-Pro-Middle.uf2>，其构建的源码为：<https://github.com/HSLink/CherryDAP_HSLink/tree/upgrade-bl-use-app>。

请注意，在这个过程中请**不要断电**！！！

如果一切顺利，HSLink会在LED多次绿色和蓝色闪烁后自动重新出现`CHERRYUF2`移动存储设备，之后再拖入所需要的版本即可更新。

### 回退

一般来说，新Bootloader版本均能启动各版本APP，旧Bootloader能启动旧版本APP，进入Bootloader将所需固件拖入`CHERRYUF2`即可。但我们并未对其进行仔细测试。如果遇到恶性问题，请在QQ群中或是[Issues · cherry-embedded/CherryDAP](https://github.com/cherry-embedded/CherryDAP/issues)处提出，我们将在**master**分支尽快修复。

## FAQ

### 为什么 HSLink Pro 没有电源输出？

当Vref引脚悬空的时候，默认`Tvcc`和`+5V`引脚均不会输出电源，并且所有端口的电平将默认为`3.3V`。只有当`Vref`引脚上施加`1.6V`以上的电压的时候，`Tvcc`才会输出一个与`Vref`相同的电压，`+5V`引脚才会输出`5V`电压。

### 我的HSLink似乎变砖了？

一般情况下正常升级不会出现问题，但是如果出现了问题，可以尝试以下救砖方式：

* 对于开机闪烁2次蓝灯的版本，在闪烁期间按下按钮，即可保持Bootloader，出现`CHERRYUF2`移动存储设备。
* 对于出售的v1.2及之后的版本，Bootloader中已添加看门狗，在多次启动失败后保持在Bootloader的升级状态，即出现`CHERRYUF2`移动存储设备。对于这个版本，尝试连接usb等待30s。
* 如果你有条件使用串口下载，包括但不限于探针、夹子、焊线，推荐使用[hpmicro/hpm_manufacturing_tool: HPMicro Manufacturing Tool](https://github.com/hpmicro/hpm_manufacturing_tool/releases/tag/v0.5.0)下载[GitHub releases](https://github.com/cherry-embedded/CherryDAP/releases)中的**merge.bin**。按住按键插入usb后即可连接isp工具，默认配置即可下载。
* 对于初版也可尝试在插入usb后**立即**按住按钮，出现`CHERRYUF2`移动存储设备即可。由于窗口时间不长，请多次尝试。

## 开源资料

软件：[CherryDAP/projects/HSLink-Pro at master · cherry-embedded/CherryDAP](https://github.com/cherry-embedded/CherryDAP/tree/master/projects/HSLink-Pro)

硬件：[HSLink/HSLink_Hardware: A place to release hslink hardware](https://github.com/HSLink/HSLink_Hardware)

上位机：[HSLink/HSLinkUpper](https://github.com/HSLink/HSLinkUpper)

需求收集：[HSLink需求收集 · Issue #34 · cherry-embedded/CherryDAP](https://github.com/cherry-embedded/CherryDAP/issues/34)
