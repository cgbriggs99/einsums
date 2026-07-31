import os
import sys
import hashlib
import argparse

"""
Takes a string and converts it into a Windows- and POSIX-compatible string that is hopefully shorter than the original name (11 characters).
"""

__letters = "abcdefghijkmnpqrstuxyz0123456789"  # The lower case letters and numbers, with a few removed so that the number of characters is a power of 2.

assert(len(__letters) == 32)

__fnv_offset = 0xcbf29ce484222325
__fnv_prime = 0x00000100000001b3


def extract_parts(name):
    dirname, basename = os.path.split(name)
    
    base, extension = os.path.splitext(basename)
    
    return dirname, base, extension


def parse_five_bytes(byte_list):
    values = [0 for i in range(8)]
    
    values[0] = (byte_list[0] & 0xf8) >> 3
    values[1] = (byte_list[0] & 0x07) << 2 | (byte_list[1] & 0xc0) >> 6
    values[2] = (byte_list[1] & 0x3e) >> 1
    values[3] = (byte_list[1] & 0x01) << 4 | (byte_list[2] & 0xf0) >> 4
    values[4] = (byte_list[2] & 0x0f) << 1 | (byte_list[3] & 0x80) >> 7
    values[5] = (byte_list[3] & 0x7c) >> 2
    values[6] = (byte_list[3] & 0x03) << 3 | (byte_list[4] & 0xe0) >> 5
    values[7] = byte_list[4] & 0x1f
    
    return "".join(map(lambda x: __letters[x], values))


def parse_four_bytes(byte_list):
    values = [0 for i in range(7)]
    
    values[0] = (byte_list[0] & 0xc0) >> 6
    values[1] = (byte_list[0] & 0x3e) >> 1
    values[2] = (byte_list[0] & 0x01) << 4 | (byte_list[1] & 0xf0) >> 4
    values[3] = (byte_list[1] & 0x0f) << 1 | (byte_list[2] & 0x80) >> 7
    values[4] = (byte_list[2] & 0x7c) >> 2
    values[5] = (byte_list[2] & 0x03) << 3 | (byte_list[3] & 0xe0) >> 5
    values[6] = byte_list[3] & 0x1f
    
    return "".join(map(lambda x: __letters[x], values))


def parse_three_bytes(byte_list):
    values = [0 for i in range(5)]
    
    values[0] = (byte_list[0] & 0xf0) >> 4
    values[1] = (byte_list[0] & 0x0f) << 1 | (byte_list[1] & 0x80) >> 7
    values[2] = (byte_list[1] & 0x7c) >> 2
    values[3] = (byte_list[1] & 0x03) << 3 | (byte_list[2] & 0xe0) >> 5
    values[4] = byte_list[2] & 0x1f
    
    return "".join(map(lambda x: __letters[x], values))


def parse_two_bytes(byte_list):
    values = [0 for i in range(4)]
    
    values[0] = (byte_list[0] & 0x80) >> 7
    values[1] = (byte_list[0] & 0x7c) >> 2
    values[2] = (byte_list[0] & 0x03) << 3 | (byte_list[1] & 0xe0) >> 5
    values[3] = byte_list[1] & 0x1f
    
    return "".join(map(lambda x: __letters[x], values))


def parse_one_byte(byte_list):
    values = [0 for i in range(2)]
    
    values[0] = (byte_list[0] & 0xe0) >> 5
    values[1] = byte_list[0] & 0x1f
    
    return "".join(map(lambda x: __letters[x], values))


def parse_bytes(byte_list):
    match len(byte_list):
        case 1:
            return parse_one_byte(byte_list)
        case 2:
            return parse_two_bytes(byte_list)
        case 3:
            return parse_three_bytes(byte_list)
        case 4:
            return parse_four_bytes(byte_list)
        case 5:
            return parse_five_bytes(byte_list)
        case _:
            raise ValueError(f"Byte list needs to have between 1 and 5 entries, inclusive. Got {len(byte_list)} instead.")

        
def hash_func(bytes):
    out = __fnv_offset
    
    for byte in bytes:
        out *= __fnv_prime
        out ^= byte
        out &= 0xffffffffffffffff
    return out.to_bytes(length=8)


def convert_name(name):
    digest = b""
    
    if isinstance(name, str):
        digest = hash_func(bytes(name, "utf8"))
    else:
        digest = hash_func(bytes(name))
        
    i = 0
    mod = len(digest) % 5
        
    out = ""
    while i < len(digest):
        if i == 0 and mod != 0:
            byte_list = digest[:mod]
            out += parse_bytes(byte_list)
            i += mod
        else:
            byte_list = digest[i:i + 5]
            out += parse_bytes(byte_list)
            i += 5
    if len(out) >= len(name):
        return name
    return out

    
def convert_path(name):
    dir, base, extension = extract_parts(name)
    out = ""
    if len(dir) != 0:
        out += convert_name(dir)
        out += os.path.sep
    
    out += convert_name(base)
    
    if len(extension) != 0:
        out += extension
        
    return out

    
def main():
    parser = argparse.ArgumentParser()
    
    parser.add_argument("filepath", help="The filename to convert, which may include a path component. If a path is included, the path will be converted separately.")
    parser.add_argument("--no-separate", help="Don't separate the file components.", action="store_true")
    
    args = parser.parse_args()
    
    if args.no_separate :
        print(convert_name(args.filepath))
    else :
        print(convert_path(args.filepath))

    
if __name__ == "__main__":
    main()
        
