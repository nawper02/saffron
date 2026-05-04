#!/usr/bin/env python3
"""
Reorder saffron so that the implementation section matches the order of the header section.

Usage:
  python sf_reorganize.py saffron.h
  python sf_reorganize.py saffron.h -o saffron.reordered.h
  python sf_reorganize.py saffron.h --in-place --backup
  python sf_reorganize.py saffron.h --check
"""
from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

HEADER_MARK = "/* SF_HEADER */"
IMPL_MARK = "/* SF_IMPLEMENTATION */"
CATEGORY_RE = re.compile(r"^[ \t]*/\*\s*(SF_[A-Z0-9_]+)\s*\*/[ \t]*(?:\r?\n|$)", re.M)
IDENT = r"[A-Za-z_]\w*"
KEYWORDS = {
    "if", "for", "while", "switch", "return", "sizeof", "typedef", "struct",
    "enum", "union", "do", "case", "else",
}


@dataclass
class Category:
    name: str
    marker_start: int
    marker_end: int
    body_start: int
    body_end: int
    marker_text: str
    body_text: str


@dataclass
class FuncBlock:
    name: str
    start: int
    end: int
    text: str
    category: Optional[str]


def find_matching_brace(s: str, open_i: int) -> int:
    """Return index one past the matching top-level close brace."""
    depth = 0
    i = open_i
    n = len(s)
    state = "code"
    while i < n:
        ch = s[i]
        nxt = s[i + 1] if i + 1 < n else ""

        if state == "code":
            if ch == "/" and nxt == "/":
                state = "line_comment"; i += 2; continue
            if ch == "/" and nxt == "*":
                state = "block_comment"; i += 2; continue
            if ch == '"':
                state = "string"; i += 1; continue
            if ch == "'":
                state = "char"; i += 1; continue
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    i += 1
                    # Include trailing spaces/tabs and one newline after the closing brace.
                    while i < n and s[i] in " \t":
                        i += 1
                    if i < n and s[i] == "\r":
                        i += 1
                        if i < n and s[i] == "\n":
                            i += 1
                    elif i < n and s[i] == "\n":
                        i += 1
                    return i
            i += 1; continue

        if state == "line_comment":
            if ch == "\n":
                state = "code"
            i += 1; continue
        if state == "block_comment":
            if ch == "*" and nxt == "/":
                state = "code"; i += 2; continue
            i += 1; continue
        if state == "string":
            if ch == "\\":
                i += 2; continue
            if ch == '"':
                state = "code"
            i += 1; continue
        if state == "char":
            if ch == "\\":
                i += 2; continue
            if ch == "'":
                state = "code"
            i += 1; continue

    raise ValueError(f"unmatched brace at byte offset {open_i}")


def iter_categories(section: str) -> List[Category]:
    matches = [m for m in CATEGORY_RE.finditer(section)
               if m.group(1) not in {"SF_HEADER", "SF_IMPLEMENTATION"}]
    cats: List[Category] = []
    for idx, m in enumerate(matches):
        body_end = matches[idx + 1].start() if idx + 1 < len(matches) else len(section)
        cats.append(Category(
            name=m.group(1),
            marker_start=m.start(),
            marker_end=m.end(),
            body_start=m.end(),
            body_end=body_end,
            marker_text=section[m.start():m.end()],
            body_text=section[m.end():body_end],
        ))
    return cats


def split_statements(text: str) -> Iterable[str]:
    """Yield top-level-ish semicolon statements from header category text."""
    start = 0
    for m in re.finditer(r";", text):
        yield text[start:m.end()]
        start = m.end()


def function_name_from_prototype(stmt: str) -> Optional[str]:
    stripped = re.sub(r"/\*.*?\*/", "", stmt, flags=re.S).strip()
    if not stripped or "(" not in stripped or not stripped.endswith(";"):
        return None
    if stripped.startswith(("typedef", "#")):
        return None
    # Ignore variables/arrays like static const uint8_t foo[256];
    if ")" not in stripped:
        return None
    # For normal C prototypes, the function name is the identifier immediately
    # before the first argument list. This also handles names with no space
    # before '(', e.g. _sf_sff_prse_sprit_3d(...).
    m = re.search(rf"\b({IDENT})\s*\(", stripped)
    if not m:
        return None
    name = m.group(1)
    return None if name in KEYWORDS else name


