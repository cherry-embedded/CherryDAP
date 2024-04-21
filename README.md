# CherryDAP

CherryDAP is a DAPLink template based on [CherryUSB](https://github.com/sakumisu/CherryUSB) and [ARMmbed DAPLink](https://github.com/ARMmbed/DAPLink).

# Feature

- CMSIS DAP version 2.1
- Support SWD + JTAG
- Support USB2UART

![cherrydap1](./assets/cherrydap1.png)
![cherrydap2](./assets/cherrydap2.png)
![ses_debug_hpm](./assets/ses_debug_hpm.png)
# Projects

## BL616

- USB High speed
- UART with cycle dma(no packet will be lost), so the baudrate can reach 40Mbps

| Function | Label | GPIO |
|:--------:|:-----:|:----:|
| JTAG_TCK | IO10 | 10 |
| JTAG_TMS | IO12 | 12 |
| JTAG_TDI | IO14  | 14 |
| JTAG_TDO | IO16  | 16 |
| SWD_SWCLK | IO10 | 10 |
| SWD_SWDIO | IO12 | 12 |
| UART TX | IO11 | 11 |
| UART RX | IO13 | 13 |
| nRESET | - | - |

![m0sdock](./assets/m0sdock.png)
![m0sdock2](./assets/m0sdock2.png)

You can compile with:

```
cd projects/bl616

make BL_SDK_BASE=<pwd of bouffalo_sdk prefix>/bouffalo_sdk CROSS_COMPILE=<pwd of toolchain prefix>/toolchain_gcc_t-head_linux/bin/riscv64-unknown-elf-
```

## HPM5301EVKlite
![hpm5301evklite](./assets/hpm5301evklite.png)

- USB High speed
- Support UART, use DMA(@[**RCSN** ](https://github.com/RCSN))
- The 20PJTAG socket of the J5 interface is used by default. 
- support  JTAG (@[**RCSN** ](https://github.com/RCSN))
- support SWD (@[**Aladdin-Wang**](https://github.com/Aladdin-Wang))
- for example, use hpm5301evklite debug hpm5300evk

![debug_5300evk](./assets/debug_5300evk.png)

- Use uart3 as usb2uart

![53uart](./assets/53uart.png)


- build


1、 sdk version must be greater than 1.3

2、 download https://github.com/hpmicro/sdk_env

3、 if the sdk is not hpm5300evklite, you can download the pack unzip to sdk_env/hpm_sdk   https://github.com/hpmicro/hpm_sdk/releases/download/v1.3.0/hpm_sdk_v1.3.0_patch-hpm5301evklite.zip

4、open sdk_env start_gui.exe on window

![sdk_env](./assets/sdk_env.png)


- download firmware

- the firmware bin file path: ../bin/hpm5301_daplink.bin

1、use hpm_manufacturing_tool  https://github.com/hpmicro/hpm_manufacturing_tool

(1) baidu pan: https://pan.baidu.com/s/1RaYHOD7xk7fnotmgLpoAlA?pwd=xk2n 
提取码：xk2n 复制这段内容打开「百度网盘APP 即可获取」

![hpm_acc_tools](./assets/hpm_acc_tools.png)


(2) unzip, open hpm_manufacturing_gui.exe,

(3) uart0 use usbttl module to connect tool, and press the SW1 and SW2 buttons simultaneously, then release SW1 (RESET), and then release SW2 (boot)

![hpm_isp_uart](./assets/hpm_isp_uart.png)

(4) connenct the tool and download firmware

![hpm_download](./assets/hpm_download.png)


2、use ses ide

3、use gdb

