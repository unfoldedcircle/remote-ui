#!/usr/bin/env python3
# Repairs plural messages in translation files exported from SimpleLocalize.
#
# SimpleLocalize collapses a Qt numerus message into a single ICU plural string, which Qt Linguist
# does not understand — lrelease drops the message and the app falls back to the English source:
#
#     <message>
#         <source>%n device(s) need attention</source>
#         <translation>{%n, plural, one {%n Gerät braucht Aufmerksamkeit} two {%n Geräte brauchen Aufmerksamkeit}}</translation>
#     </message>
#
# This script turns every such message back into the Qt Linguist form, keeping the branch order of
# the ICU string (SimpleLocalize numbers the branches one/two/three... in numerusform order):
#
#     <message numerus="yes">
#         <source>%n device(s) need attention</source>
#         <translation>
#           <numerusform>%n Gerät braucht Aufmerksamkeit</numerusform>
#           <numerusform>%n Geräte brauchen Aufmerksamkeit</numerusform>
#         </translation>
#     </message>
#
# Which messages are plural is taken from the master English file, the only source of truth for the
# message catalogue: every <message numerus="yes"> in it is matched by <source> text.
#
# Run it on the l10n export before committing, from anywhere in the repo:
#
#     scripts/fix-icu-plurals.py                 # patches resources/translations in place
#     scripts/fix-icu-plurals.py --dry-run -v    # show what would change
#     scripts/fix-icu-plurals.py some/other/dir
#
# Only files whose plural messages are still in the ICU form are touched, so re-running is a no-op.
# Exits 1 if a plural message could not be converted.

import argparse
import html
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MASTER = REPO_ROOT / "resources" / "translations" / "en_US.ts"
DEFAULT_FOLDER = REPO_ROOT / "resources" / "translations"

# indentation of a <numerusform> relative to its <translation>
FORM_INDENT = 2

MESSAGE_RE = re.compile(r"<message\b[^>]*>.*?</message>", re.S)
SOURCE_RE = re.compile(r"<source>(.*?)</source>", re.S)
TRANSLATION_RE = re.compile(r"([ \t]*)<translation\b([^>]*?)(?:/>|>(.*?)</translation>)", re.S)
ICU_RE = re.compile(r"^\{\s*[^,{}]*,\s*plural\s*,(.*)\}$", re.S)
# any English file is a master, never a target: en_US.ts, en_GB.ts, translations_en_US.ts, ...
ENGLISH_RE = re.compile(r"(^|_)en(_|$)")
BRANCH_RE = re.compile(r"\s*(=\d+|[A-Za-z]+)\s*\{")

# ICU keywords SimpleLocalize may emit; anything else is reported but still converted in place order
KNOWN_BRANCHES = {"zero", "one", "two", "three", "four", "five", "six", "few", "many", "other"}


def parse_icu_plural(text):
    """Split an ICU plural string into its branches, in the order they appear.

    Returns a list of (keyword, value) pairs, or None if the text is not an ICU plural string.
    Raises ValueError if it looks like one but cannot be parsed.
    """
    body = ICU_RE.match(text.strip())
    if not body:
        return None

    body = body.group(1)
    branches = []
    pos = 0
    while pos < len(body):
        if body[pos].isspace():
            pos += 1
            continue
        match = BRANCH_RE.match(body, pos)
        if not match:
            raise ValueError(f"expected a plural branch at {body[pos:pos + 20]!r}")
        # brace matching: a branch may legitimately contain braces of its own
        depth = 1
        end = match.end()
        while end < len(body) and depth:
            if body[end] == "{":
                depth += 1
            elif body[end] == "}":
                depth -= 1
            end += 1
        if depth:
            raise ValueError(f"unterminated branch {match.group(1)!r}")
        branches.append((match.group(1), body[match.end():end - 1]))
        pos = end

    if not branches:
        raise ValueError("no plural branches")
    return branches


def plural_sources(master):
    """The <source> texts of every numerus message in the master file, unescaped for comparison."""
    text = master.read_text(encoding="utf-8")
    sources = set()
    for block in MESSAGE_RE.finditer(text):
        if 'numerus="yes"' not in block.group(0).split(">", 1)[0]:
            continue
        source = SOURCE_RE.search(block.group(0))
        if source:
            sources.add(html.unescape(source.group(1)))
    return sources


