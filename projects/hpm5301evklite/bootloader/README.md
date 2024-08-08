# CherryDAP HPM5301系列的Bootloader

## 注意事项
1. Boot区预留为128k，APP从`0x80020000`开始。
2. APP工程的`HPM_BUILD_TYPE`必须为`flash_uf2`，编译出来的bin才可以转换为`.uf2`升级
