# Makefile - Yocto Scarthgap RPi5 build targets
#
# Usage:
#   ./setup.sh        - clone layers and check host tools
#   make configure    - initialize build dir and write config
#   make build        - build the image

SHELL := /bin/bash
TOPDIR := $(shell pwd)
BUILD_DIR := $(TOPDIR)/build
POKY_DIR := $(TOPDIR)/layers/poky
FIPS_IMAGE := wolfssl-fips-ready-image
BASE_IMAGE := core-image-base
MACHINE := raspberrypi5
DEPLOY_DIR := $(BUILD_DIR)/tmp/deploy/images/$(MACHINE)
QEMU_MACHINE := qemuarm64
QEMU_IMAGE := wolfssl-fips-ready-image-qemu
QEMU_CONF := $(TOPDIR)/conf/qemu-override.conf
QEMU_DEPLOY_DIR := $(BUILD_DIR)/tmp/deploy/images/$(QEMU_MACHINE)

# Helper: source the OE environment and run a bitbake command
define bitbake-cmd
	@if [ ! -d "$(POKY_DIR)" ]; then \
		echo "Error: poky not found. Run ./setup.sh first."; \
		exit 1; \
	fi
	@source $(POKY_DIR)/oe-init-build-env $(BUILD_DIR) > /dev/null 2>&1 && $(1)
endef

LAYERS_DIR := $(TOPDIR)/layers
OVERRIDES_LAYER := $(LAYERS_DIR)/meta-wolfssl-overrides
BBLAYERS_CONF := $(BUILD_DIR)/conf/bblayers.conf
LOCAL_CONF := $(BUILD_DIR)/conf/local.conf
FIPS_CONF := $(TOPDIR)/conf/wolfssl-fips-ready.conf
FIPS_MARKER := \# ==== wolfssl-fips-ready config ====

.PHONY: configure build build-minimal clean distclean clean-project image-info move-image ip-dhcp fips-on fips-off fips-status qemu run-qemu shell help

## configure: Initialize build dir, write bblayers.conf and local.conf
configure:
	@$(TOPDIR)/configure.sh

## build: Build the image (FIPS image if enabled, base image otherwise)
build:
	@if [ -f "$(BBLAYERS_CONF)" ] && grep -q "meta-wolfssl-overrides" "$(BBLAYERS_CONF)" 2>/dev/null; then \
		echo "FIPS enabled - building $(FIPS_IMAGE)"; \
	else \
		echo "Building $(BASE_IMAGE)"; \
	fi
	$(call bitbake-cmd, \
		if grep -q "meta-wolfssl-overrides" "$(BBLAYERS_CONF)" 2>/dev/null; then \
			bitbake $(FIPS_IMAGE); \
		else \
			bitbake $(BASE_IMAGE); \
		fi)

## build-minimal: Build core-image-minimal for RPi5
build-minimal:
	$(call bitbake-cmd, bitbake core-image-minimal)

## clean: Clean the active image sstate (cleansstate)
clean:
	$(call bitbake-cmd, \
		if grep -q "meta-wolfssl-overrides" "$(BBLAYERS_CONF)" 2>/dev/null; then \
			bitbake -c cleansstate $(FIPS_IMAGE); \
		else \
			bitbake -c cleansstate $(BASE_IMAGE); \
		fi)

## distclean: Remove the entire build directory
distclean:
	@echo "Removing build directory: $(BUILD_DIR)"
	rm -rf $(BUILD_DIR)

