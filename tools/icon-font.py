#!/usr/bin/env python3
# Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Builds and verifies the embedded icon font. See docs/icon-font.md.

The app embeds exactly one icon font, `resources/icons/icon-font.ttf`, built from a Font
Awesome webfont:

  build          patch a source webfont and write it with its provenance file
  check          verify the tracked font against its provenance (used by CI and by the build)
  info           print what a font file contains
  mapping        regenerate resources/icons/icon-mapping.json from a Font Awesome package
                 plus resources/icons/icon-mapping-overrides.json
  check-mapping  verify the mapping, the overrides, the fallback file and the icon names the
                 sources use against each other and the tracked font (used by CI)

Patching does two things:

1. Emoji code points are unmapped. Font Awesome maps a few hundred emoji code points to
   icon glyphs, so any text containing an emoji would render as icons.
2. The font is renamed. Font Awesome Free is SIL OFL 1.1 with the Reserved Font Name
   "Font Awesome", so a modified version must not carry that name. Both editions are
   renamed to the same family, which keeps the QML independent of the edition.

Requires fontTools (`pip install fonttools`). Only needed to rebuild the font, not to
build the app.
"""

import argparse
import datetime
import hashlib
import json
import os
import re
import sys
import urllib.request

FAMILY = "UC Icons"
PS_NAME = "UCIcons"
EMOJI_BASE = "https://unicode.org/Public/emoji/15.1/"
EMOJI_FILES = ["emoji-sequences.txt", "emoji-zwj-sequences.txt"]
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONT = os.path.join(ROOT, "resources", "icons", "icon-font.ttf")
PROVENANCE = os.path.join(ROOT, "resources", "icons", "icon-font.json")
MAPPING = os.path.join(ROOT, "resources", "icons", "icon-mapping.json")
OVERRIDES = os.path.join(ROOT, "resources", "icons", "icon-mapping-overrides.json")
FALLBACK = os.path.join(ROOT, "resources", "icons", "icon-fallback.json")
SOURCES = os.path.join(ROOT, "src")


def sha256(path):
    with open(path, "rb") as handle:
        return hashlib.sha256(handle.read()).hexdigest()


def emoji_codepoints(cache_dir=None):
    """Code points of every emoji sequence, from the Unicode data files."""
    points = set()
    for name in EMOJI_FILES:
        cached = os.path.join(cache_dir, name) if cache_dir else None
        if cached and os.path.exists(cached):
            with open(cached, encoding="utf-8") as handle:
                text = handle.read()
        else:
            with urllib.request.urlopen(EMOJI_BASE + name) as response:
                text = response.read().decode("utf-8")
            if cached:
                with open(cached, "w", encoding="utf-8") as handle:
                    handle.write(text)
        for line in text.splitlines():
            line = line.split("#")[0].strip()
            if not line:
                continue
            field = line.split(";")[0].strip()
            if ".." in field:
                start, end = field.split("..")
                points.update(range(int(start, 16), int(end, 16) + 1))
            else:
                points.update(int(part, 16) for part in field.split())
    return points


def font_names(font):
    """The name table entries of interest, as {nameID: text}."""
    out = {}
    for record in font["name"].names:
        if record.nameID in (1, 2, 4, 5, 6, 16, 17) and record.nameID not in out:
            out[record.nameID] = record.toUnicode()
    return out


def cmd_info(args):
    from fontTools.ttLib import TTFont

    font = TTFont(args.font, lazy=True)
    names = font_names(font)
    print(f"file:        {args.font}")
    print(f"sha256:      {sha256(args.font)}")
    print(f"format:      {'OpenType/CFF' if font.sfntVersion == 'OTTO' else 'TrueType'}")
    print(f"family:      {names.get(1)} ({names.get(2)})")
    print(f"version:     {names.get(5)}")
    print(f"glyphs:      {font['maxp'].numGlyphs}")
    print(f"code points: {len(font.getBestCmap())}")
    return 0


def cmd_build(args):
    from fontTools.ttLib import TTFont

    source = args.source
    font = TTFont(source)
    original = font_names(font)

    match = re.search(r"Font Awesome version:\s*([0-9.]+)", original.get(5, ""))
    fa_version = match.group(1) if match else "unknown"
    edition = args.edition
    if edition is None:
        edition = "pro" if "Pro" in original.get(1, "") else "free"

    emoji = emoji_codepoints(args.emoji_cache)
    removed = set()
    for table in font["cmap"].tables:
        for codepoint in [cp for cp in table.cmap if cp in emoji]:
            removed.add(codepoint)
            del table.cmap[codepoint]

    subfamily = "Regular"
    unique = f"{FAMILY}; {fa_version} {original.get(2, '')}".strip()
    for name_id, value in ((1, FAMILY), (2, subfamily), (3, unique), (4, FAMILY),
                           (6, PS_NAME), (16, FAMILY), (17, subfamily)):
        font["name"].setName(value, name_id, 3, 1, 0x409)
        font["name"].setName(value, name_id, 1, 0, 0)
    if "CFF " in font:
        font["CFF "].cff.fontNames = [PS_NAME]

    font.save(args.output)

    provenance = {
        "comment": "Generated by tools/icon-font.py. See docs/icon-font.md.",
        "edition": edition,
        "fontAwesomeVersion": fa_version,
        "sourceFamily": original.get(1),
        "sourceStyle": original.get(2),
        "sourceVersion": original.get(5),
        "sourceFile": os.path.basename(source),
        "sourceSha256": sha256(source),
        "family": FAMILY,
        "emojiCodePointsRemoved": len(removed),
        "codePoints": len(TTFont(args.output, lazy=True).getBestCmap()),
        "sha256": sha256(args.output),
        "builtOn": datetime.date.today().isoformat(),
    }
    with open(args.provenance, "w", encoding="utf-8") as handle:
        json.dump(provenance, handle, indent=2, sort_keys=True)
        handle.write("\n")

    print(f"[+] {args.output}: {edition} edition, Font Awesome {fa_version}, "
          f"{provenance['codePoints']} code points, {len(removed)} emoji code points unmapped")
    print(f"[+] {args.provenance}")
    if edition == "pro":
        print("[!] Commercially licensed font: never commit this file or its provenance.")
    return 0


def cmd_check(args):
    """Verifies the font against its provenance. Returns non-zero on a mismatch."""
    from fontTools.ttLib import TTFont

    with open(args.provenance, encoding="utf-8") as handle:
        provenance = json.load(handle)
    problems = []

    actual = sha256(args.font)
    if actual != provenance["sha256"]:
        problems.append(f"{args.font} does not match its provenance file "
                        f"(sha256 {actual}, expected {provenance['sha256']})")

    font = TTFont(args.font, lazy=True)
    names = font_names(font)
    if names.get(1) != provenance["family"]:
        problems.append(f"family is '{names.get(1)}', expected '{provenance['family']}'")
    # OFL 1.1 forbids the Reserved Font Name in the *names* of a modified version. The
    # copyright (0), version (5) and license (13, 14) entries keep it and must stay intact.
    for name_id in (1, 2, 4, 6, 16, 17):
        if "Font Awesome" in (names.get(name_id) or ""):
            problems.append(f"the Reserved Font Name 'Font Awesome' is still in name ID {name_id}")

    if args.require_free:
        if provenance["edition"] != "free":
            problems.append(f"edition is '{provenance['edition']}', expected 'free'")
        if "Pro" in (provenance.get("sourceFamily") or ""):
            problems.append(f"built from a Pro font: {provenance.get('sourceFamily')}")

    for problem in problems:
        print(f"error: {problem}", file=sys.stderr)
    if problems:
        return 1
    print(f"[+] {os.path.basename(args.font)}: {provenance['edition']} edition, "
          f"Font Awesome {provenance['fontAwesomeVersion']}, family '{provenance['family']}'")
    return 0


def load_json(path):
    with open(path, encoding="utf-8") as handle:
        return json.load(handle)


def package_version(package_dir):
    try:
        return load_json(os.path.join(package_dir, "package.json"))["version"]
    except (OSError, KeyError, ValueError):
        return "unknown"


def icon_names(package_dir):
    """Canonical icon name -> character for every icon of a Font Awesome package.

    Reads metadata/icon-families.json when the package has it: it lists every icon of the
    release (all editions) with its aliases. A package without it, e.g. a trimmed copy, still
    has scss/_variables.scss, where the first name of a code point is the canonical one and the
    names that follow are its aliases; aliases are not mapped, the overrides file is the place
    for extra names.
    """
    metadata = os.path.join(package_dir, "metadata", "icon-families.json")
    scss = os.path.join(package_dir, "scss", "_variables.scss")
    if os.path.exists(metadata):
        families = load_json(metadata)
        return {name: chr(int(entry["unicode"], 16)) for name, entry in families.items()}, "metadata"
    if os.path.exists(scss):
        names = {}
        with open(scss, encoding="utf-8") as handle:
            for match in re.finditer(r"^\$fa-var-([a-z0-9-]+):\s*\\([0-9a-f]+);", handle.read(), re.M):
                glyph = chr(int(match.group(2), 16))
                if glyph not in names.values():
                    names[match.group(1)] = glyph
        return names, "scss"
    sys.exit(f"error: {package_dir} has neither metadata/icon-families.json nor scss/_variables.scss")


def used_icon_names():
    """Every literal uc:<name> in the QML and C++ sources."""
    names = set()
    for directory, _, files in os.walk(SOURCES):
        for name in files:
            if name.endswith((".qml", ".cpp", ".h")):
                with open(os.path.join(directory, name), encoding="utf-8") as handle:
                    names.update(re.findall(r"uc:([a-z0-9_-]+)", handle.read()))
    return names


def cmd_mapping(args):
    names, source = icon_names(args.package)
    overrides = load_json(args.overrides)["overrides"]

    missing = [(name, target) for name, target in overrides.items() if target not in names]
    for name, target in missing:
        print(f"error: override {name} -> {target}: {target} is not an icon of this release", file=sys.stderr)
    if missing:
        return 1

    mapping = dict(names)
    for name, target in overrides.items():
        mapping[name] = names[target]

    with open(args.output, "w", encoding="utf-8") as handle:
        json.dump(mapping, handle, indent=1, sort_keys=True, ensure_ascii=True)
        handle.write("\n")

    print(f"[+] {args.output}: {len(names)} icons of Font Awesome {package_version(args.package)} ({source}) "
          f"+ {len(overrides)} overrides = {len(mapping)} names")
    return 0


def cmd_check_mapping(args):
    """Verifies the mapping, the overrides, the fallback file and the sources against each other."""
    from fontTools.ttLib import TTFont

    mapping = load_json(args.mapping)
    overrides = load_json(args.overrides)["overrides"]
    fallback_doc = load_json(args.fallback)
    fallback = fallback_doc["fallback"]
    placeholder = fallback_doc["placeholder"]
    cmap = TTFont(args.font, lazy=True).getBestCmap()
    problems = []

    def drawable(name):
        return name in mapping and ord(mapping[name]) in cmap

    for name, target in overrides.items():
        if target not in mapping:
            problems.append(f"override {name} -> {target}: {target} is not in the mapping")
        elif target in overrides:
            # The target's own glyph is not in the mapping under its name (e.g. list-alt -> list
            # while list itself is overridden), so only the generator can verify this one.
            continue
        elif mapping.get(name) != mapping[target]:
            problems.append(f"override {name} -> {target} is not applied in the mapping (run 'mapping')")

    if not drawable(placeholder):
        problems.append(f"the placeholder {placeholder} is not drawable by {os.path.basename(args.font)}")
    for name, target in fallback.items():
        if name not in mapping:
            problems.append(f"fallback {name} -> {target}: {name} is not in the mapping")
        if not drawable(target):
            problems.append(f"fallback {name} -> {target}: {os.path.basename(args.font)} cannot draw {target}")

    used = used_icon_names()
    for name in sorted(used):
        if name not in mapping:
            problems.append(f"the sources use uc:{name}, which is not in the mapping")
        elif not drawable(name) and name not in fallback:
            problems.append(f"the sources use uc:{name}, which {os.path.basename(args.font)} cannot draw "
                            f"and which has no fallback entry")

    for problem in problems:
        print(f"error: {problem}", file=sys.stderr)
    if problems:
        return 1
    print(f"[+] {os.path.basename(args.mapping)}: {len(mapping)} names, {len(overrides)} overrides, "
          f"{len(fallback)} fallbacks, {len(used)} names used by the sources, all consistent")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)

    build = sub.add_parser("build", help="patch a source webfont into the embedded icon font")
    build.add_argument("source", help="Font Awesome webfont, e.g. webfonts/fa-solid-900.ttf")
    build.add_argument("-o", "--output", default=FONT)
    build.add_argument("-p", "--provenance", default=PROVENANCE)
    build.add_argument("--edition", choices=["free", "pro"], default=None)
    build.add_argument("--emoji-cache", default=None, help="directory for the Unicode emoji data files")
    build.set_defaults(func=cmd_build)

    check = sub.add_parser("check", help="verify the font against its provenance file")
    check.add_argument("-f", "--font", default=FONT)
    check.add_argument("-p", "--provenance", default=PROVENANCE)
    check.add_argument("--require-free", action="store_true", help="also fail if the font is not the Free edition")
    check.set_defaults(func=cmd_check)

    info = sub.add_parser("info", help="print what a font file contains")
    info.add_argument("font")
    info.set_defaults(func=cmd_info)

    mapping = sub.add_parser("mapping", help="regenerate the icon name mapping from a Font Awesome package")
    mapping.add_argument("package", help="extracted Font Awesome package (Pro, so that every icon is listed)")
    mapping.add_argument("-o", "--output", default=MAPPING)
    mapping.add_argument("--overrides", default=OVERRIDES)
    mapping.set_defaults(func=cmd_mapping)

    check_mapping = sub.add_parser("check-mapping", help="verify mapping, overrides, fallbacks and the sources")
    check_mapping.add_argument("-m", "--mapping", default=MAPPING)
    check_mapping.add_argument("--overrides", default=OVERRIDES)
    check_mapping.add_argument("--fallback", default=FALLBACK)
    check_mapping.add_argument("-f", "--font", default=FONT)
    check_mapping.set_defaults(func=cmd_check_mapping)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
