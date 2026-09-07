cd boot/tinyuf2
cmake -S . -B build -GNinja -DBOARD=hpm5301evklite -DHPM_BUILD_TYPE=flash_xip; cmake --build build
cd -
cmake -S . -B build -GNinja -DBOARD=hslinklite -DHPM_BUILD_TYPE=flash_uf2; cmake --build build
cp boot/tinyuf2/build/output/demo.bin HSLink-Lite.bin
dd if=build/output/HSLink-Lite.bin of=HSLink-Lite.bin bs=1024 seek=127 conv=notrunc