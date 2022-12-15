import operator
import re
import struct
import sys

valr = re.compile(valr_s := r"\w+").match
valn = re.compile(valn_s := r"\$\w+").match
vali = re.compile(vali_s := "[-+]?\\d+|'(?:[\\S ]|\\\\(?:[0-7]{1,3}|x[0-9A-Fa-f]{1,2}|[\\\\'\"abfnrtv]))'").match
val = re.compile(val_s := f"({valr_s}|{valn_s}|{vali_s})").match
val_s_nogroup = val_s[1:-1]
load = re.compile(load_s := r"\s*".join([r"\[", val_s, r"(?:([+\-*/%])", val_s, r")?\]"])).match
load_s_nogroup = r"\s*".join([r"\[", val_s_nogroup, r"(?:[+\-*/%]", val_s_nogroup, r")?\]"])
exp = re.compile(exp_s := f"(?:{val_s}|{load_s})").match
exp_s_nogroup = f"(?:{val_s_nogroup}|{load_s_nogroup})"
exp_s_group = f"({val_s_nogroup}|{load_s_nogroup})"
get_inst_arg = re.compile(get_inst_arg_s :=
    r"\s*".join([r"(\w+)\b", f"((?:{exp_s_nogroup}", ',',
 f")*){exp_s_group}?"])
).findall
assign = re.compile(assign_s := r"\s*".join([r"(\w+)", "=", f"({vali_s})"])).findall

OP_TO_FUNC = {'+': operator.add, '-': operator.sub, '*': operator.mul, '/': operator.floordiv}

