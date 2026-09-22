#!/usr/bin/env bash
# Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Rebuilds resources/icons/icon-font.ttf. See docs/icon-font.md.
#
# The repository ships the Font Awesome Free edition of the icon font, which is what a
# normal build embeds — nothing has to be run for that. This script is needed in two
# cases only:
#
#   Firmware build: overlay the licensed Pro edition in a disposable checkout.
#       tools/icon-font.sh --pro                      # download, needs FONTAWESOME_NPM_AUTH_TOKEN
#       tools/icon-font.sh --pro /ci-cache/fa-pro     # use a cached, extracted package
#
#   Maintenance: re-vendor the Free edition, e.g. after a Font Awesome upgrade.
#       tools/icon-font.sh --free                     # download from the public registry
#       tools/icon-font.sh --free /ci-cache/fa-free   # use a cached, extracted package
#
# !! The Pro font is commercially licensed and must NEVER be committed. !!
# Run it in a disposable checkout, or restore afterwards with:
#   git checkout -- resources/icons/icon-font.ttf resources/icons/icon-font.json
set -euo pipefail
cd "$(dirname "$0")/.."

# Keep in step with the icon code points in resources/icons/icon-mapping.json.
FA_VERSION="6.5.1"
# Style embedded per edition. Free has no Light weight, so the public build uses Solid.
FREE_STYLE="fa-solid-900.ttf"
PRO_STYLE="fa-light-300.ttf"
FREE_PACKAGE="@fortawesome/fontawesome-free@${FA_VERSION}"
PRO_PACKAGE="@fortawesome/fontawesome-pro@${FA_VERSION}"
PRO_REGISTRY="https://npm.fontawesome.com/"

EDITION=""
PACKAGE_DIR=""

usage() {
  echo "usage: $0 --free|--pro [<extracted-package-dir>]" >&2
  echo "  <extracted-package-dir>: a Font Awesome npm package with a webfonts/ directory" >&2
  echo "  without it the package is downloaded; --pro then needs FONTAWESOME_NPM_AUTH_TOKEN" >&2
  exit 1
}

for arg in "$@"; do
  case "$arg" in
    --free|--pro)
      [ -n "$EDITION" ] && usage
      EDITION="${arg#--}"
      ;;
    -h|--help) usage ;;
    -*) echo "error: unknown option '$arg'" >&2; usage ;;
    *)
      [ -n "$PACKAGE_DIR" ] && usage
      PACKAGE_DIR="$arg"
      ;;
  esac
done
[ -n "$EDITION" ] || usage

if [ "$EDITION" = "pro" ]; then
  PACKAGE="$PRO_PACKAGE"; STYLE="$PRO_STYLE"
else
  PACKAGE="$FREE_PACKAGE"; STYLE="$FREE_STYLE"
fi

python3 -c "import fontTools" 2>/dev/null || {
  echo "error: fontTools is required: pip install fonttools" >&2
  exit 1
}

# Validate the requested mode before downloading anything.
if [ -n "$PACKAGE_DIR" ]; then
  [ -f "$PACKAGE_DIR/webfonts/$STYLE" ] || {
    echo "error: $PACKAGE_DIR/webfonts/$STYLE not found" >&2
    echo "       expected an extracted $PACKAGE package" >&2
    exit 1
  }
  VERSION="$(python3 -c "import json,sys; print(json.load(open('$PACKAGE_DIR/package.json'))['version'])" 2>/dev/null || echo unknown)"
  [ "$VERSION" = "$FA_VERSION" ] || {
    echo "error: $PACKAGE_DIR is Font Awesome $VERSION, expected $FA_VERSION" >&2
    echo "       a different release renumbers icons; update FA_VERSION and the icon mapping together" >&2
    exit 1
  }
  SOURCE="$PACKAGE_DIR/webfonts/$STYLE"
else
  command -v npm >/dev/null || { echo "error: npm is required to download the package" >&2; exit 1; }
  if [ "$EDITION" = "pro" ]; then
    if [ -z "${FONTAWESOME_NPM_AUTH_TOKEN:-}" ] && ! npm config get "@fortawesome:registry" | grep -q fontawesome; then
      echo "error: no Pro package directory given and no credentials available" >&2
      echo "       set FONTAWESOME_NPM_AUTH_TOKEN, or configure the @fortawesome registry:" >&2
      echo "         npm config set \"@fortawesome:registry\" $PRO_REGISTRY" >&2
      echo "         npm config set \"//npm.fontawesome.com/:_authToken\" <token>" >&2
      exit 1
    fi
  fi

  WORK="$(mktemp -d)"
  trap 'rm -rf "$WORK"' EXIT
  if [ "$EDITION" = "pro" ] && [ -n "${FONTAWESOME_NPM_AUTH_TOKEN:-}" ]; then
    # Ephemeral credentials: never write the token into a persistent .npmrc.
    {
      echo "@fortawesome:registry=$PRO_REGISTRY"
      echo "//npm.fontawesome.com/:_authToken=$FONTAWESOME_NPM_AUTH_TOKEN"
    } > "$WORK/.npmrc"
    export NPM_CONFIG_USERCONFIG="$WORK/.npmrc"
  fi
  echo "[*] downloading $PACKAGE"
  ( cd "$WORK" && npm pack --silent "$PACKAGE" >/dev/null && tar -xzf ./*.tgz )
  SOURCE="$WORK/package/webfonts/$STYLE"
  [ -f "$SOURCE" ] || { echo "error: $STYLE not found in $PACKAGE" >&2; exit 1; }
fi

echo "[*] building the icon font from $(basename "$SOURCE") ($EDITION edition)"
python3 tools/icon-font.py build "$SOURCE" --edition "$EDITION"

if [ "$EDITION" = "free" ]; then
  python3 tools/icon-font.py check --require-free
else
  python3 tools/icon-font.py check
  echo "[!] Pro font in the working tree. Do not commit it:"
  echo "    git checkout -- resources/icons/icon-font.ttf resources/icons/icon-font.json"
fi
