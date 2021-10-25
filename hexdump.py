import sys
import os
import re
import unicodedata

print_noline = sys.stderr.write
offset = 16
offset_format = "%08x"
format = "%02x"
length_ordinal = 2
valid_formats = {"x", "X", "d"}
option_name_to_option = {'format': '-f', 'offset': '-o'}
options = sys.argv[1:]
if not options:
    print("Cannot execute hexdump with no inputs")
    sys.exit(1)

options_set = {*options}
valid_formats = {"X": ("%02X", "%08X", 2), "x": ("%02x", "%08x", 2), "d": ("%03d", "%10d", 3), "o": ("%03o", "%-11o", 3)}
need_input = []


if '-f' in options_set:
    i = options.index('-f')
    options.remove('-f')
    if options[i].isalpha():
        key = options[i]
        if key in valid_formats:
            format, offset_format, length_ordinal = valid_formats[key]
            options.remove(key)
        else:
            if key == 'b':
                print("Option 'b' (binary) for formatting is not yet supported")
            else:
                print("Option input '{key}' for option '-f' is not a valid input")
            sys.exit(1)
    else:
        need_input.append((i, 'format'))

if '-o' in options_set:
    i = options.index('-o')
    options.remove('-o')
    if options[i].isascii() and options[i].isdigit():
        key = options[i]
        options.remove(key)
        offset = int(key)
    else:
        need_input.append((i, 'offset'))

if '-h' in options_set:
    options.remove('-h')
    print("""
HEXDUMP PROGRAM
-h, -? = Print this help message
-f = Choose format:
    o = octal format for numbers (left-aligned for offsets)
    d = decimal format for numbers (right-aligned for offsets)
    X = uppercase hex format for numbers (right-aligned, zero-filled for offsets)
    x = lowercase hex format for numbers (right-aligned, zero-filled for offsets) [DEFAULT]
-o = how many characters in a dump line/offset number [DEFAULT 16]
""")

need_input.sort(key=lambda x: x[0])
for _, x in need_input:
    inp = input(f"Enter input for option {option_name_to_option[x]!r}: ")
    if not inp:
        continue
    if x == 'format':
        if inp.isalpha():
            if inp in valid_formats:
                format, offset_format, length_ordinal = valid_formats[inp]
            else:
                if inp == 'b':
                    print("Option 'b' (binary) for formatting is not yet supported")
                else:
                    print("Option input {inp!r} for option '-f' is not a valid input")
                sys.exit(1)
        else:
            print("Invalid input for option '-f': {inp!r}")
            sys.exit(1)
    elif x == 'offset':
        if inp.isascii() and inp.isdigit():
            offset = int(inp)
        else:
            print("Invalid input for option '-o': {inp!r}")
            sys.exit(1)
    else:
        sys.exit(-1073741819)

path = f"{os.getcwd()}\\{options[0]}"
try:
    open(path)
except FileNotFoundError:
    path = options[0]
except OSError:
    path = options[0]

with open(path, "r") as file:
    text = file.read()
    length = len(text)
    position = 0
    print_noline(offset_format % position)
    while position < length:
        contents = text[:offset]
        text = text[offset:]
        hex = ' '.join(format % ord(x) for x in contents).ljust(offset-1+offset*length_ordinal)
        filtered_contents = ''
        for character in contents:
            try:
                unicodedata.name(character)
            except ValueError:
                filtered_contents += '.'
            else:
                filtered_contents += character
        print_noline(f":  {hex}  |{filtered_contents}|\n")
        position += len(contents)
        print_noline(offset_format % position)
