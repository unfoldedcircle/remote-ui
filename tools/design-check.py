#!/usr/bin/env python3
# Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Checks the QML sources against the rules of the design system (docs/design-system.md).

The rules a reviewer would otherwise have to catch by eye:

  font-size        a pixel font size below 22 (fonts.primaryFont(20), font.pixelSize: 18, ...)
  literal-colour   a colour written as a literal instead of a token: "#3a3a3a", color: "pink",
                   Qt.rgba(0, 0, 0, 0.5) (Qt.rgba / Qt.hsla / Qt.hsva built from a token are fine)
  lighter-darker   Qt.lighter / Qt.darker / Qt.tint: shades outside the token set

Files on the allow-list (tools/design-check-allow.txt) are skipped, each with its reason: the
entity detail screens follow the design system in a later change (decision D-7).

  design-check.py [paths ...]   check the given files or folders (default: src/qml)
  design-check.py --self-test   check the rules against seeded violations

Exits with 1 when a rule is broken. Uses the Python standard library only.
"""

import argparse
import fnmatch
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
ALLOW_LIST = os.path.join(ROOT, "tools", "design-check-allow.txt")
MIN_FONT_SIZE = 22

FONT_CALL = re.compile(r"\bfonts\.(\w+)\(\s*(\d+)")
FONT_PROPERTY = re.compile(r"\b(pixelSize|pointSize)\s*:\s*(\d+)")
HEX_COLOUR = re.compile(r"[\"']#(?:[0-9a-fA-F]{3,4}|[0-9a-fA-F]{6}|[0-9a-fA-F]{8})[\"']")
NAMED_COLOUR = re.compile(r"\b\w*[cC]olor\s*:\s*[\"'][a-zA-Z]+[\"']")
QT_COLOUR = re.compile(r"\bQt\.(rgba|rgb|hsla|hsva)\s*\(")
QT_SHADE = re.compile(r"\bQt\.(lighter|darker|tint)\s*\(")


def strip_comments(text):
    """Blanks // and /* */ comments outside string literals, keeping the line structure."""
    out = []
    i = 0
    quote = None
    while i < len(text):
        c = text[i]
        if quote:
            out.append(c)
            if c == "\\" and i + 1 < len(text):
                out.append(text[i + 1])
                i += 2
                continue
            if c == quote or c == "\n":
                quote = None
            i += 1
            continue
        if c in "\"'`":
            quote = c
            out.append(c)
            i += 1
        elif text.startswith("//", i):
            end = text.find("\n", i)
            end = len(text) if end < 0 else end
            out.append(" " * (end - i))
            i = end
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = len(text) if end < 0 else end + 2
            out.append("".join("\n" if ch == "\n" else " " for ch in text[i:end]))
            i = end
        else:
            out.append(c)
            i += 1
    return "".join(out)


def call_arguments(text, start):
    """The text between the parenthesis that opens at or after `start` and its partner."""
    open_at = text.index("(", start)
    depth = 0
    for i in range(open_at, len(text)):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return text[open_at + 1:i]
    return text[open_at + 1:]


def check_text(text):
    """Yields (line, rule, snippet) for every broken rule in the QML source text."""
    code = strip_comments(text)
    line_starts = [0] + [m.end() for m in re.finditer("\n", code)]

    def line_of(pos):
        lo, hi = 0, len(line_starts) - 1
        while lo < hi:
            mid = (lo + hi + 1) // 2
            if line_starts[mid] <= pos:
                lo = mid
            else:
                hi = mid - 1
        return lo + 1

    findings = []
    for m in FONT_CALL.finditer(code):
        if m.group(1) != "iconFont" and int(m.group(2)) < MIN_FONT_SIZE:
            findings.append((line_of(m.start()), "font-size", m.group(0) + ")"))
    for m in FONT_PROPERTY.finditer(code):
        if int(m.group(2)) < MIN_FONT_SIZE:
            findings.append((line_of(m.start()), "font-size", m.group(0)))
    for m in HEX_COLOUR.finditer(code):
        findings.append((line_of(m.start()), "literal-colour", m.group(0)))
    for m in NAMED_COLOUR.finditer(code):
        findings.append((line_of(m.start()), "literal-colour", m.group(0)))
    for m in QT_COLOUR.finditer(code):
        args = call_arguments(code, m.start())
        if "colors." not in args:
            findings.append((line_of(m.start()), "literal-colour", "Qt.%s(%s)" % (m.group(1), args.strip())))
    for m in QT_SHADE.finditer(code):
        findings.append((line_of(m.start()), "lighter-darker", m.group(0) + "...)"))
    return sorted(findings)


