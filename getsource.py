import dis
import inspect
import linecache
from itertools import islice
from textwrap import dedent
from types import CodeType, FunctionType


def _find_code(
    ref: CodeType,
    co: CodeType,
    excl: set[CodeType] | None = None,
) -> tuple[CodeType, int] | None:
    if excl and ref in excl:
        return None
    search = []
    for i, item in enumerate(ref.co_consts):
        if not isinstance(item, CodeType):
            continue
        if item is co:
            return ref, i
        search.append(item)
    for item in search:
        if found := _find_code(item, co, excl):
            return found

def get_parent_code(co: CodeType) -> tuple[CodeType, int] | None:
    frame = inspect.currentframe()
    excl: set[CodeType] = set()
    while frame := frame.f_back:
        root = frame.f_code
        if res := _find_code(root, co, excl):
            return res
        excl.add(root)
    return None


LOAD_CONST = dis.opmap['LOAD_CONST']

def get_pos_by_src(
    src: str,
    co: CodeType,
) -> dis.Positions | None:
    sln: int = co.co_firstlineno
    copos = list(co.co_positions())

    mod: CodeType = compile(src, "", "exec")
    for instr in dis.get_instructions(mod):
        if instr.opcode != LOAD_CONST:
            continue
        if instr.positions.lineno != sln:
            continue

        const = mod.co_consts[instr.arg]
        if (
            isinstance(const, CodeType)
            and copos == list(const.co_positions())
        ):
            return instr.positions
    return None
        

def _get_lines(co: CodeType) -> list[str]:
    try:
        if res := linecache._getlines_from_code(co):
            return res
    except AttributeError:
        pass
    return linecache.getlines(co.co_filename)

def get_pos_and_lines(
    fcode: CodeType,
) -> tuple[list[str], dis.Positions] | None:
    lines = _get_lines(fcode)
    if lines is None:
        return None

    res = get_parent_code(fcode)
    if res is not None:
        src, idx = res
        assert isinstance(src.co_consts[idx], CodeType), \
            f"non-code at constants tuple index {idx}"

        for instr in dis.get_instructions(src):
            if instr.opcode == LOAD_CONST and instr.arg == idx:
                return lines, instr.positions
    else:
        if pos := get_pos_by_src("".join(lines), fcode):
            return lines, pos

    return None


def get_source(
    func: FunctionType,
    keep_indents: bool = False,
) -> str | None:
    res = get_pos_and_lines(func.__code__)
    if res is None:
        return None

    lines, pos = res
    sln = pos.lineno
    scol = pos.col_offset
    eln = pos.end_lineno
    ecol = pos.end_col_offset

    src = lines[sln - 1]

    if sln == eln:
        if keep_indents and src[:scol].isspace():
            return src[:ecol]
        return src[scol:ecol]

    last = lines[eln - 1][:ecol]
    tail = "".join(lines[sln : eln - 1]) + last
    if src[:scol].isspace():
        src += tail
        if not keep_indents:
            src = dedent(src)
    else:
        src = src[scol:]
        if not keep_indents and not src.startswith("def"):
            tail = dedent(tail)
        src += tail
    return src