def patch_message(block, sources, problems):
    """Convert one <message> block if it holds an ICU plural. Returns the block, changed flag."""
    source = SOURCE_RE.search(block)
    if not source or html.unescape(source.group(1)) not in sources:
        return block, False

    name = html.unescape(source.group(1))
    if "<numerusform>" in block:
        return block, False  # already in Qt form

    translation = TRANSLATION_RE.search(block)
    if not translation:
        problems.append(f"{name!r}: no <translation> element")
        return block, False

    indent, attrs, value = translation.group(1), translation.group(2), translation.group(3)
    if value is None:
        problems.append(f"{name!r}: empty <translation/>, nothing to convert")
        return block, False

    try:
        branches = parse_icu_plural(value)
    except ValueError as err:
        problems.append(f"{name!r}: {err}")
        return block, False
    if branches is None:
        problems.append(f"{name!r}: plural in the master file, but the translation is a plain string")
        return block, False

    unknown = [kw for kw, _ in branches if kw not in KNOWN_BRANCHES and not kw.startswith("=")]
    if unknown:
        problems.append(f"{name!r}: unexpected plural branch(es) {unknown}, kept in the exported order")

    forms = "\n".join(f"{indent}{' ' * FORM_INDENT}<numerusform>{v.strip()}</numerusform>"
                      for _, v in branches)
    qt_form = f"{indent}<translation{attrs.rstrip()}>\n{forms}\n{indent}</translation>"
    block = block[:translation.start()] + qt_form + block[translation.end():]

    # mark the message itself as numerus, the way lupdate writes it
    open_tag, rest = block.split(">", 1)
    if 'numerus="yes"' not in open_tag:
        block = f"{open_tag.rstrip()} numerus=\"yes\">{rest}"
    return block, True


def patch_file(path, sources, dry_run):
    """Returns (sources of the fixed messages, list of problems)."""
    text = path.read_text(encoding="utf-8")
    problems = []
    fixed = []

    def replace(match):
        block, changed = patch_message(match.group(0), sources, problems)
        if changed:
            source = SOURCE_RE.search(block)
            fixed.append(html.unescape(source.group(1)))
        return block

    patched = MESSAGE_RE.sub(replace, text)
    if fixed and not dry_run:
        path.write_text(patched, encoding="utf-8")
    return fixed, problems


def main():
    parser = argparse.ArgumentParser(
        description="Convert SimpleLocalize's ICU plural strings back to Qt Linguist numerusforms.")
    parser.add_argument("folder", nargs="?", type=Path, default=DEFAULT_FOLDER,
                        help=f"folder with the translation files (default: {DEFAULT_FOLDER})")
    parser.add_argument("--master", type=Path, default=DEFAULT_MASTER,
                        help=f"master English file listing the plural messages (default: {DEFAULT_MASTER})")
    parser.add_argument("-n", "--dry-run", action="store_true", help="report what would change, write nothing")
    parser.add_argument("-v", "--verbose", action="store_true", help="list every converted message")
    args = parser.parse_args()

    if not args.master.is_file():
        sys.exit(f"master file not found: {args.master}")
    if not args.folder.is_dir():
        sys.exit(f"folder not found: {args.folder}")

    sources = plural_sources(args.master)
    if not sources:
        print(f"no plural messages in {args.master}, nothing to do")
        return 0
    print(f"{len(sources)} plural message(s) in {args.master}")

    master = args.master.resolve()
    targets = sorted(p for p in args.folder.glob("*.ts")
                     if p.resolve() != master and not ENGLISH_RE.search(p.stem))
    if not targets:
        sys.exit(f"no non-English .ts files in {args.folder}")

    total = 0
    failed = False
    for path in targets:
        fixed, problems = patch_file(path, sources, args.dry_run)
        total += len(fixed)
        if fixed or problems:
            print(f"  {path.name}: {len(fixed)} fixed" + (" (dry run)" if args.dry_run and fixed else ""))
        if args.verbose:
            for name in fixed:
                print(f"    {name!r}")
        for problem in problems:
            print(f"    WARNING {problem}")
            failed = True

    print(f"{total} message(s) converted in {len(targets)} file(s)"
          + (" — dry run, nothing written" if args.dry_run else ""))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
