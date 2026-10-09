#!/usr/bin/env python3
# Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Checks the pinned version of the OpenSpec CLI. See openspec/README.md.

Every command in the repository runs the CLI as `npx <package>@<version>` with the same version, so a
new OpenSpec release changes nothing until it is adopted on purpose:

  (no option)        every command names the same version, and none uses a tag such as `latest`
  --latest           also fail when the npm registry has a newer release than the pinned one
  --set <version>    change the version in every command (when adopting a release)

Archived OpenSpec changes are frozen history and are not checked. In GitHub Actions a failure is
also written as an error annotation and to the job summary.
"""
import argparse
import json
import os
import re
import subprocess
import sys
import urllib.request

PACKAGE = '@fission-ai/openspec'
REGISTRY = 'https://registry.npmjs.org/@fission-ai%2Fopenspec/latest'
RELEASES = 'https://github.com/Fission-AI/OpenSpec/releases'
EXCLUDE = ['openspec/changes/archive']
PIN = re.compile(re.escape(PACKAGE) + r'@([0-9A-Za-z.\-]+)')
VERSION = re.compile(r'^\d+\.\d+\.\d+$')


def occurrences():
    """(file, line number, version) of every pinned command in the tracked files."""
    pathspecs = ['--', '.'] + [f':!{p}' for p in EXCLUDE]
    r = subprocess.run(['git', 'grep', '-n', '-I', '-E', re.escape(PACKAGE) + '@'] + pathspecs,
                       capture_output=True, text=True)
    found = []
    for line in r.stdout.splitlines():
        path, number, text = line.split(':', 2)
        for m in PIN.finditer(text):
            found.append((path, int(number), m.group(1)))
    return found


def fail(title, text):
    print(f'ERROR: {title}\n\n{text}', file=sys.stderr)
    if os.environ.get('GITHUB_ACTIONS') == 'true':
        print(f'::error title={title}::{text.splitlines()[0]}')
        summary = os.environ.get('GITHUB_STEP_SUMMARY')
        if summary:
            with open(summary, 'a') as f:
                f.write(f'## {title}\n\n{text}\n')
    sys.exit(1)


def pinned_version(found):
    if not found:
        fail('OpenSpec version not found',
             f'No command in the repository names `{PACKAGE}@<version>`. The OpenSpec commands in\n'
             f'`openspec/README.md` are expected to name the pinned version.')
    versions = sorted({v for _, _, v in found})
    bad = [(p, n, v) for p, n, v in found if not VERSION.match(v)]
    if len(versions) > 1 or bad:
        lines = '\n'.join(f'- `{p}:{n}` names `{v}`' for p, n, v in found)
        most = max(versions, key=lambda v: sum(1 for _, _, x in found if x == v))
        fail('OpenSpec version differs between commands',
             f'Every command must name the same OpenSpec version, as a number such as `1.14.1`, never\n'
             f'`latest`, so a new release changes nothing until it is adopted on purpose. Found:\n\n'
             f'{lines}\n\n'
             f'To fix it, set one version everywhere, for example the one most commands use:\n\n'
             f'    python3 tools/openspec-version.py --set {most if VERSION.match(most) else "<version>"}\n')
    return versions[0]


def as_tuple(version):
    return tuple(int(x) for x in version.split('.'))


def check_latest(pinned):
    with urllib.request.urlopen(REGISTRY, timeout=30) as response:
        latest = json.load(response)['version']
    if as_tuple(latest) <= as_tuple(pinned):
        print(f'OpenSpec {pinned} is pinned and is the latest release.')
        return
    fail(f'OpenSpec {latest} is released, this repository uses {pinned}',
         f'A newer OpenSpec CLI is available. Nothing changes until it is adopted on purpose; to do so:\n\n'
         f'1. Read the release notes from {pinned} to {latest}: {RELEASES}\n'
         f'   Look for changes to the built-in `spec-driven` schema (its instructions and templates)\n'
         f'   and to `openspec validate`.\n'
         f'2. Bring the workflow schema copy in `openspec/schemas/spec-driven-with-adr/` up to date with\n'
         f'   the built-in schema of {latest}, as `openspec/README.md` describes in "Keeping the\n'
         f'   workflow schema in step with OpenSpec".\n'
         f'3. Change the version in every command:\n\n'
         f'       python3 tools/openspec-version.py --set {latest}\n\n'
         f'4. Check the result: `python3 tools/openspec-version.py`, then with the new version\n'
         f'   `npx {PACKAGE}@{latest} schema validate spec-driven-with-adr`,\n'
         f'   `validate --changes --strict` and `validate --specs`.\n'
         f'5. Regenerate your local `/opsx:*` commands with `npx {PACKAGE}@{latest} init --tools <tool>`\n'
         f'   and open a pull request with the changes.\n\n'
         f'This check runs once a week and fails until the release is adopted.\n')


def set_version(version):
    if not VERSION.match(version):
        sys.exit(f'Not a version number: {version}')
    changed = set()
    for path, _, old in occurrences():
        if old == version or path in changed:
            continue
        text = open(path, encoding='utf-8').read()
        open(path, 'w', encoding='utf-8').write(PIN.sub(f'{PACKAGE}@{version}', text))
        changed.add(path)
    for path in sorted(changed):
        print(f'{path}: {PACKAGE}@{version}')
    if not changed:
        print(f'Every command already names {version}.')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--latest', action='store_true', help='also compare with the latest release on npm')
    parser.add_argument('--set', metavar='VERSION', help='change the version in every command')
    args = parser.parse_args()
    os.chdir(subprocess.run(['git', 'rev-parse', '--show-toplevel'], capture_output=True, text=True,
                            check=True).stdout.strip())
    if args.set:
        set_version(args.set)
        return
    pinned = pinned_version(occurrences())
    print(f'Every command names OpenSpec {pinned}.', flush=True)
    if args.latest:
        check_latest(pinned)


if __name__ == '__main__':
    main()
