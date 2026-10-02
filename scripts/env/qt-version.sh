# Select the Qt installation for the current shell (build environment for `make`, cmake and Qt Creator).
# Source it (bash or zsh), don't execute it:
#   . scripts/env/qt-version.sh            # newest ~/Qt/5.*/gcc_64 (Linux) or clang_64 (macOS)
#   . scripts/env/qt-version.sh 5.15.19    # ~/Qt/5.15.19/gcc_64 (Linux) or clang_64 (macOS)
#   . scripts/env/qt-version.sh 5.15.2     # ~/Qt/5.15.2/gcc_64
#   . scripts/env/qt-version.sh /opt/qt    # any Qt prefix (must contain bin/qmake)
# A shared Qt has priority. Without one the first form takes the newest ~/Qt/5.*/gcc_64-static or clang_64-static and
# the version form the -static directory of that version (the macOS guide installs only the static Qt).
# Sets QTDIR, QT_ROOT_DIR, QT_VERSION, Qt5_DIR, CMAKE_PREFIX_PATH and PATH, and removes the entries of a previously
# selected Qt from PATH, CMAKE_PREFIX_PATH, LD_LIBRARY_PATH and QT_PLUGIN_PATH first, so it can be sourced repeatedly.
# QT_ROOT overrides ~/Qt. The Makefile picks up QTDIR and QT_VERSION (QTDIR_STATIC follows QT_VERSION). A static Qt
# is exported as QTDIR_STATIC as well, the variable of `make linux-static` / `make macos-static`; `make linux`,
# `make macos` and `make test` refuse it in QTDIR, they need a shared Qt.
# Docs: docs/install.md, docs/install-debian-13.md, docs/static-compile-macos.md

_qt_root="${QT_ROOT:-$HOME/Qt}"
_qt_where=
_qt_dir=
_qt_fallback=

# _qt_newest SUFFIX: the newest $_qt_root/5.*/gcc_64SUFFIX or clang_64SUFFIX. find instead of a glob: zsh refuses to
# run a command whose pattern matches nothing, and one of gcc_64 and clang_64 is always missing.
_qt_newest() {
    find -L "$_qt_root" -mindepth 2 -maxdepth 2 2>/dev/null | grep -E "/5\.[^/]*/(gcc|clang)_64$1\$" |
        sort -V | tail -n 1
}
# _qt_usable DIR: DIR is set and contains bin/qmake (an empty DIR would test /bin/qmake)
_qt_usable() {
    [ -n "$1" ] && [ -x "$1/bin/qmake" ]
}

if [ -z "${1:-}" ]; then
    _qt_where="$_qt_root/5.*"
    _qt_dir=$(_qt_newest '')
    if ! _qt_usable "$_qt_dir"; then _qt_dir=$(_qt_newest -static); _qt_fallback=1; fi
elif [ -x "$1/bin/qmake" ]; then
    _qt_dir="$1"
else
    _qt_where="$_qt_root/$1"
    # clang_64 is the macOS name (aqtinstall, configure-qt-macos.sh), gcc_64 the Linux one; the static Qt comes last
    for _qt_spec in clang_64 gcc_64 clang_64-static gcc_64-static; do
        if _qt_usable "$_qt_where/$_qt_spec"; then _qt_dir="$_qt_where/$_qt_spec"; break; fi
    done
    case "$_qt_dir" in *-static) _qt_fallback=1 ;; esac
fi
if ! _qt_usable "$_qt_dir"; then
    echo "qt-version.sh: no Qt installation in $_qt_where/{gcc_64,clang_64}[-static] (see docs/install.md)" >&2
    unset _qt_root _qt_where _qt_dir _qt_fallback _qt_spec
    unset -f _qt_newest _qt_usable
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
# A QTDIR_STATIC equal to QTDIR was exported here along with a static Qt: it goes with that selection
if [ -n "${QTDIR_STATIC:-}" ] && [ "$QTDIR_STATIC" = "${QTDIR:-}" ]; then unset QTDIR_STATIC; fi

export QTDIR="$_qt_dir"
export QT_ROOT_DIR="$QTDIR"            # variable name used by the GitHub workflow
export QT_VERSION="$("$QTDIR/bin/qmake" -query QT_VERSION)"
export Qt5_DIR="$QTDIR/lib/cmake/Qt5"
export PATH="$QTDIR/bin:$PATH"
export CMAKE_PREFIX_PATH="$QTDIR${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
# Same test as the Makefile: the static targets build with QTDIR_STATIC, the others refuse a static QTDIR
if grep -qE '^CONFIG \+=.*\bstatic\b' "$QTDIR/mkspecs/qconfig.pri" 2>/dev/null; then
    export QTDIR_STATIC="$QTDIR"
    case "$(uname -s)" in Darwin) _qt_os=macos ;; *) _qt_os=linux ;; esac
    echo "Qt $QT_VERSION (static): $QTDIR" >&2
    if [ -n "$_qt_fallback" ]; then
        echo "qt-version.sh: no shared Qt in $_qt_where, took the static one, also exported as QTDIR_STATIC" >&2
    else
        echo "qt-version.sh: a static Qt, also exported as QTDIR_STATIC" >&2
    fi
    echo "qt-version.sh: make $_qt_os-static builds with it; make $_qt_os and make test need a shared Qt" >&2
else
    echo "Qt $QT_VERSION: $QTDIR" >&2
fi
unset _qt_root _qt_where _qt_dir _qt_fallback _qt_spec _qt_os _qt_old _qt_new
unset -f _qt_newest _qt_usable _qt_strip
