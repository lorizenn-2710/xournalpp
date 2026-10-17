#!/usr/bin/env python3
"""Regenerate src/core/enums/generated/*.NameMap.generated.h (port of generateConvert.php)."""
import glob
import os
import re

os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), "."))

for src in sorted(glob.glob("*.enum.h")):
    name = src[: -len(".enum.h")]
    values = []
    in_enum = False
    text = open(src, encoding="utf-8").read()
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)  # drop block comments
    for raw in text.splitlines():
        line = raw.strip()
        if not in_enum:
            if line.startswith("enum "):
                in_enum = True
            continue
        if line.startswith("};"):
            break
        if not line or line.startswith("//") or line in ("{", "};"):
            continue
        line = line.split("//")[0].strip().rstrip(",")
        if "=" in line:
            line = line.split("=")[0].strip()
        if not line or line == "ENUMERATOR_COUNT":
            continue
        values.append(line)

    out = os.path.join("generated", f"{name}.NameMap.generated.h")
    with open(out, "w", encoding="utf-8") as fp:
        fp.write("// ** THIS FILE IS GENERATED **\n")
        fp.write("// ** use generateConvert.php to update this file **\n\n\n")
        fp.write("#pragma once\n\n")
        fp.write(f"constexpr const char* {name.upper()}_NAMES[] = {{  // {name} to string conversion map\n")
        for i, v in enumerate(values):
            sep = "," if i + 1 < len(values) else "};"
            fp.write(f'        "{v.lower().replace("_", "-")}"{sep}\n')
    print(f"{out}: {len(values)} entries")
