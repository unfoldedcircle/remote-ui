#!/bin/bash
# Patches Qt 5.15.2 sources so they compile with GCC >= 11 (Debian 13 ships GCC 14).
# QTBUG-90395: missing '#include <limits>' (fixed upstream in 5.15.3).
# Idempotent: re-running does nothing if a file is already patched.
set -euo pipefail
SRC="${1:-$HOME/Qt/5.15.2/Src}"

add_include() {   # add_include <file> <anchor-regex>   -> inserts '#include <limits>' after the first anchor line
    local f="$SRC/$1" anchor="$2"
    if grep -q '^#include <limits>' "$f"; then echo "already patched: $1"; return; fi
    sed -i "0,/$anchor/s//&\n#include <limits>/" "$f"
    grep -q '^#include <limits>' "$f" || { echo "PATCH FAILED: $1"; exit 1; }
    echo "patched: $1"
}

add_include qtbase/src/corelib/text/qbytearraymatcher.h        '^#include <QtCore\/qbytearray.h>'
add_include qtbase/src/corelib/tools/qoffsetstringarray_p.h    '^#include <tuple>'
add_include qtbase/src/corelib/global/qendian.h                '^#include <QtCore\/qglobal.h>'
add_include qtbase/src/corelib/global/qfloat16.h               '^#include <QtCore\/qglobal.h>'
add_include qtdeclarative/src/3rdparty/masm/yarr/Yarr.h        '^#include <limits.h>'
add_include qtdeclarative/src/qmldebug/qqmlprofilerevent_p.h   '^#include <QtCore\/qstring.h>'
