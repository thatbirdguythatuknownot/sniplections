from itertools import count, takewhile
from warnings import warning

NoneType = type(None)

d_ops = ['+', '-', '*', '/', '//', '%', '**']
negindexes = {1, 3, 4, 5}
class _sequence_metaclass(type):
    def __new__(cls, clsn, b, a):
        return type.__new__(cls, clsn, b, a)
    def __getitem__(self, x):
        if isinstance(x, tuple):
            if isinstance(thing := x[-1], slice):
                args = x[:-1]
                if thing.start is ...:
                    args += (thing.start,)
                    if thing.step is None:
                        if thing.stop is None:
                            return Sequence(*args)
                        else:
                            return Sequence(*args, step=thing.stop)
                    else:
                        if thing.stop is None:
                            return Sequence(*args, op=thing.step)
                        else:
                            return Sequence(*args, step=thing.stop, op=thing.step)
                else:
                    if thing.start is not None:
                        args += (thing.start,)
                    if thing.step is None:
                        if thing.stop is None:
                            return LimitedSequence(*args)
                        else:
                            return LimitedSequence(*args, step=thing.stop)
                    else:
                        if thing.stop is None:
                            return LimitedSequence(*args, op=thing.step)
                        else:
                            return LimitedSequence(*args, step=thing.stop, op=thing.step)
            elif all([not isinstance(y, slice) for y in x]):
                pass
            else:
                return Sequence(*x) if x[-1] is ... else LimitedSequence(*x)
        elif isinstance(x, slice):
            if (isinstance(x.start, (int, NoneType))
                    and isinstance(x.stop, (int, NoneType))
                    and isinstance(x.step, (int, NoneType))):
                return range(x.start or 0, x.stop, x.step or 1)
            else:
                return LimitedSequence(x.start, x.stop, x.step)
        else:
            return (0,)

class Sequence(metaclass=_sequence_metaclass):
    def __init__(self, *args, step=1, op=0):
        op_str = d_ops[op%7]
        assert_msg = f"Sequence numbers must be 'previous number {op_str} {step}'".replace('+ -', '- ').replace('- -', '+ ')
        eval_str = f"x {op_str} {step}"
        if args[0] is ...:
            try:
                raise Warning
            except Warning:
                print(*__import__('traceback').format_stack(), end='')
                print("NoGenerateWarning: '...' before a sequence of numbers is not evaluated")
        adjust = 1
        for i, x in enumerate(args):
            if i and i < len(args)-1:
                assert x is not ..., "'...' must be at start/end of argument list"
                assert eval(eval_str) == (thing := args[i+adjust]) or thing is ..., assert_msg
            elif not i:
                if x is ...:
                    args = args[1:]
                    adjust = 0
                else: assert eval(eval_str) == (thing := args[i+adjust]) or thing is ..., assert_msg
            else:
                if x == ...: args = args[:-1]
        jstr = ', '.join(map(str, args))
        self.reproduce = f"Sequence({jstr}, step={step}, op={op})"
        if step > 0 if op in negindexes else step < 0:
            self.represent = f"Sequence ..., {jstr}; Step {op_str}{step}".replace('+-', '-')
        else:
            self.represent = f"Sequence {jstr}, ...; Step {op_str}{step}".replace('--', '+')
        self.numbers = list(args)
        self.numappend = self.numbers.append
        self.initial = args[-1]
        self.thing = takewhile(lambda x: x[0] != len(args)-1, enumerate(args))
        self.op = f"self.initial {op_str}= {step}\nself.numappend(self.initial)"
    def __repr__(self):
        return self.reproduce
    def __str__(self):
        return self.represent
    def __iter__(self):
        return self
    def __next__(self):
        try:
            return next(self.thing)
        except StopIteration:
            del self.thing
            exec(self.op)
            return self.initial
        except AttributeError:
            exec(self.op)
            return self.initial
    def __getitem__(self, i):
        try:
            return self.numbers[i]
        except IndexError:
            raise IndexError(f"Index {i} of {repr(self)} is not yet generated") from None

class LimitedSequence:
    def __init__(self, *args, step=1, op=0):
        op_str = d_ops[op%7]
        assert_msg = f"Sequence numbers must be 'previous number {op_str} {step}'".replace('+ -', '- ').replace('- -', '+ ')
        eval_str = f"x {op_str} {step}"
        for i, x in enumerate(args):
            if i < len(args)-1:
                assert eval(eval_str) == args[i+1], assert_msg
        jstr = ', '.join(map(str, args))
        self.reproduce = f"LimitedSequence({jstr}, step={step}, op={op})"
        if step > 0 if op in negindexes else step < 0:
            self.represent = f"Sequence {jstr}; Step {op_str}{step}".replace('+-', '-')
        else:
            self.represent = f"Sequence {jstr}; Step {op_str}{step}".replace('--', '+')
        self.numbers = list(args)
        self.index = 0
    def __repr__(self):
        return self.reproduce
    def __str__(self):
        return self.represent
    def __iter__(self):
        return self
    def __next__(self):
        try:
            thing = self.numbers[self.index]
            self.index += 1
            return thing
        except IndexError:
            raise StopIteration from None
    def __getitem__(self, i):
        return self.numbers[i]
