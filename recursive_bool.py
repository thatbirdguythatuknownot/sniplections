from collections.abc import Sequence, Sized
from typing import Any

def _rbool(x: Any, *, _S: list[Any] | None = None) -> bool | None:
    """
    _rbool(...) -> bool | None

    True -> satisfies the conditions of truthiness
    False -> a falsy object
    None -> an empty object
    """
    if not isinstance(x, Sequence):
        return bool(x) or (isinstance(x, Sized) and None)

    if _S is None:
        _S = []
    if x in _S:
        return True
    _S.append(x)

    res = True
    for o in x:
        if _rbool(o, _S=_S) is not None:
            break
    else:
        res = None

    _S.pop()
    return res

def rbool(x: Any) -> bool:
    """
    rbool(x: Any) -> bool

    True -> a non-false object
    False -> an object that satisfies the conditions of falsiness:
        1. falsy non-sized object
        2. empty sized object
        3. a sequence full of objects satisfying either 2. or 3.
    """
    return _rbool(x) or False
