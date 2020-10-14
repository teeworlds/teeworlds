import csv
import sys
import re

def check_name(kind, qualifiers, type, name):
    if kind == "variable":
        return check_variable_name(qualifiers, type, name)
    elif kind in "class struct".split():
        if name[0] not in "CI":
            return "should start with 'C' (or 'I' for interfaces)"
        if len(name) < 2:
            return "must be at least two characters long"
        if not name[1].isupper():
            return "must start with an uppercase letter"
    elif kind == "enum_constant":
        if not name.isupper():
            return "must only contain uppercase letters, digits and underscores"
    return None

ALLOW = set("""
    dx dy
    fx fy
    mx my
    ix iy
    px py
    sx sy
    wx wy
    x0 x1
    y0 y1
""".split())
def check_variable_name(qualifiers, type, name):
    if qualifiers == "" and type == "" and name == "argc":
        return None
    if qualifiers == "" and type == "pp" and name == "argv":
        return None
    if qualifiers == "cs":
        # Allow all uppercase names for constant statics.
        if name.isupper():
            return None
        qualifiers = "s"
    # Allow single lowercase letters as member and variable names.
    if qualifiers in ["m", ""] and len(name) == 1 and name.islower():
        return None
    prefix = "".join([qualifiers, "_" if qualifiers else "", type])
    if not name.startswith(prefix):
        return "should start with {!r}".format(prefix)
    if name in ALLOW:
        return None
    name = name[len(prefix):]
    if not name[0].isupper():
        if prefix:
            return "should start with an uppercase letter after the prefix {!r}".format(prefix)
        else:
            return "should start with an uppercase letter"
    return None

def main():
    import argparse
    p = argparse.ArgumentParser(description="Check identifiers (input via stdin in CSV format from extract_identifiers.py) for naming style in Teeworlds code")
    args = p.parse_args()

    identifiers = list(csv.DictReader(sys.stdin))

    unclean = False
    for i in identifiers:
        error = check_name(i["kind"], i["qualifiers"], i["type"], i["name"])
        if error:
            unclean = True
            print("{}:{}:{}: {}: {}".format(i["file"], i["line"], i["column"], i["name"], error))
    return unclean

if __name__ == "__main__":
    sys.exit(main())
