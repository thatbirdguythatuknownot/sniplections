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
    Generate a punnett square from shared genes (e.g. R/r and S/s from RRssTT x RrSSAA).
    Non-shared genes will just be prepended or appended to the genotype of the results.
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
    # only useful if there are no common keys
    uncommon_keys = []
    i = 0
    len_org_2 = len(org_2)
    while i < len_org_2:
        geno_1 = org_2[i]
        g1l = geno_1.lower()
        if cancel := g1l in dup_traits:
            print(f"warning: ignoring duplicate trait {geno_1!r}")
        elif uncommon := g1l not in traits_lower:
            pass
        else:
            dup_traits.add(g1l)
            sort_keys[g1l] = traits_lower.pop(g1l)
        i += 1
        if i == len_org_2 or g1l != (geno_2 := org_2[i]).lower():
            print("warning: assuming pure homozygous genotype")
            if cancel:
                continue
            tup = (geno_1, geno_1)
        else:
            i += 1
            if cancel:
                continue
            # prioritize uppercase
            if not geno_1.isupper() and geno_2.isupper():
                tup = (geno_2, geno_1)
            else:
                tup = (geno_1, geno_2)
        if uncommon:
            uncommon_keys.append(tup)
            continue
        genotype_2.append(tup)
    if not sort_keys:
        crossed_1 = []
        crossed_2 = []
        pheno_1 = []
        pheno_2 = []
        for a, b in genotype_1:
            # already ordered by the while
            crossed_1.append(a + b)
            pheno_1.append(traits.get(a, a))
        # there should be nothing in genotype_2 so use uncommon_keys
        for a, b in uncommon_keys:
            # already ordered by the while
            crossed_2.append(a + b)
            pheno_2.append(traits.get(a, a))
        if not crossed_1 and not crossed_2:
            # if there are no traits at all to be crossed, return:
            # - still a 2D list as the first value, but effectively empty
            # - 2 empty lists
            # - 2 empty dictionaries
            return [[]], [], [], {}, {}
        gtype_1 = ''.join(crossed_1)
        gtype_2 = ''.join(crossed_2)
        crossed_1 = [gtype_1] if gtype_1 else []
        crossed_2 = [gtype_2] if gtype_2 else []
        genotype = gtype_1 + gtype_2
        ptype_1 = '/'.join(phenotype := pheno_1 + pheno_2)
        # return:
        # - a 2D list with only one element (combined traits)
        # - the first traits to cross as a single element in a list
        # - the second traits to cross as a single element in a list
        # - 2 dictionaries each with only one key-value pair (where value is 1)
        return ([[SquareUnit(genotype, phenotype)]], crossed_1, crossed_2,
                {genotype: 1}, {ptype_1: 1})
    g_prefix = []
    p_prefix = []
    for x in reversed(traits_lower.values()):
        a, b = genotype_1[x]
        # already ordered by the while
        g_prefix.append(a + b)
        p_prefix.append(traits.get(a, a))
    g_prefix = ''.join(g_prefix)
    g_suffix = []
    p_suffix = []
    for a, b in uncommon_keys:
        # already ordered by the while
        g_suffix.append(a + b)
        p_suffix.append(traits.get(a, a))
    g_suffix = ''.join(g_suffix)
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
            pheno = p_prefix[:]
            for a, b in zip(cross_1, cross_2):
                # prioritize uppercase
                if not a.isupper() and b.isupper():
                    a, b = b, a
                pheno.append(traits.get(a, a))
                geno.append(a + b)
            pheno.extend(p_suffix)
            genotype_R[gtype := f"{g_prefix}{''.join(geno)}{g_suffix}"] += 1
            phenotype_R['/'.join(pheno)] += 1
            row.append(SquareUnit(gtype, pheno))
        square.append(row)
    return (square, [f"{g_prefix}{x}" for x in crossed_1],
            [f"{x}{g_suffix}" for x in crossed_2], dict(genotype_R),
            dict(phenotype_R))

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
            header = f"{' ' * max_left_len}   |{header}|\n"
        else:
            header = f"|{header}|\n"
    else:
        col_max_lens = [2+max(map(len, col)) for col in col_strs]
        header = ""
    if col_max_lens:
        sep_1 = f"+{'+'.join('-'*max_len for max_len in col_max_lens)}+\n"
        if max_left_len:
            sep = f"+{'-' * max_left_len}--{sep_1}"
            sep_1 = f"{' ' * max_left_len}   {sep_1}"
        else:
            sep = sep_1
        if not header:
            sep_1 = ""
    else:
        sep = sep_1 = ""
    to_join = [header]
    for i, row in enumerate(square):
        if row:
            row_s = ' | '.join(f'{str(x):<{col_max_lens[i]-2}}'
                               for i, x in enumerate(row))
            row_s = f"| {row_s} |\n"
        if crossed_1:
            if not row:
                row_s = "||\n"
            to_join.append(f"| {crossed_1[i]:>{max_left_len}} {row_s}")
        elif row:
            to_join.append(row_s)
    return sep_1 + sep.join(to_join) + sep[:-1]

def string_gen_punnett(values):
    """
    Return the formatted string for the tuple containing the 5 values
      returned by gen_punnett().
    """
    square, crossed_1, crossed_2, genotype_ratio, phenotype_ratio = values
    res = string_punnett(square, crossed_1, crossed_2)
    if not res:
        res = "++\n++"
    if genotype_ratio:
        gtypes = []
        num_fmts = []
        for gtype, num in genotype_ratio.items():
            gtypes.append(gtype)
            num_fmts.append(f"{num:^{len(gtype)}}")
        res = f"{res}\n\nGenotype Ratio:" \
              f"\n  {':'.join(gtypes)}\n  {':'.join(num_fmts)}"
    else:
        res = f"{res}\n\nNo Genotype Ratio!"
    if phenotype_ratio:
        max_len = max(map(len, phenotype_ratio)) + 2
        lines = '\n'.join(f"{phenotype:>{max_len}} : {num}"
                          for phenotype, num in phenotype_ratio.items())
        res = f"{res}\n\nPhenotype Ratio:\n{lines}"
    else:
        res = f"{res}\n\nNo Phenotype Ratio!"
    return res
