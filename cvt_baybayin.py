import string
import unicodedata as ud

from dataclasses import dataclass
from enum import IntEnum, IntFlag, auto
from typing import Literal, Protocol


@dataclass(slots=True)
class Syll:
    cons: str = ""
    vowl: str | None = None
    final: bool = False

    def __str__(self) -> str:
        if self.vowl is None:
            return self.cons
        return self.cons + self.vowl


class RMode(IntEnum):
    NONE = 0
    TRADITIONAL = NONE
    USE_DA = NONE
    STANDARD = NONE

    OLD = auto()
    ZAMBALES = OLD
    SAMBALES = OLD
    ARCHAIC = OLD

    NEW = auto()
    BIKOL = NEW
    BICOL = NEW
    MODERN = NEW
    COMMON = NEW


class PudpodMode(IntEnum):
    NONE = 0
    TRADITIONAL = NONE
    COMMON = NONE

    VIRAMA = auto()
    KRUS_KUDLIT = VIRAMA
    KRUS = CRUS = VIRAMA
    STANDARD = VIRAMA

    PAMUDPOD = auto()
    UNAMBIGUOUS = PAMUDPOD


class VowelMode(IntFlag):
    """
    VowelMode

    Example: encoding 'ne'
    ([Xy] means baybayin digraph of letter 'Xa' and vowel sign 'y')
        WITH_E | USING_MARK  =>  [Na]e
                WITH_E only  =>  [Ni]e
         none (TRADITIONAL)  =>  [Ni]

    If using 'WITH_E'/'WITH_O' flags, make sure to have the 'USING_MARK'
    and 'WITH_*' flags the same in both encoding/decoding in order to
    avoid unexpected improper conversions.
    """
    TRADITIONAL = 0
    COMMON = TRADITIONAL
    STANDARD = TRADITIONAL

    USING_MARK = 1
    WITH_E = auto()
    WITH_O = auto()
    HEURISTIC = auto()


class VwlHeuristic(Protocol):
    def __call__(self, vowl: str, cons: str | None = None) -> bool:
        """
        Returns True if the alt vowel should be used, False otherwise.

        'vowl' is the vowel; can either be the original or alt vowel.
        'cons' is the consonant before the vowel (optional).
        """
        ...


vowl_enc_map: dict[str, tuple[str, str | None]] = {
    'A': ('\N{TAGALOG LETTER A}', None),
    'I': ('\N{TAGALOG LETTER I}', '\N{TAGALOG VOWEL SIGN I}'),
    'U': ('\N{TAGALOG LETTER U}', '\N{TAGALOG VOWEL SIGN U}'),
}

vowl_dec_map: dict[str, tuple[str, bool]] = {}
for latin, (baybay, suffix) in vowl_enc_map.items():
    vowl_dec_map[baybay] = (latin, True)
    vowl_dec_map[suffix] = (latin, False)


vowl_enc_foreign: dict[str, str] = {
    'E': 'I',
    'O': 'U',
}

vowl_dec_foreign: dict[str, str] = {}
for alt, orig in vowl_enc_foreign.items():
    vowl_dec_foreign[orig] = alt


enc_map: dict[str, str] = {
    ',': '\N{PHILIPPINE SINGLE PUNCTUATION}',
    '.': '\N{PHILIPPINE DOUBLE PUNCTUATION}',
}
for ch in string.ascii_uppercase:
    if ch in "AEIOU":  # don't include vowels
        continue
    if ch == 'R':  # 'R' is special-cased
        continue
    try:
        letter = ud.lookup(f"TAGALOG LETTER {ch}A")
    except KeyError:
        continue
    else:
        enc_map[ch] = letter

LETTER_NA = enc_map['N']
LETTER_NGA = '\N{TAGALOG LETTER NGA}'

RA_ARCHAIC = '\N{TAGALOG LETTER ARCHAIC RA}'
RA_MODERN = '\N{TAGALOG LETTER RA}'

dec_map: dict[str, str] = {}
for latin, baybay in enc_map.items():
    dec_map[baybay] = latin

dec_map[LETTER_NGA] = 'NG'
dec_map[RA_ARCHAIC] = 'R'
dec_map[RA_MODERN] = 'R'


pudpod_syms_set: set[str] = {
    SYM_PAMUDPOD := '\N{TAGALOG SIGN PAMUDPOD}',
    SYM_KRUS := '\N{TAGALOG SIGN VIRAMA}',
}


