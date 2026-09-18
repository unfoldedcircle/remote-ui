# Select the Qt installation for the current shell (build environment for `make`, cmake and Qt Creator).
# Source it, don't execute it:
#   . scripts/env/qt-version.sh            # newest ~/Qt/5.*/gcc_64
#   . scripts/env/qt-version.sh 5.15.19    # ~/Qt/5.15.19/gcc_64
#   . scripts/env/qt-version.sh 5.15.2     # ~/Qt/5.15.2/gcc_64
#   . scripts/env/qt-version.sh /opt/qt    # any Qt prefix (must contain bin/qmake)
# Sets QTDIR, QT_ROOT_DIR, QT_VERSION, Qt5_DIR, CMAKE_PREFIX_PATH and PATH, and removes the entries of a previously
# selected Qt from PATH, CMAKE_PREFIX_PATH, LD_LIBRARY_PATH and QT_PLUGIN_PATH first, so it can be sourced repeatedly.
# QT_ROOT overrides ~/Qt. The Makefile picks up QTDIR and QT_VERSION (QTDIR_STATIC follows QT_VERSION).
# Docs: docs/install.md, docs/install-debian-13.md

_qt_root="${QT_ROOT:-$HOME/Qt}"
if [ -z "${1:-}" ]; then
    _qt_dir=$(ls -d "$_qt_root"/5.*/gcc_64 2>/dev/null | sort -V | tail -n 1)
elif [ -x "$1/bin/qmake" ]; then
    _qt_dir="$1"
else
    _qt_dir="$_qt_root/$1/gcc_64"
fi
if [ ! -x "$_qt_dir/bin/qmake" ]; then
    echo "qt-version.sh: no Qt installation in ${_qt_dir:-$_qt_root/5.*/gcc_64} (see docs/install.md)" >&2
    unset _qt_root _qt_dir
    return 1 2>/dev/null || exit 1
fi

# _qt_strip VAR: remove every ~/Qt/<version>/<any>/{bin,lib,plugins} entry from the colon separated list in VAR
_qt_strip() {
    eval "_qt_old=\"\${$1:-}\""
    _qt_new=$(printf '%s' "$_qt_old" | tr ':' '\n' | grep -v -E "^$_qt_root/[^/]+/[^/]+(/(bin|lib|plugins|lib/cmake/Qt5))?$" | paste -sd: -)
    if [ -n "$_qt_new" ]; then export "$1=$_qt_new"; else unset "$1"; fi
}
_qt_strip PATH
_qt_strip CMAKE_PREFIX_PATH
_qt_strip LD_LIBRARY_PATH
_qt_strip QT_PLUGIN_PATH

export QTDIR="$_qt_dir"
export QT_ROOT_DIR="$QTDIR"            # variable name used by the GitHub workflow
export QT_VERSION="$("$QTDIR/bin/qmake" -query QT_VERSION)"
export Qt5_DIR="$QTDIR/lib/cmake/Qt5"
export PATH="$QTDIR/bin:$PATH"
export CMAKE_PREFIX_PATH="$QTDIR${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
echo "Qt $QT_VERSION: $QTDIR" >&2
unset _qt_root _qt_dir _qt_old _qt_new
unset -f _qt_strip
