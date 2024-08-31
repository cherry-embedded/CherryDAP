import argparse
import os

argparser = argparse.ArgumentParser(description='Merge binary files')
# 保存合并后的bin文件
argparser.add_argument('-o', '--output', required=True, help='Output file')
argparser.add_argument('files', nargs='+', help='Files to merge, the format is `path@offset`, e.g. `/home/hs/example.bin@0x200`')

args = argparser.parse_args()

def merge_files(files: list):
    # 两个bin文件中间的空白区域用0xFF填充
    # offset是相对于合并之后的bin文件的偏移量
    merged = b''
    for file in files:
        path, offset = file.split('@')
        offset = int(offset, 16)
        with open(path, 'rb') as f:
            bin = f.read()
            merged += b'\xFF' * (offset - len(merged))
            merged += bin
    return merged

merged = merge_files(args.files)
with open(os.path.join(args.output, 'Merger.bin'), 'wb') as f:
    f.write(merged)