def encode(
    text: str,
    mode_r: RMode = RMode.STANDARD,
    mode_pudpod: PudpodMode = PudpodMode.STANDARD,
    mode_vowel: VowelMode = VowelMode.STANDARD,
) -> str:
    if mode_r == RMode.NEW:
        RA = RA_MODERN
    elif mode_r == RMode.OLD:
        RA = RA_ARCHAIC
    else:
        assert mode_r == RMode.USE_DA, f"unknown R mode: {mode_r}"
        RA = enc_map['D']

    if mode_pudpod == PudpodMode.PAMUDPOD:
        PUDPOD = SYM_PAMUDPOD
    elif mode_pudpod == PudpodMode.KRUS:
        PUDPOD = SYM_KRUS
    else:
        assert mode_pudpod == PudpodMode.NONE, \
            f"unknown pudpod mode: {mode_pudpod}"
        PUDPOD = vowl_enc_map['A'][1]

    res: list[Syll | str] = []

    for ch in text:
        ch_base = ch.upper()

        # Vowels
        if orig := vowl_enc_foreign.get(ch_base):
            if mode_vowel & getattr(VowelMode, f"WITH_{ch_base}"):
                if (not mode_vowel & VowelMode.USING_MARK
                        and res and isinstance(last := res[-1], Syll)
                        and not last.final):
                    last.vowl = vowl_enc_map[orig][1]
                    last.final = True
                res.append(ch)
                continue
            ch_base = orig

        if vwt := vowl_enc_map.get(ch_base):
            if res and isinstance(last := res[-1], Syll) and not last.final:
                last.vowl = vwt[1]
                last.final = True
            else:
                res.append(Syll(vowl=vwt[0], final=True))
            continue

        # Non-alphabetic characters
        repl = enc_map.get(ch_base, ch)
        if not ch.isalpha():
            res.append(repl)
            continue

        # Consonants
        if (ch_base == 'G'
                and res and isinstance(last := res[-1], Syll)
                and not last.final
                and last.cons == LETTER_NA):
            last.cons = LETTER_NGA
        elif ch_base == 'R':
            res.append(Syll(RA))
        else:
            is_final = repl == ch  # is unconverted consonant
            res.append(Syll(repl, final=is_final))

    for elem in res:
        if isinstance(elem, Syll) and not elem.final:
            elem.vowl = PUDPOD

    return "".join(map(str, res))


def decode(
    text: str,
    mode_vowel: VowelMode = VowelMode.STANDARD,
    alt_vwl_hstic: VwlHeuristic | None = None,
) -> str:
    has_heuristic = (
        (mode_vowel & VowelMode.HEURISTIC) != 0
        and alt_vwl_hstic is not None
    )

    res: list[Syll | str] = []
    for ch in text:
        # Vowels
        if vwt := vowl_dec_map.get(ch):
            is_suffix = False
            cons = None

            if alt := vowl_dec_foreign.get(to_use := vwt[0]):
                use_heuristic = (
                    has_heuristic
                    and (mode_vowel & getattr(VowelMode, f"WITH_{alt}")) != 0
                )
            else:
                use_heuristic = False

            if (not vwt[1]  # vowel is a suffix
                    and res and isinstance(last := res[-1], Syll)
                    and not last.final):
                is_suffix = True
                if use_heuristic:
                    cons = last.cons

            if use_heuristic and alt_vwl_hstic(alt, cons):
                to_use = alt

            to_use = to_use.lower()
            if is_suffix:
                last.vowl = to_use
                last.final = True
            else:
                res.append(Syll(vowl=to_use, final=True))
            continue
        elif (not has_heuristic
                and (orig := vowl_enc_foreign.get(ch_base := ch.upper()))
                and mode_vowel & getattr(VowelMode, f"WITH_{ch_base}")):
            if res and isinstance(last := res[-1], Syll):
                if (
                    not last.final
                    if mode_vowel & VowelMode.USING_MARK
                    else last.vowl == orig.lower()
                ):
                    last.vowl = ch
                    last.final = True
                    continue
            res.append(ch)
            continue

        if (ch in pudpod_syms_set
                and res and isinstance(last := res[-1], Syll)
                and not last.final):
            last.vowl = None
            last.final = True
            continue

        # Non-alphabetic characters
        repl = dec_map.get(ch, ch)
        if not repl.isalpha():
            res.append(repl)
            continue

        # Consonants
        is_final = repl == ch  # is unconverted consonant
        if not is_final:
            repl = repl.lower()
        res.append(Syll(repl, final=is_final))

    for elem in res:
        if isinstance(elem, Syll) and not elem.final:
            elem.vowl = 'a'

    return "".join(map(str, res))


if __name__ == "__main__":
    mvwl = VowelMode.WITH_O | VowelMode.WITH_E
    print(encode("nangangalakal ng kalakal sa kalakalan", mode_r=RMode.COMMON, mode_vowel=mvwl))