def load_allow_list(path=ALLOW_LIST):
    patterns = []
    with open(path, encoding="utf-8") as f:
        for raw in f:
            entry = raw.split("#", 1)[0].strip()
            if entry:
                patterns.append(entry)
    return patterns


def allowed(rel_path, patterns):
    return any(fnmatch.fnmatchcase(rel_path, p) for p in patterns)


def qml_files(paths):
    for path in paths:
        if os.path.isfile(path):
            yield path
            continue
        for folder, _, names in os.walk(path):
            for name in sorted(names):
                if name.endswith((".qml", ".js")):
                    yield os.path.join(folder, name)


def run(paths):
    patterns = load_allow_list()
    broken = 0
    checked = 0
    for path in qml_files(paths):
        rel = os.path.relpath(os.path.abspath(path), ROOT).replace(os.sep, "/")
        if allowed(rel, patterns):
            continue
        checked += 1
        with open(path, encoding="utf-8") as f:
            for line, rule, snippet in check_text(f.read()):
                print("%s:%d: [%s] %s" % (rel, line, rule, snippet))
                broken += 1
    print("design-check: %d file(s) checked, %d problem(s)" % (checked, broken))
    if not checked:
        # a wrong path must not pass as a clean tree
        print("design-check: no QML file found in %s" % ", ".join(paths))
        return 1
    return 1 if broken else 0


# Seeded violations for --self-test: (QML snippet, rules it must raise)
SELF_TEST = [
    ('Text { font: fonts.primaryFont(20) }', ["font-size"]),
    ('Text { font: fonts.secondaryFont(21, "Medium") }', ["font-size"]),
    ('Text { font.pixelSize: 18 }', ["font-size"]),
    ('Text { font { pointSize: 12 } }', ["font-size"]),
    ('Text { font: fonts.primaryFont(22) }', []),
    ('Icon { font: fonts.iconFont(16) }', []),
    ('Rectangle { color: "#3a3a3a" }', ["literal-colour"]),
    ("Rectangle { color: '#80FFFFFF' }", ["literal-colour"]),
    ('Rectangle { border.color: "pink" }', ["literal-colour"]),
    ('GradientStop { color: Qt.rgba(0, 0, 0, 0.5) }', ["literal-colour"]),
    ('GradientStop { color: Qt.rgba(colors.bg.r, colors.bg.g, colors.bg.b, 0.5) }', []),
    ('Rectangle { color: Qt.lighter(colors.surface, 1.2) }', ["lighter-darker"]),
    ('Rectangle { color: Qt.darker(colors.red) }', ["lighter-darker"]),
    ('Rectangle { color: colors.textPrimary }', []),
    ('// color: "#ff0000" in a comment\nItem {}', []),
    ('/* Qt.darker(x) */ Item {}', []),
    ('Text { text: "http://host/configurator"; color: colors.textPrimary }', []),
    ('Text { text: "#1 on the list" }', []),
]


def self_test():
    failures = 0
    for snippet, expected in SELF_TEST:
        found = [rule for _, rule, _ in check_text(snippet)]
        if found != expected:
            print("self-test FAILED: %r raised %s, expected %s" % (snippet, found, expected))
            failures += 1
    patterns = ["src/qml/components/entities/*/*", "src/qml/settings/settings/Color.qml"]
    for rel, expected in [("src/qml/components/entities/light/Brightness.qml", True),
                          ("src/qml/components/entities/EntityList.qml", False),
                          ("src/qml/settings/settings/Color.qml", True),
                          ("src/qml/settings/settings/Wifi.qml", False)]:
        if allowed(rel, patterns) != expected:
            print("self-test FAILED: allow-list match of %s is %s" % (rel, not expected))
            failures += 1
    load_allow_list()
    print("design-check self-test: %d case(s), %d failure(s)" % (len(SELF_TEST) + 4, failures))
    return 1 if failures else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("paths", nargs="*", help="files or folders to check (default: src/qml)")
    parser.add_argument("--self-test", action="store_true", help="check the rules against seeded violations")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    return run(args.paths or [os.path.join(ROOT, "src", "qml")])


if __name__ == "__main__":
    sys.exit(main())
