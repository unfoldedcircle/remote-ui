# Task runner for the remote-ui desktop builds: `make` without a target prints the available targets.
# This is not the build system. The targets run qmake and make in a shadow build directory, exactly like
# Qt Creator and the GitHub workflow do.
#
# Attention: qmake writes a `Makefile` into its working directory. Never run qmake in the repository root,
# it would overwrite this file. Always use a build directory (all targets below do).

SHELL := bash
MAKEFLAGS += --no-print-directory
.DEFAULT_GOAL := help

# Overridable on the command line, e.g. `make linux QT_VERSION=5.15.2` or `make linux-static QTDIR_STATIC=/opt/qt-static`,
# or from the environment: `. scripts/env/qt-version.sh [version]` exports QTDIR and QT_VERSION for the shell, and
# QTDIR_STATIC as well when it selected a static Qt.
# QT_VERSION defaults to the version of an exported QTDIR, else to the newest Qt in ~/Qt (docs/install.md).
qt_version_of  = $(filter 5.%,$(notdir $(patsubst %/,%,$(dir $(1)))))
# The Qt directory name inside ~/Qt/<version>/ follows aqtinstall: gcc_64 on Linux, clang_64 on macOS.
UNAME_S       := $(shell uname -s)
QT_SPEC       := $(if $(filter Darwin,$(UNAME_S)),clang_64,gcc_64)
QTDIR_ENV     := $(QTDIR)
QT_VERSION   ?= $(or $(call qt_version_of,$(QTDIR_ENV)), \
                     $(call qt_version_of,$(lastword $(shell ls -d "$(HOME)"/Qt/5.*/$(QT_SPEC)* 2>/dev/null | sort -V))),5.15.19)
ifeq ($(origin QT_VERSION),command line)   # `make linux QT_VERSION=x` beats an exported QTDIR
QTDIR         = $(HOME)/Qt/$(QT_VERSION)/$(QT_SPEC)
endif
QTDIR        ?= $(HOME)/Qt/$(QT_VERSION)/$(QT_SPEC)
QTDIR_STATIC ?= $(HOME)/Qt/$(QT_VERSION)/$(QT_SPEC)-static
JOBS         ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu)
# Docker toolchain images (docs/cross-compile.md, docs/static-compile.md). `docker pull <image>` to update one.
TOOLCHAIN_IMAGE ?= unfoldedcircle/r2-toolchain-qt-5.15.19-static:latest
DESKTOP_IMAGE   ?= unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-x64:latest
WINDOWS_IMAGE   ?= unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-windows-x64:latest

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ARCH := $(shell uname -m)
# Output directory suffix of the macOS builds: x64 or arm64
MACOS_ARCH := $(if $(filter arm64,$(ARCH)),arm64,x64)

##@ Build

linux: QT = $(QTDIR)
linux: LINK = shared
linux: BUILD_DIR = $(ROOT)/build
linux: OUT_DIR = $(ROOT)/binaries/Linux-x64
linux: QMAKE_ARGS = CONFIG+=release
linux: DOC = docs/install.md (docs/install-debian-13.md, docs/install-ubuntu-22.04.md)
linux: ENV_FILE = scripts/env/linux.sh
linux: ## Build the simulator with the dynamic Qt -> binaries/Linux-x64/remote-ui
	$(build)

linux-static: QT = $(QTDIR_STATIC)
linux-static: LINK = static
linux-static: BUILD_DIR = $(ROOT)/build-static
linux-static: OUT_DIR = $(ROOT)/binaries/Linux-x64-static
linux-static: QMAKE_ARGS = CONFIG+=release CONFIG+=static
linux-static: DOC = docs/static-compile-debian-13.md
linux-static: ENV_FILE = scripts/env/linux-static.sh
linux-static: ## Build the self-contained simulator with the static Qt -> binaries/Linux-x64-static/remote-ui
	$(build)

macos: QT = $(QTDIR)
macos: LINK = shared
macos: BUILD_DIR = $(ROOT)/build
macos: OUT_DIR = $(ROOT)/binaries/macOS-$(MACOS_ARCH)
macos: QMAKE_ARGS = CONFIG+=release
macos: DOC = docs/static-compile-macos.md
macos: ENV_FILE = scripts/env/macos.sh
macos: BIN = Remote UI.app/Contents/MacOS/Remote UI
macos: ## Build the macOS simulator with the dynamic Qt -> binaries/macOS-<arch>/Remote UI.app
	$(build)

