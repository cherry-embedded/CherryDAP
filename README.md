# CherryDAP

CherryDAP is a DAPLink template based on [CherryUSB](https://github.com/sakumisu/CherryUSB) and [ARMmbed DAPLink](https://github.com/ARMmbed/DAPLink).

# Feature

- CMSIS DAP version 2.1
- Support SWD + JTAG
- Support USB2UART

![cherrydap1](./assets/cherrydap1.png)
![cherrydap2](./assets/cherrydap2.png)

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
