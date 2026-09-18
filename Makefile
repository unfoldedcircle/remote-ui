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
# or from the environment: `. scripts/env/qt-version.sh [version]` exports QTDIR and QT_VERSION for the shell.
# QT_VERSION defaults to the version of an exported QTDIR, else to the newest Qt in ~/Qt (docs/install.md).
qt_version_of  = $(filter 5.%,$(notdir $(patsubst %/,%,$(dir $(1)))))
QTDIR_ENV     := $(QTDIR)
QT_VERSION   ?= $(or $(call qt_version_of,$(QTDIR_ENV)), \
                     $(call qt_version_of,$(lastword $(shell ls -d "$(HOME)"/Qt/5.*/gcc_64* 2>/dev/null | sort -V))),5.15.19)
ifeq ($(origin QT_VERSION),command line)   # `make linux QT_VERSION=x` beats an exported QTDIR
QTDIR         = $(HOME)/Qt/$(QT_VERSION)/gcc_64
endif
QTDIR        ?= $(HOME)/Qt/$(QT_VERSION)/gcc_64
QTDIR_STATIC ?= $(HOME)/Qt/$(QT_VERSION)/gcc_64-static
JOBS         ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu)
# Remote Two/3 cross-compile toolchain (docs/cross-compile.md). `docker pull $(TOOLCHAIN_IMAGE)` to update it.
TOOLCHAIN_IMAGE ?= unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ARCH := $(shell uname -m)

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

ucr2: ## Cross-compile the static Remote Two/3 (aarch64) binary in the Docker toolchain -> binaries/linux-arm64/release/remote-ui
	@docker image inspect "$(TOOLCHAIN_IMAGE)" >/dev/null 2>&1 || docker pull "$(TOOLCHAIN_IMAGE)"
	git -C "$(ROOT)" submodule update --init --recursive
	mkdir -p "$(ROOT)/build"
	$(call ts_snapshot,$(ROOT)/build/.clean-ts-ucr2)
	docker run --rm --user=$$(id -u):$$(id -g) -v "$(ROOT)":/sources "$(TOOLCHAIN_IMAGE)"
	cd "$(ROOT)" && git describe --match "v[0-9]*" --tags HEAD --always > binaries/linux-arm64/release/version.txt
	$(call ts_restore,$(ROOT)/build/.clean-ts-ucr2)
	@echo; echo "Build finished: $(ROOT)/binaries/linux-arm64/release/remote-ui ($$(cat "$(ROOT)/binaries/linux-arm64/release/version.txt"))"
	@echo "Install it on the device as described in docs/cross-compile.md"

test: ## Build and run the unit tests (CMake, dynamic Qt)
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

##@ Clean

clean: ## Remove the dynamic build (build/, intermediate files, binaries/Linux-x64/)
	rm -rf "$(ROOT)/build/linux-$(ARCH)/release" "$(ROOT)/binaries/Linux-x64" \
	       "$(ROOT)/build/Makefile" "$(ROOT)/build/.qmake.stash" "$(ROOT)/build/version.txt" "$(ROOT)/build/.clean-ts"

clean-static: ## Remove the static build (build-static/, intermediate files, binaries/Linux-x64-static/)
	rm -rf "$(ROOT)/build/linux-$(ARCH)/release-static" "$(ROOT)/binaries/Linux-x64-static" "$(ROOT)/build-static"

clean-ucr2: ## Remove the Remote Two/3 cross-compile build (intermediate files, binaries/linux-arm64/)
	rm -rf "$(ROOT)/build/linux-arm64" "$(ROOT)/binaries/linux-arm64" "$(ROOT)/build/.clean-ts-ucr2"

clean-all: ## Remove every build, test build and output directory
	rm -rf "$(ROOT)/build" "$(ROOT)/build-static" "$(ROOT)/binaries" "$(ROOT)/test/build"

translations-restore: ## Discard the lupdate changes qmake makes to resources/translations/*.ts (all of them!)
	cd "$(ROOT)" && git checkout -- 'resources/translations/*.ts'

##@ Help

help: ## Show this help
	@awk 'BEGIN { FS = ":.*## "; printf "Usage: make <target> [VARIABLE=value]\n" } \
	      /^##@/ { printf "\n%s\n", substr($$0, 5) } \
	      /^[a-zA-Z0-9_-]+:.*## / { printf "  %-22s %s\n", $$1, $$2 }' $(MAKEFILE_LIST)
	@printf "\nVariables:\n  %-22s %s\n  %-22s %s\n  %-22s %s\n  %-22s %s\n  %-22s %s\n" \
	        "QT_VERSION" "$(QT_VERSION)" "QTDIR" "$(QTDIR)" "QTDIR_STATIC" "$(QTDIR_STATIC)" "JOBS" "$(JOBS)" \
	        "TOOLCHAIN_IMAGE" "$(TOOLCHAIN_IMAGE)"
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
	@echo; echo "Build finished: $(OUT_DIR)/remote-ui ($$(cat "$(OUT_DIR)/version.txt"))"
	@echo "Run it with:  make run-$@   or:  . $(ENV_FILE) && $(subst $(ROOT)/,,$(OUT_DIR))/remote-ui"
endef

.PHONY: help linux linux-static ucr2 test run-linux run-linux-static clean clean-static clean-ucr2 clean-all translations-restore
