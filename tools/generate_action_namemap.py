#!/usr/bin/env python3
"""Regenerate Action.NameMap.generated.h from Action.enum.h.

Replacement for src/core/enums/generateConvert.php, which cannot run here:
this machine has no PHP. It follows the exact same rules:

  * read every line of Action.enum.h;
  * start collecting at the first line beginning with "enum ";
  * stop at "};"
  * skip blank lines and lines beginning with "//";
  * skip any line containing ENUMERATOR_COUNT;
  * strip anything from "=" or "," onwards;
  * trim;
  * emit each value with "_" -> "-" and lowercased, in order.

Usage:  python3 tools/generate_action_namemap.py [source_dir] [output_path]
Defaults: source_dir = src/core/enums, output = src/core/enums/generated/Action.NameMap.generated.h
"""

import os
import sys

TAB = "    "


def parse_enum(path):
    values = []
    in_enum = False
    in_block_comment = False
    with open(path, encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()

            # Block comments (/** ... */) must be skipped BEFORE anything else: a doc comment
            # inside the enum would otherwise be emitted as if it were an enumerator, shifting
            # every following action name by one. The upstream PHP generator has the same flaw
            # but upstream's enum happens to contain no block comments, so it never showed.
            if in_block_comment:
                if "*/" in line:
                    in_block_comment = False
                    line = line.split("*/", 1)[1].strip()
                    if not line:
                        continue
                else:
                    continue
            while "/*" in line:
                before, rest = line.split("/*", 1)
                if "*/" in rest:
                    line = before + rest.split("*/", 1)[1]
                else:
                    in_block_comment = True
                    line = before
                    break
            line = line.strip()

            if not in_enum:
                if line[:5] == "enum ":
                    in_enum = True
                continue
            if line[:1] == "{":
                continue
            if line[:2] == "};":
                break
            if line == "" or line[:2] == "//":
                continue
            if "ENUMERATOR_COUNT" in line:
                continue
            for sep in ("=", ","):
                idx = line.find(sep)
                if idx != -1:
                    line = line[:idx]
            line = line.strip()
            if line:
                values.append(line)
    return values


def render(name, values):
    out = []
    out.append("// ** THIS FILE IS GENERATED **")
    out.append("// ** use generateConvert.php to update this file **")
    out.append("")
    out.append("")
    out.append("#pragma once")
    out.append("")
    out.append(
        "constexpr const char* ACTION_NAMES[] = {  // %s to string conversion map" % name
    )
    for i, v in enumerate(values):
        # Guard: an enumerator must be a bare identifier. Anything else means the parser let a
        # comment or a stray token through, which silently shifts every later action name.
        if not v.replace("_", "").isalnum() or not v[0].isupper():
            raise SystemExit(
                "refusing to generate: %r is not a valid enumerator name "
                "(check for a comment the parser failed to skip)" % v
            )
        text = v.replace("_", "-").lower()
        suffix = "," if i < len(values) - 1 else ""
        out.append('%s%s"%s"%s' % (TAB, TAB, text, suffix))
    # The upstream PHP generator strips the trailing comma with fseek(-1) and then
    # writes "};" on the same line, so the last entry is followed directly by "};".
    out[-1] += "};"
    out.append("")
    return "\n".join(out)


def main():
    src_dir = sys.argv[1] if len(sys.argv) > 1 else "src/core/enums"
    out_path = (
        sys.argv[2]
        if len(sys.argv) > 2
        else os.path.join(src_dir, "generated", "Action.NameMap.generated.h")
    )

    enum_path = os.path.join(src_dir, "Action.enum.h")
    values = parse_enum(enum_path)
    if not values:
        raise SystemExit("no enumerators parsed from %s" % enum_path)

    text = render("Action", values)

    old = None
    if os.path.exists(out_path):
        with open(out_path, encoding="utf-8") as fh:
            old = fh.read()

    if old == text:
        print("unchanged: %s (%d enumerators)" % (out_path, len(values)))
        return 0

    with open(out_path, "w", encoding="utf-8") as fh:
        fh.write(text)
    print("wrote: %s (%d enumerators)" % (out_path, len(values)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