macos-static: QT = $(QTDIR_STATIC)
macos-static: LINK = static
macos-static: BUILD_DIR = $(ROOT)/build-static
macos-static: OUT_DIR = $(ROOT)/binaries/macOS-$(MACOS_ARCH)-static
macos-static: QMAKE_ARGS = CONFIG+=release CONFIG+=static
macos-static: DOC = docs/static-compile-macos.md
macos-static: ENV_FILE = scripts/env/macos.sh
macos-static: BIN = Remote UI.app/Contents/MacOS/Remote UI
macos-static: ## Build the self-contained macOS simulator with the static Qt -> binaries/macOS-<arch>-static/Remote UI.app
	$(build)

ucr2: IMAGE = $(TOOLCHAIN_IMAGE)
ucr2: OUT_DIR = $(ROOT)/binaries/linux-arm64/release
ucr2: ## Cross-compile the static Remote Two/3 (aarch64) binary in the Docker toolchain -> binaries/linux-arm64/release/remote-ui
	$(docker_build)
	@echo "Install it on the device as described in docs/cross-compile.md"

linux-x64: IMAGE = $(DESKTOP_IMAGE)
linux-x64: OUT_DIR = $(ROOT)/binaries/linux-x64/release
linux-x64: ## Build the static simulator in the Docker toolchain image (no Qt needed) -> binaries/linux-x64/release/remote-ui
	$(docker_build)
	@echo "Run it with:  make run-linux-x64   or:  . scripts/env/linux-static.sh && binaries/linux-x64/release/remote-ui"

windows-x64: IMAGE = $(WINDOWS_IMAGE)
windows-x64: OUT_DIR = $(ROOT)/binaries/windows-x64/release
windows-x64: BIN = remote-ui.exe
windows-x64: ## Cross-compile the experimental static Windows simulator in the Docker toolchain image -> binaries/windows-x64/release/remote-ui.exe
	$(docker_build)
	@echo "Copy remote-ui.exe to a Windows machine and start it with scripts\\env\\windows.cmd, see docs/static-compile-windows.md"

