s = '⁰¹²³⁴⁵⁶⁷⁸⁹'
el_norm = ["hydrogen", "helium", "lithium", "beryllium", "boron", "carbon", "nitrogen", "oxygen", "fluorine", "neon", "sodium", "magnesium", "aluminium", "silicon", "phosphorus", "sulfur", "chlorine", "argon", "potassium", "calcium", "scandium", "titanium", "vanadium", "chromium", "manganese", "iron", "cobalt", "nickel", "copper", "zinc", "gallium", "germanium", "arsenic", "selenium", "bromine", "krypton", "rubidium", "strontium", "yttrium", "zirconium", "niobium", "molybdenum", "technetium", "ruthenium", "rhodium", "palladium", "silver", "cadmium", "indium", "tin", "antimony", "tellurium", "iodine", "xenon", "cesium", "barium", "lanthanum", "cerium", "praseodymium", "neodymium", "promethium", "samarium", "europium", "gadolinium", "terbium", "dysprosium", "holmium", "erbium", "thulium", "ytterbium", "lutetium", "hafnium", "tantalum", "tungsten", "rhenium", "osmium", "iridium", "platinum", "gold", "mercury", "thallium", "lead", "bismuth", "polonium", "astatine", "radon", "francium", "radium", "actinium", "thorium", "protactinium", "uranium", "neptunium", "plutonium", "americium", "curium", "berkelium", "californium", "einsteinium", "fermium", "mendelevium", "nobelium", "lawrencium", "rutherfordium", "dubnium", "seaborgium", "bohrium", "hassium", "meitnerium", "darmstadtium", "roentgenium", "copernicium", "nihonium", "flerovium", "moscovium", "livermorium", "tennessine", "oganesson"]
el_abbr = ['h', 'he', 'li', 'be', 'b', 'c', 'n', 'o', 'f', 'ne', 'na', 'mg', 'al', 'si', 'p', 's', 'cl', 'ar', 'k', 'ca', 'sc', 'ti', 'v', 'cr', 'mn', 'fe', 'co', 'ni', 'cu', 'zn', 'ga', 'ge', 'as', 'se', 'br', 'kr', 'rb', 'sr', 'y', 'zr', 'nb', 'mo', 'tc', 'ru', 'rh', 'pd', 'ag', 'cd', 'in', 'sn', 'sb', 'te', 'i', 'xe', 'cs', 'ba', 'la', 'ce', 'pr', 'nd', 'pm', 'sm', 'eu', 'gd', 'tb', 'dy', 'ho', 'er', 'tm', 'yb', 'lu', 'hf', 'ta', 'w', 're', 'os', 'ir', 'pt', 'au', 'hg', 'tl', 'pb', 'bi', 'po', 'at', 'rn', 'fr', 'ra', 'ac', 'th', 'pa', 'u', 'np', 'pu', 'am', 'cm', 'bk', 'cf', 'es', 'fm', 'md', 'no', 'lr', 'rf', 'db', 'sg', 'bh', 'hs', 'mt', 'ds', 'rg', 'cn', 'nh', 'fl', 'mc', 'lv', 'ts', 'og']
N_ELEMENTS = len(el_norm)
orbitals = {'s': 2, 'p': 6, 'd': 10, 'f': 14}
N_ORBITALS = len(orbitals)
_orbitals_items = [*orbitals.items()][::-1]
_suborbitals = [_orbitals_items[-x:] for x in range(1, N_ORBITALS)]
def gen_config(atom_num):
    t = []
    glob_row = 1
    for suborbital in _suborbitals:
        row_num = glob_row
        for name, max_num in suborbital:
            if atom_num >= max_num:
                t.append((row_num, name, max_num))
                row_num += 1
                atom_num -= max_num
            elif atom_num:
                t.append((row_num, name, atom_num))
                break
            else:
                break
        else:
            glob_row += 1
            row_num = glob_row
            for name, max_num in suborbital:
                if atom_num >= max_num:
                    t.append((row_num, name, max_num))
                    row_num += 1
                    atom_num -= max_num
                elif atom_num:
                    t.append((row_num, name, atom_num))
                    break
                else:
                    break
            else:
                continue
            break
        break
    else:
        while True:
            row_num = glob_row
            for name, max_num in _orbitals_items:
                if atom_num >= max_num:
                    t.append((row_num, name, max_num))
                    row_num += 1
                    atom_num -= max_num
                elif atom_num:
                    t.append((row_num, name, atom_num))
                    break
                else:
                    break
            else:
                glob_row += 1
                continue
            break
    return t

def config_to_print(atom_num):
    return ''.join([f"{row}{name}{''.join([s[int(d)] for d in f'{num}'])}"
                    for row, name, num in gen_config(atom_num)])

def parse_element_string(element):
    is_unabbr = False
    try:
        an = int(element)
    except ValueError:
        E = element.lower()
        if len(E) > 2:
            an = el_norm.index(E) + 1
            is_unabbr = True
        else:
            an = el_abbr.index(E) + 1
    return an, is_unabbr

def main():
    for X in input().split():
        try:
            number, is_unabbr = parse_element_string(X)
        except ValueError:
            print(f"Element {X!r} does not exist")
            continue
        if not is_unabbr and number <= N_ELEMENTS:
            print(f"EC {X} ({el_norm[number - 1]}) = {config_to_print(number)}")
        else:
            print(f"EC {X} = {config_to_print(number)}")

if __name__ == '__main__':
    while True:
        main()