class VM:
    def __init__(self, stacksize=4096, memsize=8192, fmt='H',
                 dispsize=40, chars=" &", displines=6, spritesize=3,
                 X_init=0):
        *self.chars, = map(ord, chars)
        self.cycles = 0
        self.disp = bytearray(displines * dispsize).replace(
            b'\0', chars[0].encode('l1')
        )
        self.dispi = 0
        self.dispsize = dispsize
        self.fmt = fmt
        self.fmt_size = fmt_size = struct.calcsize(fmt)
        self.label_jump_cache = []
        self.line = memoryview(self.disp)
        self.mem = self._mem = memoryview(bytearray(total_size :=
            (memsize + stacksize) * fmt_size
        ))
        self.memsize = total_size
        self.memview_cache = {'B': self._mem}
        self.offsets = {}
        self.reg = self._reg = memoryview(bytearray(36))
        self.regview_cache = {'B': self._reg}
        self.spritesize = spritesize
        self.flags = self.reg[32:].cast('I')
        if fmt != 'B':
            self.mem = self._mem.cast(fmt)
            self.memview_cache[fmt] = self.mem
            self.reg = self.reg.cast(fmt)
            self.regview_cache[fmt] = self.reg
        self.offsets['X'] = self.set_new_mem(X_init, 'I')
    def mem_cast(self, fmt):
        if memview := self.memview_cache.get(fmt):
            return memview
        self.memview_cache[fmt] = memview = self._mem.cast(fmt)
        return memview
    def reg_cast(self, fmt):
        if regview := self.regview_cache.get(fmt):
            return regview
        self.regview_cache[fmt] = regview = self._reg.cast(fmt)
        return regview
    def get_reg(self, name):
        if name == 'al': return self.reg_cast('B')[0]
        if name == 'bl': return self.reg_cast('B')[4]
        if name == 'cl': return self.reg_cast('B')[8]
        if name == 'dl': return self.reg_cast('B')[12]
        if name == 'ah': return self.reg_cast('B')[1]
        if name == 'bh': return self.reg_cast('B')[5]
        if name == 'ch': return self.reg_cast('B')[9]
        if name == 'dh': return self.reg_cast('B')[13]
        if name == 'ax': return self.reg_cast('H')[0]
        if name == 'bx': return self.reg_cast('H')[2]
        if name == 'cx': return self.reg_cast('H')[4]
        if name == 'dx': return self.reg_cast('H')[6]
        if name == 'sp': return self.reg_cast('H')[8]
        if name == 'bp': return self.reg_cast('H')[10]
        if name == 'si': return self.reg_cast('H')[12]
        if name == 'ss': return self.reg_cast('H')[13]
        if name == 'di': return self.reg_cast('H')[14]
        if name == 'eax': return self.reg_cast('I')[0]
        if name == 'ebx': return self.reg_cast('I')[1]
        if name == 'ecx': return self.reg_cast('I')[2]
        if name == 'edx': return self.reg_cast('I')[3]
        if name == 'esp': return self.reg_cast('I')[4]
        if name == 'ebp': return self.reg_cast('I')[5]
        if name == 'esi': return self.reg_cast('I')[6]
        if name == 'edi': return self.reg_cast('I')[7]
        raise NameError(f"register not found: {name}")
    def set_reg(self, name, val):
        if name == 'al': self.reg_cast('B')[0] = val; return
        if name == 'bl': self.reg_cast('B')[4] = val; return
        if name == 'cl': self.reg_cast('B')[8] = val; return
        if name == 'dl': self.reg_cast('B')[12] = val; return
        if name == 'ah': self.reg_cast('B')[1] = val; return
        if name == 'bh': self.reg_cast('B')[5] = val; return
        if name == 'ch': self.reg_cast('B')[9] = val; return
        if name == 'dh': self.reg_cast('B')[13] = val; return
        if name == 'ax': self.reg_cast('H')[0] = val; return
        if name == 'bx': self.reg_cast('H')[2] = val; return
        if name == 'cx': self.reg_cast('H')[4] = val; return
        if name == 'dx': self.reg_cast('H')[6] = val; return
        if name == 'sp': self.reg_cast('H')[8] = val; return
        if name == 'bp': self.reg_cast('H')[10] = val; return
        if name == 'si': self.reg_cast('H')[12] = val; return
        if name == 'ss': self.reg_cast('H')[13] = val; return
        if name == 'di': self.reg_cast('H')[14] = val; return
        if name == 'eax': self.reg_cast('I')[0] = val; return
        if name == 'ebx': self.reg_cast('I')[1] = val; return
        if name == 'ecx': self.reg_cast('I')[2] = val; return
        if name == 'edx': self.reg_cast('I')[3] = val; return
        if name == 'esp': self.reg_cast('I')[4] = val; return
        if name == 'ebp': self.reg_cast('I')[5] = val; return
        if name == 'esi': self.reg_cast('I')[6] = val; return
        if name == 'edi': self.reg_cast('I')[7] = val; return
        raise NameError(f"register not found: {name}")
    def get_flag(self, char):
        if char == 'C': return not not self.flags[0] & 1
        if char == 'P': return not not self.flags[0] & 4
        if char == 'A': return not not self.flags[0] & 16
        if char == 'Z': return not not self.flags[0] & 64
        if char == 'S': return not not self.flags[0] & 128
        if char == 'T': return not not self.flags[0] & 256
        if char == 'I': return not not self.flags[0] & 512
        if char == 'D': return not not self.flags[0] & 1024
        if char == 'O': return not not self.flags[0] & 2048
        raise NameError(f"flag not found: {char}")
    def set_flag(self, char):
        if char == 'C': self.flags[0] |= 1
        if char == 'P': self.flags[0] |= 4
        if char == 'A': self.flags[0] |= 16
        if char == 'Z': self.flags[0] |= 64
        if char == 'S': self.flags[0] |= 128
        if char == 'T': self.flags[0] |= 256
        if char == 'I': self.flags[0] |= 512
        if char == 'D': self.flags[0] |= 1024
        if char == 'O': self.flags[0] |= 2048
        raise NameError(f"flag not found: {char}")
    def get_mem(self, offset, fmt=None):
        if fmt is None: fmt = self.fmt
        return self.mem_cast(fmt)[offset]
    def set_mem(self, offset, value, fmt=None):
        if fmt is None: fmt = self.fmt
        self.mem_cast(fmt)[offset] = value
    def set_new_mem(self, value, fmt=None):
        if fmt is None: fmt = self.fmt
        old_offset = self.get_reg('ss')
        new_offset = old_offset + self.fmt_size
        self.set_reg('ss', new_offset)
        self.mem_cast(fmt)[old_offset] = value
        return old_offset
    def get_val(self, string):
        if string[0] == '$':
            val = self.mem[self.offsets[string[1:]]]
        elif string[0].isalpha():
            val = self.get_reg(string)
        else:
            val = eval(string)
            if string[0] == "'":
                val = ord(val)
        return val
    def set_val(self, string, value):
        if string[0] == '$':
            self.mem[self.offsets[string[1:]]] = value
        elif string[0].isalpha():
            self.set_reg(string, value)
        else:
            raise TypeError("literal cannot be set")
    def get_var(self, name, fmt=None):
        if fmt is None: fmt = self.fmt
        return self.get_mem(self.offsets[name], fmt)
    def set_var(self, name, value, fmt=None):
        if fmt is None: fmt = self.fmt
        return self.set_mem(self.offsets[name], value, fmt)
    def get_load(self, _base, _op, _off):
        base = self.get_val(_base)
        if _op:
            op = OP_TO_FUNC(_op)
            off = self.get_val(_off)
            val = self.get_mem(op(base, off))
        else:
            val = self.get_mem(base)
        return val
    def get_load_addr(self, _base, _op, _off):
        base = self.get_val(_base)
        if _op:
            op = OP_TO_FUNC(_op)
            off = self.get_val(_off)
            return op(base, off)
        return base
    def set_load(self, val, _base, _op, _off):
        base = self.get_val(_base)
        if _op:
            op = OP_TO_FUNC(_op)
            off = self.get_val(_off)
            offset = op(base, off)
        else:
            offset = base
        self.set_mem(offset, val)
    def dispchar(self, X=None):
        if X is None: X = self.get_var('X')
        self.line[self.dispi] = self.chars[
            X <= self.dispi < X+self.spritesize
        ]
        self.dispi += 1
        if self.dispi == self.dispsize:
            self.dispi = 0
            self.line = self.line[self.dispsize:]
    def display(self):
        l = []
        line = memoryview(self.disp)
        fill = chr(self.chars[0])
        while line:
            l.append(''.join(map(chr, line[:self.dispsize].tolist()))
                     .replace('\0', fill))
            line = line[self.dispsize:]
        print('\n'.join(l))
    def run(self, prog):
        for A in prog.split('\n'):
            if not (A := A.strip()): continue
            if A[0] == '@':
                name, val_s = [*assign(A[1:].strip())[0]]
                val = eval(val_s)
                if val_s[0] == "'":
                    val = ord(val)
                self.offsets[name] = self.set_new_mem(val)
                continue
            elif A[0] == '.': continue # TODO: labels
            split = [*get_inst_arg(A)[0]]
            split[1:2] = map(str.strip, split[1].split(','))
            X = self.get_var('X')
            self.dispchar(X)
            match split:
                case ["nop", '', '']:
                    self.cycles += 1
                case ["noop", '', '']:
                    self.cycles += 1
                case ["add", lhs, rhs]:
                    self.dispchar(X)
                    _lhs, _rhs = exp(lhs), exp(rhs)
                    if s := _rhs.group(1):
                        val = self.get_val(s)
                    else:
                        val = self.get_load(*_rhs.groups()[1:])
                    try:
                        if s := _lhs.group(1):
                            self.set_val(s, self.get_val(s) + val)
                        else:
                            addr = self.get_load_addr(*_lhs.groups()[1:])
                            self.set_mem(addr, self.get_mem(addr) + val)
                    except ValueError:
                        self.set_flag('O')
                    self.cycles += 2
                case ["addx", '', rhs]:
                    self.dispchar(X)
                    _rhs = exp(rhs)
                    if s := _rhs.group(1):
                        val = self.get_val(s)
                    else:
                        val = self.get_load(*_rhs.groups()[1:])
                    X += val
                    self.set_var('X', X)
                    self.cycles += 2
                case ["sub", lhs, rhs]:
                    self.dispchar(X)
                    _lhs, _rhs = exp(lhs), exp(rhs)
                    if s := _rhs.group(1):
                        val = self.get_val(s)
                    else:
                        val = self.get_load(*_rhs.groups()[1:])
                    try:
                        if s := _lhs.group(1):
                            self.set_val(s, self.get_val(s) - val)
                        else:
                            addr = self.get_load_addr(*_lhs.groups()[1:])
                            self.set_mem(addr, self.get_mem(addr) - val)
                    except ValueError:
                        self.set_flag('O')
                    self.cycles += 2
                case ["subx", '', rhs]:
                    self.dispchar(X)
                    _rhs = exp(rhs)
                    if s := _rhs.group(1):
                        val = self.get_val(s)
                    else:
                        val = self.get_load(*_rhs.groups()[1:])
                    X -= val
                    self.set_var('X', X)
                    self.cycles += 2
                case ["mul", lhs, rhs]:
                    self.dispchar(X)
                    self.dispchar(X)
                    _lhs, _rhs = exp(lhs), exp(rhs)
                    if s := _rhs.group(1):
                        val = self.get_val(s)
                    else:
                        val = self.get_load(*_rhs.groups()[1:])
                    try:
                        if s := _lhs.group(1):
                            self.set_val(s, self.get_val(s) * val)
                        else:
                            addr = self.get_load_addr(*_lhs.groups()[1:])
                            self.set_mem(addr, self.get_mem(addr) * val)
                    except ValueError:
                        self.set_flag('O')
                    self.cycles += 3
                case ["mulx", '', rhs]:
                    self.dispchar(X)
                    self.dispchar(X)
                    _rhs = exp(rhs)
                    if s := _rhs.group(1):
                        val = self.get_val(s)
                    else:
                        val = self.get_load(*_rhs.groups()[1:])
                    X *= val
                    self.set_var('X', X)
                    self.cycles += 3
                case ["div", lhs, rhs]:
                    self.dispchar(X)
                    self.dispchar(X)
                    self.dispchar(X)
                    self.dispchar(X)
                    _lhs, _rhs = exp(lhs), exp(rhs)
                    if s := _rhs.group(1):
                        val = self.get_val(s)
                    else:
                        val = self.get_load(*_rhs.groups()[1:])
                    try:
                        if s := _lhs.group(1):
                            self.set_val(s, self.get_val(s) * val)
                        else:
                            addr = self.get_load_addr(*_lhs.groups()[1:])
                            self.set_mem(addr, self.get_mem(addr) * val)
                    except ValueError:
                        self.set_flag('O')
                    self.cycles += 5
                case ["divx", '', rhs]:
                    self.dispchar(X)
                    self.dispchar(X)
                    self.dispchar(X)
                    self.dispchar(X)
                    _rhs = exp(rhs)
                    if s := _rhs.group(1):
                        val = self.get_val(s)
                    else:
                        val = self.get_load(*_rhs.groups()[1:])
                    X *= val
                    self.set_var('X', X)
                    self.cycles += 5
                case ["mov", lhs, rhs]:
                    _lhs, _rhs = exp(lhs), exp(rhs)
                    if s := _rhs.group(1):
                        val = self.get_val(s)
                    else:
                        val = self.get_load(*_rhs.groups()[1:])
                    if s := _lhs.group(1):
                        self.set_val(s, val)
                    else:
                        self.set_load(val, *_lhs.groups()[1:])
                    self.cycles += 1
                case _:
                    raise TypeError(f"invalid opcode and/or opargs: {split}")

if __name__ == '__main__':
    cols, syms, lines = 40, " #", 12
    try:
        if more := sys.argv[1:]: cols = int(more[0])
        if more[1:]: syms = f"{more[1]:>2}"
        if more[2:]: lines = int(more[2])*6
    except ValueError:
        print("Usage: python3 vm.py <length> <symbols> <numlines>")
        exit(1)
    vm = VM(fmt='h', dispsize=cols, displines=lines, chars=syms)
    vm.run(open(0).read())
    vm.display()
