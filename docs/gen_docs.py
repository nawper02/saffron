#!/usr/bin/env python3
"""
gen_docs.py — generate saffron_manual.md from saffron.h

Usage:
    python3 docs/gen_docs.py               # writes docs/saffron_manual.md
    python3 docs/gen_docs.py --stdout      # print to stdout instead
"""

import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths (relative to this script)
# ---------------------------------------------------------------------------
SCRIPT_DIR  = Path(__file__).parent
HEADER      = SCRIPT_DIR.parent / "saffron.h"
OUTPUT      = SCRIPT_DIR / "saffron_manual.md"

# ---------------------------------------------------------------------------
# Pretty names for section tags found in the header
# ---------------------------------------------------------------------------
SECTION_TITLES = {
    "SF_DEFINES":              "Constants & Limits",
    "SF_TYPES":                "Types & Enumerations",
    "SF_CORE_FUNCTIONS":       "Core",
    "SF_MEMORY_FUNCTIONS":     "Memory / Arena",
    "SF_EVENT_FUNCTIONS":      "Events & Input",
    "SF_SCENE_FUNCTIONS":      "Scene",
    "SF_FRAME_FUNCTIONS":      "Frames",
    "SF_DRAWING_FUNCTIONS":    "Drawing",
    "SF_UI_FUNCTIONS":         "UI",
    "SF_MESH_AUTHORING_FUNCTIONS": "Mesh Authoring",
    "SF_PICKING_FUNCTIONS":    "Picking / Raycasting",
    "SF_LOG_FUNCTIONS":        "Logging",
    "SF_MATH_FUNCTIONS":       "Math",
    "SF_IMPLEMENTATION_HELPERS": "Implementation Helpers",
    "SF_HEADER":               None,   # skip
    "SF_INCLUDES":             None,   # skip
    "SF_GAMMA_LUT":            None,   # skip
    "SF_FONT_DATA":            None,   # skip
    "SF_IMPLEMENTATION":       None,   # stop parsing here
}

# Functions/macros that are internal (not for public docs).
# Leading underscore = private implementation helper.
# Trailing underscore = raw function exposed only so the sf_get_*() macros
#   can call it; users should use the macro wrapper instead.
INTERNAL_PREFIXES = ("_",)
INTERNAL_SUFFIXES = ("_",)   # e.g. sf_get_obj_  →  use sf_get_obj() macro

# ---------------------------------------------------------------------------
# Parsing helpers
# ---------------------------------------------------------------------------

def is_internal(name: str) -> bool:
    return (any(name.startswith(p) for p in INTERNAL_PREFIXES) or
            any(name.endswith(s)   for s in INTERNAL_SUFFIXES))


def clean_comment(raw_body: str) -> str:
    """Strip leading * and whitespace from each line of a block comment body."""
    lines = []
    for line in raw_body.splitlines():
        line = re.sub(r'^\s*\*?\s?', '', line).rstrip()
        lines.append(line)
    # Trim leading/trailing blank lines
    while lines and not lines[0]:
        lines.pop(0)
    while lines and not lines[-1]:
        lines.pop()
    return '\n'.join(lines)


def parse_impl_comments(path: Path) -> dict:
    """
    Scan the SF_IMPLEMENTATION section for block comments immediately above
    a function definition.  Returns {function_name: cleaned_comment_text}.
    """
    raw = path.read_text()
    impl_m = re.search(r'/\* SF_IMPLEMENTATION \*/', raw)
    if not impl_m:
        return {}
    impl = raw[impl_m.end():]

    comments = {}

    # Match: func_name(...) {\n  /* comment */
    # The opening brace is followed immediately by the block comment on the next line.
    # Use (?:[^*]|\*(?!/))* so the comment body cannot cross a */ boundary.
    pattern = re.compile(
        r'\b(\w+)\s*\([^{]*\)\s*\{[ \t]*\n'   # func_name(...) {
        r'[ \t]*/\*((?:[^*]|\*(?!/))*)\*/',    # /* comment body */
    )

    for m in pattern.finditer(impl):
        func_name = m.group(1)
        body      = m.group(2)

        if not is_internal(func_name):
            comments[func_name] = clean_comment(body)

    return comments


