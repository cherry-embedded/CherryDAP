#!/bin/bash

old_path="/home/runner/work/_temp/.setup-riscv-gnu-toolchain/bin"

new_path="/home/runner/risv32-unknown-elf/bin"
mkdir -p $new_path

cd $old_path

# 查找所有以 "riscv-none-elf-" 开头的文件
for file in riscv-none-elf-*; do
    # 如果文件存在
    if [[ -e "$file" ]]; then
        # 构造新的链接名称
        new_link="$new_path/riscv32-unknown-elf-${file#riscv-none-elf-}"
        
        # 创建软链接
        ln -s "$old_path/$file" "$new_link"

        printf "Create symlink: %s -> %s\n" "$new_link" "$file"
    fi
done