def parse_header_order(header: str) -> Tuple[List[str], Dict[str, List[str]], Dict[str, str]]:
    cats = iter_categories(header)
    category_order: List[str] = []
    funcs_by_cat: Dict[str, List[str]] = {}
    marker_by_cat: Dict[str, str] = {}
    seen: set[str] = set()

    for cat in cats:
        marker_by_cat.setdefault(cat.name, cat.marker_text)
        names: List[str] = []
        for stmt in split_statements(cat.body_text):
            name = function_name_from_prototype(stmt)
            if name:
                names.append(name)
        if names:
            if cat.name not in seen:
                category_order.append(cat.name)
                seen.add(cat.name)
            funcs_by_cat.setdefault(cat.name, []).extend(names)
    return category_order, funcs_by_cat, marker_by_cat


def find_line_start(s: str, i: int) -> int:
    j = s.rfind("\n", 0, i)
    return 0 if j < 0 else j + 1


def looks_like_function_prefix(prefix: str) -> Optional[str]:
    """Given text before a top-level '{', return function name if it is a def."""
    # Keep only the tail after obvious separators. Definitions in this project
    # start on one line, but this permits multi-line signatures too.
    tail = prefix.rstrip()
    if not tail or tail.endswith(("=", ";", ":")):
        return None
    if re.search(r"(^|\n)[ \t]*#", tail):
        return None
    if "(" not in tail or ")" not in tail:
        return None
    first_word = re.match(r"\s*([A-Za-z_]\w*)", tail)
    if first_word and first_word.group(1) in {"if", "for", "while", "switch"}:
        return None
    names = re.findall(rf"\b({IDENT})\s*\(", tail)
    names = [x for x in names if x not in KEYWORDS]
    if not names:
        return None
    return names[0]


def find_function_blocks(section: str, cat_ranges: List[Category]) -> List[FuncBlock]:
    """Find top-level function definitions inside the implementation section."""
    blocks: List[FuncBlock] = []
    for cat in cat_ranges:
        body = cat.body_text
        body_abs = cat.body_start
        i = 0
        search_from = 0
        while True:
            brace = body.find("{", search_from)
            if brace < 0:
                break
            line_start = find_line_start(body, brace)
            prefix = body[line_start:brace]
            name = looks_like_function_prefix(prefix)
            if not name:
                search_from = brace + 1
                continue
            end_rel = find_matching_brace(body, brace)
            start_abs = body_abs + line_start
            end_abs = body_abs + end_rel
            blocks.append(FuncBlock(name=name,
                                    start=start_abs,
                                    end=end_abs,
                                    text=section[start_abs:end_abs],
                                    category=cat.name))
            search_from = end_rel
    return blocks


def remove_ranges(s: str, ranges: List[Tuple[int, int]]) -> str:
    if not ranges:
        return s
    ranges = sorted(ranges)
    out: List[str] = []
    pos = 0
    for a, b in ranges:
        out.append(s[pos:a])
        pos = b
    out.append(s[pos:])
    return "".join(out)

def normalize_category_spacing(s: str) -> str:
    """
    Collapse excessive blank lines before SF category markers.

    This does not alter function bodies. It only changes whitespace directly
    before lines like:
        /* SF_GAMMA_LUT */
        /* SF_FONT_DATA */
    """
    return re.sub(
        r"\n{3,}([ \t]*/\*\s*SF_[A-Z0-9_]+\s*\*/)",
        r"\n\n\1",
        s
    )

def tidy_between_blocks(parts: List[str]) -> str:
    """Join blocks with exactly the blank lines already implied by block text."""
    return "".join(parts)


