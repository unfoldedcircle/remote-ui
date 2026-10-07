#! /bin/bash
# Checks the QML sources against the rules of the design system (docs/design-system.md section 8): no
# text below 22 px, only colour tokens, no Qt.lighter / Qt.darker. The allow-list with its reasons is
# tools/design-check-allow.txt. Pass files or folders to check only those.
exec python3 "$(dirname "$0")/tools/design-check.py" "$@"
