from collections import defaultdict
from itertools import product

class SquareUnit:
    __slots__ = ('genotype', 'phenotype')
    def __init__(self, genotype, phenotype):
        self.genotype = genotype
        self.phenotype = phenotype
    def __str__(self):
       return f"{self.genotype} ({'/'.join(self.phenotype)})"
    def __repr__(self):
        return f"SquareUnit(genotype={self.genotype!r}, " \
                          f"phenotype={self.phenotype!r})"

def gen_punnett(org_1, org_2, traits=None):
    """
    Generate a punnett square from shared genotypes (e.g. R/r and S/s from RRssTT x RrSSAA).
    Takes in 2 (required) strings of the combined genotypes and an optional argument for
      labeling the genotypes (dictionary with key-value pair as `allele`: `label`).
    Return a punnett square as a 2D list, crossed traits as 2 lists,
      genotype ratio as a dict, and phenotype ratio as a dict.
    """
    assert type(org_1) is type(org_2) is str, \
        "one of the 2 required arguments for gen_punnett() is not a str"
    if traits is None:
        traits = {}
    else:
        assert type(traits) is dict, \
            "'traits' argument for gen_punnett() is not a dict"
    genotype_1 = []
    genotype_2 = []
    traits_lower = {}
    i = j = 0
    len_org_1 = len(org_1)
    while i < len_org_1:
        geno_1 = org_1[i]
        g1l = geno_1.lower()
        if has_dup := g1l in traits_lower:
            print(f"ignoring duplicate trait {geno_1!r}")
        else:
            traits_lower[g1l] = j
            j += 1
        i += 1
        if i == len_org_1 or g1l != (geno_2 := org_1[i]).lower():
            print("warning: assuming pure homozygous genotype")
            if has_dup:
                continue
            genotype_1.append((geno_1, geno_1))
        else:
            i += 1
            if has_dup:
                continue
            if not geno_1.isupper() and geno_2.isupper():
                tup = (geno_2, geno_1)
            else:
                tup = (geno_1, geno_2)
            genotype_1.append(tup)
    dup_traits = set()
    sort_keys = {}
    i = 0
    len_org_2 = len(org_2)
    while i < len_org_2:
        geno_1 = org_2[i]
        g1l = geno_1.lower()
        if cancel := g1l in dup_traits:
            print(f"warning: ignoring duplicate trait {geno_1!r}")
        elif cancel := g1l not in traits_lower:
            pass
        else:
            dup_traits.add(g1l)
            sort_keys[g1l] = traits_lower.pop(g1l)
        i += 1
        if i == len_org_2 or g1l != (geno_2 := org_2[i]).lower():
            print("warning: assuming pure homozygous genotype")
            if cancel:
                continue
            genotype_2.append((geno_1, geno_1))
        else:
            i += 1
            if cancel:
                continue
            if not geno_1.isupper() and geno_2.isupper():
                tup = (geno_2, geno_1)
            else:
                tup = (geno_1, geno_2)
            genotype_2.append(tup)
    for x in reversed(traits_lower.values()):
        del genotype_1[x]
    genotype_2.sort(key=lambda x: sort_keys[x[0].lower()])
    crossed_1 = [*map(''.join, product(*genotype_1))]
    crossed_2 = [*map(''.join, product(*genotype_2))]
    genotype_R = defaultdict(int)
    phenotype_R = defaultdict(int)
    square = []
    for cross_1 in crossed_1:
        row = []
        for cross_2 in crossed_2:
            geno = []
            pheno = []
            for a, b in zip(cross_1, cross_2):
                if not a.isupper() and b.isupper():
                    a, b = b, a
                pheno.append(traits.get(a, a))
                geno.append(a + b)
            genotype_R[gtype := ''.join(geno)] += 1
            phenotype_R['/'.join(pheno)] += 1
            row.append(SquareUnit(gtype, pheno))
        square.append(row)
    return square, crossed_1, crossed_2, dict(genotype_R), dict(phenotype_R)

def string_punnett(square, crossed_1=None, crossed_2=None):
    """
    Return the formatted string for the square returned by gen_punnett().
    Optionally put crossed genotype labels as additional arguments:
        crossed_1 (first) - left label
        crossed_2 (second) - top label
    """
    if crossed_1:
        max_left_len = max(map(len, crossed_1))
    else:
        max_left_len = 0
    col_strs = [[*map(str, col)] for col in zip(*square)]
    if crossed_2:
        col_max_lens = [2+max(max(map(len, col)), len(gtype))
                        for col, gtype in zip(col_strs, crossed_2)]
        header = '|'.join(
            f"{gtype:^{max_len}}"
            for gtype, max_len in zip(crossed_2, col_max_lens)
        )
        if max_left_len:
            header = f"{' ' * max_left_len}   |{header}|"
        else:
            header = f"|{header}|"
    else:
        col_max_lens = [max(map(len, col)) for col in col_strs]
        header = ""
    if col_max_lens:
        sep_1 = f"+{'+'.join('-'*max_len for max_len in col_max_lens)}+\n"
        if max_left_len:
            sep = f"\n+{'-' * max_left_len}--{sep_1}"
            sep_1 = f"{' ' * max_left_len}   {sep_1}"
        else:
            sep = f"\n{sep_1}"
    else:
        sep = sep_1 = ""
    to_join = [header]
    for i, row in enumerate(square):
        row_s = '|'.join(f'{str(x):^{col_max_lens[i]!s}}'
                         for i, x in enumerate(row))
        row_s = f"|{row_s}|"
        if crossed_1:
            to_join.append(f"| {crossed_1[i]:>{max_left_len}} {row_s}")
    return sep_1 + sep.join(to_join) + sep[:-1]