## clean-project: Reset to clean state (keeps source files and FIPS bundle)
clean-project:
	@echo "Removing all generated files and caches..."
	rm -rf $(BUILD_DIR) $(TOPDIR)/repos $(TOPDIR)/downloads $(TOPDIR)/sstate-cache
	rm -rf $(TOPDIR)/conf/wolfssl-fips-ready.conf $(TOPDIR)/.envrc
	rm -f $(TOPDIR)/*.wic $(TOPDIR)/*.wic.bz2 $(TOPDIR)/*.wic.gz $(TOPDIR)/*.wic.bmap
	rm -f $(TOPDIR)/layers/meta-network-overrides/recipes-core/network-config/files/20-wired.network
	@echo "Done. Run './setup.sh' to start fresh."

## image-info: Show the output image location and file sizes
image-info:
	@echo "Deploy directory: $(DEPLOY_DIR)"
	@echo ""
	@if [ -d "$(DEPLOY_DIR)" ]; then \
		ls -lh $(DEPLOY_DIR)/*.wic.bz2 2>/dev/null || \
		ls -lh $(DEPLOY_DIR)/*.wic.gz 2>/dev/null || \
		ls -lh $(DEPLOY_DIR)/*.wic 2>/dev/null || \
		echo "No .wic image found yet. Run 'make build' first."; \
	else \
		echo "Deploy directory does not exist. Run 'make build' first."; \
	fi

## move-image: Copy the latest .wic image to the current directory
move-image:
	@LATEST=$$(ls -t $(DEPLOY_DIR)/*.wic.bz2 2>/dev/null | head -1); \
	if [ -z "$$LATEST" ]; then \
		LATEST=$$(ls -t $(DEPLOY_DIR)/*.wic.gz 2>/dev/null | head -1); \
	fi; \
	if [ -z "$$LATEST" ]; then \
		LATEST=$$(ls -t $(DEPLOY_DIR)/*.wic 2>/dev/null | head -1); \
	fi; \
	if [ -z "$$LATEST" ]; then \
		echo "No .wic image found. Run 'make build' first."; \
		exit 1; \
	fi; \
	echo "Copying $$LATEST to ."; \
	cp "$$LATEST" . ; \
	echo "Done: $$(basename $$LATEST)"

## generate-fips-conf: Generate wolfssl-fips-ready.conf from a bundle .zip
## Usage: make generate-fips-conf BUNDLE=/path/to/wolfssl-x.x.x-gplv3-fips-ready.zip
generate-fips-conf:
	@if [ -z "$(BUNDLE)" ]; then \
		echo "Usage: make generate-fips-conf BUNDLE=/path/to/wolfssl-x.x.x-gplv3-fips-ready.zip"; \
		exit 1; \
	fi
	@$(TOPDIR)/generate-fips-conf.sh "$(BUNDLE)"

## fips-on: Enable FIPS Ready - add overrides layer and require conf
fips-on:
	@if [ ! -f "$(BBLAYERS_CONF)" ]; then \
		echo "Error: bblayers.conf not found. Run 'make configure' first."; \
		exit 1; \
	fi
	@if [ ! -f "$(FIPS_CONF)" ]; then \
		echo "Error: conf/wolfssl-fips-ready.conf not found."; \
		echo ""; \
		echo "Generate it from your bundle .zip:"; \
		echo "  make generate-fips-conf BUNDLE=/path/to/wolfssl-x.x.x-gplv3-fips-ready.zip"; \
		echo ""; \
		echo "The bundle can live anywhere on disk."; \
		exit 1; \
	fi
	@# --- Add overrides layer to bblayers.conf ---
	@if grep -q "meta-wolfssl-overrides" "$(BBLAYERS_CONF)"; then \
		echo "[FIPS] Overrides layer already in bblayers.conf"; \
	else \
		sed -i 's|$(LAYERS_DIR)/meta-wolfssl \\|$(LAYERS_DIR)/meta-wolfssl \\\n  $(OVERRIDES_LAYER) \\|' "$(BBLAYERS_CONF)"; \
		echo "[FIPS] Added overrides layer to bblayers.conf"; \
	fi
	@# --- Add FIPS config to local.conf ---
	@if grep -q "$(FIPS_MARKER)" "$(LOCAL_CONF)"; then \
		echo "[FIPS] FIPS config block already in local.conf"; \
	else \
		echo '' >> "$(LOCAL_CONF)"; \
		echo '$(FIPS_MARKER)' >> "$(LOCAL_CONF)"; \
		echo 'require $(FIPS_CONF)' >> "$(LOCAL_CONF)"; \
		echo '' >> "$(LOCAL_CONF)"; \
		echo '# FIPS initramfs - kernel bundles wolfSSL kernel module for early boot' >> "$(LOCAL_CONF)"; \
		echo 'INITRAMFS_IMAGE = "core-image-minimal-initramfs"' >> "$(LOCAL_CONF)"; \
		echo 'INITRAMFS_IMAGE_BUNDLE = "1"' >> "$(LOCAL_CONF)"; \
		echo '$(FIPS_MARKER) END' >> "$(LOCAL_CONF)"; \
		echo "[FIPS] Added FIPS config to local.conf"; \
	fi
	@echo ""
	@echo "FIPS Ready ENABLED. Run 'make build' for the RPi5 image, or 'make qemu' for the QEMU image."

## fips-off: Disable FIPS Ready - remove overrides layer and FIPS config
fips-off:
	@if [ ! -f "$(BBLAYERS_CONF)" ]; then \
		echo "Error: bblayers.conf not found. Run 'make configure' first."; \
		exit 1; \
	fi
	@# --- Remove overrides layer from bblayers.conf ---
	@if grep -q "meta-wolfssl-overrides" "$(BBLAYERS_CONF)"; then \
		sed -i '\|meta-wolfssl-overrides|d' "$(BBLAYERS_CONF)"; \
		echo "[FIPS] Removed overrides layer from bblayers.conf"; \
	else \
		echo "[FIPS] Overrides layer already removed from bblayers.conf"; \
	fi
	@# --- Remove FIPS config block from local.conf ---
	@if grep -q "$(FIPS_MARKER)" "$(LOCAL_CONF)"; then \
		sed -i '/^$(FIPS_MARKER)/,/^$(FIPS_MARKER) END/d' "$(LOCAL_CONF)"; \
		echo "[FIPS] Removed FIPS config block from local.conf"; \
	else \
		echo "[FIPS] FIPS config block already removed from local.conf"; \
	fi
	@echo ""
	@echo "FIPS Ready DISABLED. Run 'make clean && make build' to rebuild."

## fips-status: Show whether FIPS Ready overrides are enabled
fips-status:
	@if [ ! -f "$(BBLAYERS_CONF)" ]; then \
		echo "FIPS: unknown (bblayers.conf not found)"; \
	elif grep -q "meta-wolfssl-overrides" "$(BBLAYERS_CONF)"; then \
		echo "FIPS: ENABLED"; \
		if [ -f "$(FIPS_CONF)" ]; then \
			echo "Config: conf/wolfssl-fips-ready.conf found"; \
		else \
			echo "Config: conf/wolfssl-fips-ready.conf MISSING"; \
		fi; \
	else \
		echo "FIPS: DISABLED"; \
	fi

## ip-dhcp: Set network to DHCP
ip-dhcp:
	@$(TOPDIR)/set-network.sh dhcp

## ip-<address>: Set static IP (e.g. make ip-192.168.1.100)
## Optional: GW=192.168.1.1 PREFIX=24 DNS=8.8.8.8
ip-%:
	@$(TOPDIR)/set-network.sh static $* $(GW) $(PREFIX) $(DNS)

## qemu: Build the FIPS Ready image for QEMU (aarch64)
qemu:
	@if [ ! -f "$(BBLAYERS_CONF)" ]; then \
		echo "Error: bblayers.conf not found. Run 'make configure' first."; \
		exit 1; \
	fi
	@if ! grep -q "meta-wolfssl-overrides" "$(BBLAYERS_CONF)" 2>/dev/null; then \
		echo "Error: FIPS not enabled. Run 'make fips-on' first."; \
		exit 1; \
	fi
	@echo "Building $(QEMU_IMAGE) for $(QEMU_MACHINE)"
	$(call bitbake-cmd, bitbake -R $(QEMU_CONF) $(QEMU_IMAGE))

## run-qemu: Launch the QEMU image (use QEMU_ARGS for extra options, e.g. QEMU_ARGS=nographic)
run-qemu:
	@QBCONF=$$(ls -t $(QEMU_DEPLOY_DIR)/*.qemuboot.conf 2>/dev/null | head -1); \
	if [ -z "$$QBCONF" ]; then \
		echo "Error: No .qemuboot.conf found. Run 'make qemu' first."; \
		exit 1; \
	fi; \
	echo "Using: $$QBCONF"
	$(call bitbake-cmd, \
		QBCONF=$$(ls -t $(QEMU_DEPLOY_DIR)/*.qemuboot.conf 2>/dev/null | head -1) && \
		runqemu $$QBCONF nographic $(QEMU_ARGS))

## shell: Open an interactive shell with the bitbake environment sourced
shell:
	@if [ ! -d "$(POKY_DIR)" ]; then \
		echo "Error: poky not found. Run ./setup.sh first."; \
		exit 1; \
	fi
	@echo "Entering bitbake environment shell. Type 'exit' to leave."
	@bash --init-file <(echo "source $(POKY_DIR)/oe-init-build-env $(BUILD_DIR)")

## help: Show available targets
help:
	@echo "Yocto Scarthgap RPi5 Build Targets"
	@echo "==================================="
	@echo ""
	@echo "  make configure     - Initialize build dir and write config"
	@echo "  make build         - Build image for $(MACHINE) (FIPS or base, auto-detected)"
	@echo "  make build-minimal - Build core-image-minimal for $(MACHINE)"
	@echo "  make clean         - Clean active image sstate"
	@echo "  make distclean     - Remove entire build directory"
	@echo "  make clean-project - Reset to clean state (keeps source + bundle)"
	@echo "  make image-info    - Show output image location and sizes"
	@echo "  make move-image    - Copy latest .wic image to current directory"
	@echo "  make generate-fips-conf BUNDLE=<path> - Generate FIPS config from .zip bundle"
	@echo "  make fips-on       - Enable FIPS Ready overrides"
	@echo "  make fips-off      - Disable FIPS overrides layer"
	@echo "  make fips-status   - Show FIPS override status"
	@echo "  make qemu          - Build FIPS Ready image for QEMU ($(QEMU_MACHINE))"
	@echo "  make run-qemu      - Launch QEMU image (nographic/headless by default)"
	@echo "  make ip-dhcp       - Set network to DHCP"
	@echo "  make ip-<address>  - Set static IP (e.g. make ip-192.168.1.100)"
	@echo "                       Optional: GW=x.x.x.x PREFIX=24 DNS=x.x.x.x"
	@echo "  make shell         - Open a bitbake-ready shell"
	@echo "  make help          - Show this help"
	@echo ""
	@echo "Run ./setup.sh first to clone layers and check host tools."