def parse_header(path: Path) -> list:
    """
    Returns an ordered list of (tag, title, items) where each item is a dict
    with kind in 'define' | 'typedef' | 'enum' | 'struct' | 'func'.
    """
    raw = path.read_text()

    # Stop at SF_IMPLEMENTATION (we only want declarations, not bodies)
    impl_match = re.search(r'/\* SF_IMPLEMENTATION \*/', raw)
    if impl_match:
        raw = raw[:impl_match.start()]

    sections = []
    current_tag   = None
    current_title = None
    current_items = []

    lines = raw.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]

        # ---- Section marker ------------------------------------------------
        m = re.match(r'\s*/\* (SF_[A-Z_]+) \*/', line)
        if m:
            tag = m.group(1)
            if tag == "SF_IMPLEMENTATION":
                break
            if current_tag and current_title is not None:
                sections.append((current_tag, current_title, current_items))
            current_tag   = tag
            current_title = SECTION_TITLES.get(tag, tag)
            current_items = []
            i += 1
            continue

        # ---- Skip sections with no title -----------------------------------
        if current_title is None:
            i += 1
            continue

        # ---- #define -------------------------------------------------------
        m = re.match(r'#define\s+(\w+)\s+(.*)', line)
        if m and current_tag == "SF_DEFINES":
            name  = m.group(1)
            value = m.group(2).strip()
            if not is_internal(name):
                current_items.append({"kind": "define", "name": name, "value": value})
            i += 1
            continue

        if current_tag == "SF_TYPES":

            # ---- enum ------------------------------------------------------
            m = re.match(r'typedef\s+enum\s*\{?', line)
            if m:
                block_lines = [line]
                j = i + 1
                while j < len(lines):
                    block_lines.append(lines[j])
                    if re.search(r'\}\s*\w+\s*;', lines[j]):
                        break
                    j += 1
                block = " ".join(block_lines)
                name_m = re.search(r'\}\s*(\w+)\s*;', block)
                if name_m:
                    name = name_m.group(1)
                    inner = re.search(r'\{(.*)\}', block, re.DOTALL)
                    values = []
                    if inner:
                        for val in inner.group(1).split(","):
                            v = val.strip().split("=")[0].strip()
                            if v and not v.startswith("//"):
                                values.append(v)
                    if not is_internal(name):
                        current_items.append({"kind": "enum", "name": name, "values": values})
                i = j + 1
                continue

            # ---- struct (typedef struct ...) --------------------------------
            m = re.match(r'typedef\s+struct\s+', line)
            if m:
                block_lines = [line]
                j = i + 1
                depth = line.count("{") - line.count("}")
                # For forward-decl typedefs like `typedef struct foo_ foo;`
                # depth is 0 and we don't need to collect more lines.
                while j < len(lines) and depth > 0:
                    block_lines.append(lines[j])
                    depth += lines[j].count("{") - lines[j].count("}")
                    j += 1
                block = " ".join(block_lines)

                # For inline/multi-line structs: name is after the closing }
                # For forward-decl:             name is the last word before ;
                name_m = re.search(r'\}\s*(\w+)\s*;', block)
                if not name_m:
                    # forward declaration: typedef struct foo_ foo;
                    name_m = re.match(r'typedef\s+struct\s+\w+\s+(\w+)\s*;', line)
                if name_m:
                    name = name_m.group(1)
                    inner = re.search(r'\{(.*)\}', block, re.DOTALL)
                    fields = []
                    if inner:
                        for decl in inner.group(1).split(";"):
                            decl = decl.strip()
                            if not decl or decl.startswith("//") or decl.startswith("}"):
                                continue
                            # Declarations can have multiple variables: int x, y;
                            # Split on commas, take the last identifier from each part.
                            for var in decl.split(","):
                                tokens = [t for t in re.findall(r'\b(\w+)\b', var)
                                          if not t.isdigit()]
                                if tokens:
                                    fields.append(tokens[-1])
                    if not is_internal(name):
                        current_items.append({"kind": "struct", "name": name, "fields": fields})
                i = j if depth == 0 and j > i + 1 else i + 1
                continue

            # ---- function pointer typedef -----------------------------------
            # e.g. typedef void (*sf_log_fn)(const char*, void*);
            m = re.match(r'typedef\s+.+?\(\s*\*\s*(\w+)\s*\)', line)
            if m:
                name = m.group(1)
                # Underlying: strip 'typedef ' prefix and trailing ';'
                underlying = re.sub(r'\s+', ' ', line.strip().rstrip(';'))[len('typedef '):].strip()
                if not is_internal(name):
                    current_items.append({"kind": "typedef", "name": name, "underlying": underlying})
                i += 1
                continue

            # ---- simple typedef (no braces, no function pointer) -----------
            # e.g. typedef uint32_t sf_pkd_clr_t;
            if '{' not in line and '(' not in line:
                m = re.match(r'typedef\s+(\S.*\S)\s+(\w+)\s*;', line)
                if m:
                    name       = m.group(2)
                    underlying = m.group(1).strip()
                    if not is_internal(name):
                        current_items.append({"kind": "typedef", "name": name, "underlying": underlying})
                    i += 1
                    continue

        # ---- function declaration ------------------------------------------
        # Match: return_type   name (args...);  (2+ spaces between type and name)
        func_m = re.match(r'^(\w[\w\s\*]*?)\s{2,}(\w+)\s*\(', line)
        if func_m and current_tag and current_tag.endswith("_FUNCTIONS"):
            decl_lines = [line.rstrip()]
            j = i
            while ";" not in decl_lines[-1] and j + 1 < len(lines):
                j += 1
                decl_lines.append(lines[j].rstrip())
            decl = " ".join(l.strip() for l in decl_lines)
            name_m = re.match(r'.*?\s+(\w+)\s*\(', decl)
            if name_m:
                name = name_m.group(1)
                decl = re.sub(r'\s+', ' ', decl).strip().rstrip(';')
                if not is_internal(name):
                    current_items.append({"kind": "func", "name": name, "decl": decl})
            i = j + 1
            continue

        i += 1

    # Don't forget the last section
    if current_tag and current_title is not None and current_items:
        sections.append((current_tag, current_title, current_items))

    return sections


