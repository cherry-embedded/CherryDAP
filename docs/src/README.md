---
home: true
icon: home
title: CherryDAP
# heroImage: https://theme-hope-assets.vuejs.press/logo.svg
bgImage: https://theme-hope-assets.vuejs.press/bg/6-light.svg
bgImageDark: https://theme-hope-assets.vuejs.press/bg/6-dark.svg
bgImageStyle:
  background-attachment: fixed
heroText: CherryDAP
tagline: CherryDAP 是一个使用了 CherryUSB 作为协议栈的 DAPLink 实现
actions:
  - text: 示例工程
    icon: lightbulb
    link: ./projects/
    type: primary

  - text: Docs
    link: ./guide/

highlights:
  - header: 支持特性
    # description: CherryDAP 支持的特性
    image: /assets/image/features.svg
    bgImage: https://theme-hope-assets.vuejs.press/bg/9-light.svg
    bgImageDark: https://theme-hope-assets.vuejs.press/bg/9-dark.svg
    highlights:
      - title: CMSIS DAP version 2.1
        # icon: circle-half-stroke
        # details: Switch between light and dark modes freely
        # link: https://theme-hope.vuejs.press/guide/interface/darkmode.html

      - title: 完善的调试协议支持
        details: 支持SWD和JTAG调试协议
      
      - title: 虚拟串口支持

  - header: 示例工程
    description: CherryDAP 提供了一系列 MCU 的示例工程
    image: /assets/image/advanced.svg
    bgImage: https://theme-hope-assets.vuejs.press/bg/5-light.svg
    bgImageDark: https://theme-hope-assets.vuejs.press/bg/5-dark.svg
    highlights:
      - title: BL616
        # icon: window-maximize
        # details: Fully customizable navbar with improved mobile support
        link: ./projects/BL616
      
      - title: stm32f429igt6
        link: ./projects/stm32f429igt6

      - title: HPM5301EVKLite
        link: ./projects/HPM5301EVKLite

      - title: HSLink Pro
        link: ./projects/HSLinkPro
---