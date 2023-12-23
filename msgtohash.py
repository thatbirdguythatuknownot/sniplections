from hashlib import sha512
from struct import unpack

def to_ints(string):
    return unpack(">8Q", sha512(string.encode()).digest())

C_FORMAT = """\
#define HASH_ARR_LEN {}

static uint64_t MSG_HASH[HASH_ARR_LEN] = {{{}}};

#define MSG_LENGTH {}
"""

def to_C(string):
    ints = to_ints(string)
    ints_str = "\n"
    for int_ in ints:
        ints_str += f"    {int_:#018x},\n"
    return C_FORMAT.format(len(ints), ints_str, len(string))

if __name__ == '__main__':
    while True:
        try:
            string = input("string> ")
        except (EOFError, KeyboardInterrupt):
            print("Quitting...")
        else:
            print(to_C(string))