def reorder(text: str, *, verbose: bool = False) -> Tuple[str, List[str]]:
    notes: List[str] = []
    header_i = text.find(HEADER_MARK)
    impl_i = text.find(IMPL_MARK)
    if header_i < 0 or impl_i < 0 or impl_i <= header_i:
        raise ValueError("could not find /* SF_HEADER */ followed by /* SF_IMPLEMENTATION */")

    before_impl = text[:impl_i]
    impl = text[impl_i:]
    header = text[header_i:impl_i]

    header_cat_order, header_funcs_by_cat, header_marker_by_cat = parse_header_order(header)
    impl_cats = iter_categories(impl)
    if not impl_cats:
        raise ValueError("no SF_* category markers found in implementation section")

    blocks = find_function_blocks(impl, impl_cats)
    blocks_by_name: Dict[str, FuncBlock] = {}
    dupes: Dict[str, int] = {}
    for b in blocks:
        if b.name in blocks_by_name:
            dupes[b.name] = dupes.get(b.name, 1) + 1
        else:
            blocks_by_name[b.name] = b
    if dupes:
        notes.append("duplicate implementation definitions not moved: " + ", ".join(sorted(dupes)))

    # Leftovers are each category body with only function blocks removed.
    blocks_by_cat: Dict[str, List[FuncBlock]] = {}
    for b in blocks:
        blocks_by_cat.setdefault(b.category or "", []).append(b)

    leftover_by_cat: Dict[str, str] = {}
    marker_by_cat: Dict[str, str] = {}
    impl_cat_order: List[str] = []
    for cat in impl_cats:
        impl_cat_order.append(cat.name)
        marker_by_cat[cat.name] = cat.marker_text
        rel_ranges = [(b.start - cat.body_start, b.end - cat.body_start)
                      for b in blocks_by_cat.get(cat.name, [])]
        leftover_by_cat[cat.name] = remove_ranges(cat.body_text, rel_ranges)

    moved_names: set[str] = set()
    used_categories: set[str] = set()
    new_pieces: List[str] = [impl[:impl_cats[0].marker_start]]

    for cat_name in header_cat_order:
        used_categories.add(cat_name)
        marker = marker_by_cat.get(cat_name) or header_marker_by_cat.get(cat_name) or f"/* {cat_name} */\n"
        new_pieces.append(marker)

        body_parts: List[str] = []
        missing: List[str] = []
        for fn in header_funcs_by_cat.get(cat_name, []):
            b = blocks_by_name.get(fn)
            if b:
                body_parts.append(b.text)
                moved_names.add(fn)
                if not b.text.endswith("\n"):
                    body_parts.append("\n")
                # Match the common existing style: one blank line between defs.
                if not b.text.endswith("\n\n"):
                    body_parts.append("\n")
            else:
                missing.append(fn)
        if missing:
            notes.append(f"missing implementations for {cat_name}: " + ", ".join(missing))

        # Definitions in this category that are not declared in the header are
        # left in their category, after the header-ordered definitions.
        extras_here = [b for b in blocks_by_cat.get(cat_name, []) if b.name not in moved_names]
        if extras_here:
            notes.append(f"extra implementation definitions kept in {cat_name}: " + ", ".join(b.name for b in extras_here))
            for b in extras_here:
                body_parts.append(b.text)
                moved_names.add(b.name)
                if not b.text.endswith("\n"):
                    body_parts.append("\n")
                if not b.text.endswith("\n\n"):
                    body_parts.append("\n")

        leftover = leftover_by_cat.get(cat_name, "")
        if leftover.strip():
            # Preserve non-function text from that category after ordered functions.
            if body_parts and not body_parts[-1].endswith("\n"):
                body_parts.append("\n")
            body_parts.append(leftover)
        new_pieces.append(tidy_between_blocks(body_parts))

    # Keep implementation categories that have no header function category match
    # (for example data tables) in their original relative order.
    for cat_name in impl_cat_order:
        if cat_name in used_categories:
            continue
        new_pieces.append(marker_by_cat[cat_name])
        kept = leftover_by_cat.get(cat_name, "")
        for b in blocks_by_cat.get(cat_name, []):
            if b.name not in moved_names:
                kept += b.text
                if not b.text.endswith("\n\n"):
                    kept += "\n"
        new_pieces.append(kept)

        new_impl = normalize_category_spacing("".join(new_pieces))
        return before_impl + new_impl, notes


def main(argv: Optional[List[str]] = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("path", type=Path, help="single-header C file to reorder")
    ap.add_argument("-o", "--output", type=Path, help="write result to this path")
    ap.add_argument("--in-place", action="store_true", help="rewrite the input file")
    ap.add_argument("--backup", action="store_true", help="with --in-place, also write PATH.bak first")
    ap.add_argument("--check", action="store_true", help="do not write; exit 1 if changes would be made")
    ap.add_argument("--encoding", default="utf-8", help="file encoding, default utf-8")
    args = ap.parse_args(argv)

    with args.path.open("r", encoding=args.encoding, newline="") as f:
        src = f.read()
    dst, notes = reorder(src)

    for note in notes:
        print("note:", note, file=sys.stderr)

    changed = (dst != src)
    if args.check:
        print("would change" if changed else "already ordered")
        return 1 if changed else 0

    if args.in_place:
        if args.backup:
            args.path.with_suffix(args.path.suffix + ".bak").write_text(src, encoding=args.encoding, newline="")
        args.path.write_text(dst, encoding=args.encoding, newline="")
        print(f"rewrote {args.path}")
    else:
        out = args.output or args.path.with_suffix(args.path.suffix + ".reordered")
        out.write_text(dst, encoding=args.encoding, newline="")
        print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