test: ## Build and run the unit tests (CMake, dynamic Qt)
	@grep -qE '^CONFIG \+=.*\bshared\b' "$(QTDIR)/mkspecs/qconfig.pri" 2>/dev/null || { \
	    echo "error: $(QTDIR) is not a shared Qt build (see CONFIG in mkspecs/qconfig.pri)."; \
	    echo "       Set QTDIR=<path> or install Qt as described in docs/install.md"; exit 1; }
	mkdir -p "$(ROOT)/test/build"
	@grep -qs 'Qt5_DIR:PATH=$(QTDIR)/' "$(ROOT)/test/build/CMakeCache.txt" || rm -rf "$(ROOT)/test/build"/*
	cd "$(ROOT)/test/build" && cmake -D CMAKE_PREFIX_PATH="$(QTDIR)" .. && cmake --build . -j$(JOBS)
	cd "$(ROOT)/test/build" && QT_QPA_PLATFORM=offscreen ctest --output-on-failure

##@ Run

run-linux: ## Start the dynamic build with scripts/env/linux.sh
	@test -x "$(ROOT)/binaries/Linux-x64/remote-ui" || { echo "No binary yet, run: make linux"; exit 1; }
	@cd "$(ROOT)" && export QTDIR="$(QTDIR)" && . scripts/env/linux.sh && exec binaries/Linux-x64/remote-ui

run-linux-static: ## Start the static build with scripts/env/linux-static.sh
	@test -x "$(ROOT)/binaries/Linux-x64-static/remote-ui" || { echo "No binary yet, run: make linux-static"; exit 1; }
	@cd "$(ROOT)" && . scripts/env/linux-static.sh && exec binaries/Linux-x64-static/remote-ui

run-linux-x64: ## Start the Docker-built static build with scripts/env/linux-static.sh
	@test -x "$(ROOT)/binaries/linux-x64/release/remote-ui" || { echo "No binary yet, run: make linux-x64"; exit 1; }
	@cd "$(ROOT)" && . scripts/env/linux-static.sh && exec binaries/linux-x64/release/remote-ui

run-macos: ## Start the dynamic macOS build with scripts/env/macos.sh
	@test -x "$(ROOT)/binaries/macOS-$(MACOS_ARCH)/Remote UI.app/Contents/MacOS/Remote UI" || { echo "No app yet, run: make macos"; exit 1; }
	@cd "$(ROOT)" && . scripts/env/macos.sh && exec "binaries/macOS-$(MACOS_ARCH)/Remote UI.app/Contents/MacOS/Remote UI"

run-macos-static: ## Start the static macOS build with scripts/env/macos.sh
	@test -x "$(ROOT)/binaries/macOS-$(MACOS_ARCH)-static/Remote UI.app/Contents/MacOS/Remote UI" || { echo "No app yet, run: make macos-static"; exit 1; }
	@cd "$(ROOT)" && . scripts/env/macos.sh && exec "binaries/macOS-$(MACOS_ARCH)-static/Remote UI.app/Contents/MacOS/Remote UI"

##@ Clean

clean: ## Remove the dynamic build (build/, intermediate files, binaries/Linux-x64/)
	rm -rf "$(ROOT)/build/linux-$(ARCH)/release" "$(ROOT)/binaries/Linux-x64" \
	       "$(ROOT)/build/Makefile" "$(ROOT)/build/.qmake.stash" "$(ROOT)/build/version.txt" "$(ROOT)/build/.clean-ts"

clean-static: ## Remove the static build (build-static/, intermediate files, binaries/Linux-x64-static/)
	rm -rf "$(ROOT)/build/linux-$(ARCH)/release-static" "$(ROOT)/binaries/Linux-x64-static" "$(ROOT)/build-static"

clean-macos: ## Remove the dynamic macOS build (build/, intermediate files, binaries/macOS-<arch>/)
	rm -rf "$(ROOT)/build/osx-$(ARCH)/release" "$(ROOT)/binaries/macOS-$(MACOS_ARCH)" \
	       "$(ROOT)/build/Makefile" "$(ROOT)/build/.qmake.stash" "$(ROOT)/build/version.txt" "$(ROOT)/build/.clean-ts"

clean-macos-static: ## Remove the static macOS build (build-static/, intermediate files, binaries/macOS-<arch>-static/)
	rm -rf "$(ROOT)/build/osx-$(ARCH)/release-static" "$(ROOT)/binaries/macOS-$(MACOS_ARCH)-static" "$(ROOT)/build-static"

clean-ucr2: ## Remove the Remote Two/3 cross-compile build (intermediate files, binaries/linux-arm64/)
	rm -rf "$(ROOT)/build/linux-arm64" "$(ROOT)/binaries/linux-arm64" "$(ROOT)/build/.clean-ts-ucr2"

clean-linux-x64: ## Remove the Docker-built static desktop build (intermediate files, binaries/linux-x64/)
	rm -rf "$(ROOT)/build/linux-$(ARCH)/release-static" "$(ROOT)/binaries/linux-x64" "$(ROOT)/build/.clean-ts-linux-x64"

clean-windows-x64: ## Remove the Windows cross-compile build (intermediate files, binaries/windows-x64/)
	rm -rf "$(ROOT)/build/windows-x86_64" "$(ROOT)/binaries/windows-x64" "$(ROOT)/build/.clean-ts-windows-x64"

clean-all: ## Remove every build, test build and output directory
	rm -rf "$(ROOT)/build" "$(ROOT)/build-static" "$(ROOT)/binaries" "$(ROOT)/test/build"

translations-restore: ## Discard the lupdate changes qmake makes to resources/translations/*.ts (all of them!)
	cd "$(ROOT)" && git checkout -- 'resources/translations/*.ts'

##@ Help

help: ## Show this help
	@awk 'BEGIN { FS = ":.*## "; printf "Usage: make <target> [VARIABLE=value]\n" } \
	      /^##@/ { printf "\n%s\n", substr($$0, 5) } \
	      /^[a-zA-Z0-9_-]+:.*## / { printf "  %-22s %s\n", $$1, $$2 }' $(MAKEFILE_LIST)
	@printf "\nVariables:\n  %-22s %s\n  %-22s %s\n  %-22s %s\n  %-22s %s\n  %-22s %s\n  %-22s %s\n  %-22s %s\n" \
	        "QT_VERSION" "$(QT_VERSION)" "QTDIR" "$(QTDIR)" "QTDIR_STATIC" "$(QTDIR_STATIC)" "JOBS" "$(JOBS)" \
	        "TOOLCHAIN_IMAGE" "$(TOOLCHAIN_IMAGE)" "DESKTOP_IMAGE" "$(DESKTOP_IMAGE)" "WINDOWS_IMAGE" "$(WINDOWS_IMAGE)"
	@echo "  QTDIR is ignored by linux-static on purpose: it usually points to the dynamic Qt (docs/install.md)."

# qmake runs lupdate, which rewrites every resources/translations/*.ts. ts_snapshot remembers in file $(1) which of
# them are unmodified before qmake runs, ts_restore reverts exactly those afterwards, so intentional local edits and
# files that were already modified before the build are kept.
define ts_snapshot
	@cd "$(ROOT)" && for f in $$(git ls-files 'resources/translations/*.ts'); do \
	    git diff --quiet -- "$$f" && echo "$$f"; done > "$(1)"; true
endef
define ts_restore
	@cd "$(ROOT)" && xargs -r git checkout --quiet -- < "$(1)"
endef

# Docker toolchain build recipe (ucr2, linux-x64, windows-x64). IMAGE, OUT_DIR and BIN are set per target above. The
# image runs qmake and make on the bind-mounted repository and writes the binary to OUT_DIR (binaries/<platform>/release,
# the path the GitHub workflow tars). The intermediate files go to build/<platform>/release-static/ in the repository.
BIN = remote-ui
define docker_build
	@docker image inspect "$(IMAGE)" >/dev/null 2>&1 || docker pull "$(IMAGE)"
	git -C "$(ROOT)" submodule update --init --recursive
	mkdir -p "$(ROOT)/build"
	$(call ts_snapshot,$(ROOT)/build/.clean-ts-$@)
	docker run --rm --user=$$(id -u):$$(id -g) -v "$(ROOT)":/sources "$(IMAGE)"
	cd "$(ROOT)" && git describe --match "v[0-9]*" --tags HEAD --always > "$(OUT_DIR)/version.txt"
	$(call ts_restore,$(ROOT)/build/.clean-ts-$@)
	@echo; echo "Build finished: $(OUT_DIR)/$(BIN) ($$(cat "$(OUT_DIR)/version.txt"))"
endef

# Common build recipe. Variables QT, LINK, BUILD_DIR, OUT_DIR, QMAKE_ARGS, DOC and ENV_FILE are set per target above.
define build
	@grep -qE '^CONFIG \+=.*\b$(LINK)\b' "$(QT)/mkspecs/qconfig.pri" 2>/dev/null || { \
	    echo "error: $(QT) is not a $(LINK) Qt build (see CONFIG in mkspecs/qconfig.pri)."; \
	    echo "       Set $(if $(filter static,$(LINK)),QTDIR_STATIC,QTDIR)=<path> or install Qt as described in $(DOC)"; exit 1; }
	@for t in qmake lupdate lrelease; do test -x "$(QT)/bin/$$t" || { \
	    echo "error: $(QT)/bin/$$t not found, see $(DOC)"; exit 1; }; done
	git -C "$(ROOT)" submodule update --init --recursive
	mkdir -p "$(BUILD_DIR)" "$(OUT_DIR)"
	$(call ts_snapshot,$(BUILD_DIR)/.clean-ts)
	@echo "Qt: $(QT)  ->  $(OUT_DIR)  ($(QMAKE_ARGS), -j$(JOBS))"
	cd "$(BUILD_DIR)" && PATH="$(QT)/bin:$$PATH" UC_BIN="$(OUT_DIR)" qmake "$(ROOT)/remote-ui.pro" $(QMAKE_ARGS)
	$(MAKE) -C "$(BUILD_DIR)" -j$(JOBS)
	cp "$(BUILD_DIR)/version.txt" "$(OUT_DIR)/"
	$(call ts_restore,$(BUILD_DIR)/.clean-ts)
	@echo; echo "Build finished: $(OUT_DIR)/$(BIN) ($$(cat "$(OUT_DIR)/version.txt"))"
	@echo "Run it with:  make run-$@   or:  . $(ENV_FILE) && \"$(subst $(ROOT)/,,$(OUT_DIR))/$(BIN)\""
endef

.PHONY: help linux linux-static macos macos-static linux-x64 windows-x64 ucr2 test run-linux run-linux-static run-macos run-macos-static run-linux-x64 clean clean-static clean-macos clean-macos-static clean-ucr2 clean-linux-x64 clean-windows-x64 clean-all translations-restore
