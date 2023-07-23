手动屏蔽 bouffalo_sdk 中 drivers/lhal 下的 CMakelists.txt 中的 CONFIG_CHERRYUSB, 改成如下

```
if("${CHIP}" STREQUAL "bl702")
sdk_library_add_sources(src/bflb_usb_v1.c)
elseif(("${CHIP}" STREQUAL "bl602") OR ("${CHIP}" STREQUAL "bl702l"))
# no usb
elseif(("${CHIP}" STREQUAL "bl628"))
else()
sdk_library_add_sources(src/bflb_usb_v2.c)
endif()
```