# HSLink Pro

## 升级流程

1. 长按`BL`按钮5秒以上即可进入升级模式，此时电脑上会出现一个`CHERRYUF2`的移动存储设备，将升级的`.uf2`文件拖入其中即可。最新的固件可以在<https://github.com/HalfSweet/CherryDAP/actions>或者QQ群内自行获取。

## FAQ

Q：为什么我插上去以后，`keil(MDK5)`没有反应？
A：请检查版本号，只有5.27以上的版本才能支持CMSIS-DAP V2版本，如在此版本号以下的请自行升级。

## 已知问题

- [ ] VREF输出电平有误，实际为VREF/2
- [ ] TVcc和5V输出引脚定义有误
- [ ] PyOCD和Probe-rs无法正常连接