# ---------------------------------------------------------------------------
# Markdown generation
# ---------------------------------------------------------------------------

def func_signature(decl: str) -> str:
    return f"```c\n{decl};\n```"


def generate_markdown(sections: list, header_path: Path, comments: dict) -> str:
    out = []

    out.append("# Saffron Manual\n")
    out.append(f"> Auto-generated from `{header_path.name}` — run `python3 docs/gen_docs.py` to update.\n")

    # ---- Table of Contents -------------------------------------------------
    out.append("\n## Table of Contents\n")
    for tag, title, items in sections:
        if title is None or not items:
            continue
        anchor = title.lower().replace(" ", "-").replace("/", "").replace("&", "")
        anchor = re.sub(r'[^a-z0-9\-]', '', anchor)
        out.append(f"- [{title}](#{anchor})")

    # ---- Quick-reference function table ------------------------------------
    all_funcs = []
    for tag, title, items in sections:
        if title is None:
            continue
        for item in items:
            if item["kind"] == "func":
                all_funcs.append((title, item["name"], item["decl"]))

    if all_funcs:
        out.append("\n---\n")
        out.append("## Quick Reference\n")
        out.append("| Function | Section |")
        out.append("|----------|---------|")
        for section_title, name, _ in all_funcs:
            out.append(f"| `{name}` | {section_title} |")

    # ---- Sections ----------------------------------------------------------
    out.append("\n---\n")

    for tag, title, items in sections:
        if title is None or not items:
            continue

        out.append(f"\n## {title}\n")

        if tag == "SF_DEFINES":
            limits   = [d for d in items if d["kind"] == "define"
                        and not any(d["name"].startswith(p)
                                    for p in ("SF_CLR", "SF_LOG", "SF_ALIGN",
                                              "SF_DEG", "SF_RAD", "sf_get"))]
            clr_defs = [d for d in items if d["kind"] == "define" and d["name"].startswith("SF_CLR")]
            macros   = [d for d in items if d["kind"] == "define" and d not in limits and d not in clr_defs]

            if limits:
                out.append("### Capacity Limits\n")
                out.append("| Name | Value |")
                out.append("|------|-------|")
                for d in limits:
                    out.append(f"| `{d['name']}` | `{d['value']}` |")

            if clr_defs:
                out.append("\n### Built-in Colors\n")
                out.append("| Name | Value |")
                out.append("|------|-------|")
                for d in clr_defs:
                    out.append(f"| `{d['name']}` | `{d['value']}` |")

            if macros:
                out.append("\n### Macros\n")
                out.append("| Name | Expansion |")
                out.append("|------|-----------|")
                for d in macros:
                    out.append(f"| `{d['name']}` | `{d['value']}` |")

        elif tag == "SF_TYPES":
            enums   = [i for i in items if i["kind"] == "enum"]
            structs = [i for i in items if i["kind"] == "struct"]
            tdefs   = [i for i in items if i["kind"] == "typedef"]

            if tdefs:
                out.append("### Primitive Typedefs\n")
                out.append("| Name | Underlying |")
                out.append("|------|------------|")
                for t in tdefs:
                    out.append(f"| `{t['name']}` | `{t['underlying']}` |")

            if enums:
                out.append("\n### Enumerations\n")
                for e in enums:
                    vals = ", ".join(f"`{v}`" for v in e["values"] if v)
                    out.append(f"**`{e['name']}`** — {vals}\n")

            if structs:
                out.append("\n### Structs\n")
                for s in structs:
                    fields = ", ".join(f"`{f}`" for f in s["fields"] if f)
                    out.append(f"**`{s['name']}`** — fields: {fields}\n")

        else:
            # Function section
            funcs = [i for i in items if i["kind"] == "func"]
            if not funcs:
                continue
            for fn in funcs:
                out.append(f"### `{fn['name']}`\n")
                doc = comments.get(fn["name"])
                if doc:
                    out.append(doc)
                    out.append("")
                out.append(func_signature(fn["decl"]))
                out.append("")

    out.append("\n---\n")
    out.append("*Generated by `docs/gen_docs.py`*\n")

    return "\n".join(out)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    to_stdout = "--stdout" in sys.argv

    if not HEADER.exists():
        print(f"ERROR: cannot find {HEADER}", file=sys.stderr)
        sys.exit(1)

    sections = parse_header(HEADER)
    comments = parse_impl_comments(HEADER)
    md = generate_markdown(sections, HEADER, comments)

    if to_stdout:
        print(md)
    else:
        OUTPUT.write_text(md)
        n_commented = sum(1 for fn in
                          (i for _, _, items in sections for i in items if i["kind"] == "func")
                          if fn["name"] in comments)
        print(f"Written to {OUTPUT}  ({n_commented} functions have doc comments)")


if __name__ == "__main__":
    main()
