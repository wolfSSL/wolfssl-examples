#!/bin/bash
#
# setup.sh - Clone Yocto layers and verify host tools for RPi5 Scarthgap build
#
# Usage: ./setup.sh [--ignore-missing-packages]
#
# This script is idempotent — safe to run multiple times.
#
# Repos are cloned into repos/ (git-ignored). Symlinks are created in layers/
# so that layers/ is the single directory referenced by bblayers.conf.
#

set -e

IGNORE_MISSING=false
for arg in "$@"; do
    case "$arg" in
        --ignore-missing-packages) IGNORE_MISSING=true ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BRANCH="scarthgap"
REPOS_DIR="${SCRIPT_DIR}/repos"
LAYERS_DIR="${SCRIPT_DIR}/layers"

echo "=== Yocto Scarthgap RPi5 Setup ==="
echo "Working directory: ${SCRIPT_DIR}"
echo ""

cd "${SCRIPT_DIR}"

# --------------------------------------------------------------------------
# Check required host tools
# --------------------------------------------------------------------------

echo "--- Checking host tools ---"

MISSING=()

REQUIRED_TOOLS=(
    git
    gcc
    g++
    make
    python3
    wget
    diffstat
    unzip
    makeinfo
    chrpath
    socat
    cpio
    xz
    zstd
    lz4
    file
    gawk
    locale
)

for tool in "${REQUIRED_TOOLS[@]}"; do
    if command -v "${tool}" &> /dev/null; then
        echo "  [OK]    ${tool}"
    else
        echo "  [MISS]  ${tool}"
        MISSING+=("${tool}")
    fi
done

if [ ${#MISSING[@]} -gt 0 ]; then
    echo ""
    echo "WARNING: Missing required tools: ${MISSING[*]}"
    echo ""
    echo "On Debian/Ubuntu, install them with:"
    echo "  sudo apt install gawk wget git diffstat unzip texinfo gcc build-essential \\"
    echo "    chrpath socat cpio python3 python3-pip python3-pexpect xz-utils \\"
    echo "    debianutils iputils-ping python3-git python3-jinja2 python3-subunit \\"
    echo "    zstd liblz4-tool file locales libacl1"
    echo ""
    if [ "${IGNORE_MISSING}" = true ]; then
        echo "Continuing anyway (--ignore-missing-packages)."
    else
        echo "Re-run with --ignore-missing-packages to skip this check."
        exit 1
    fi
fi

echo ""

# --------------------------------------------------------------------------
# Clone repos
# --------------------------------------------------------------------------

mkdir -p "${REPOS_DIR}"

echo "--- Cloning repos (branch: ${BRANCH}) ---"

clone_repo() {
    local url="$1"
    local dir="$2"
    local branch="$3"

    if [ -d "${REPOS_DIR}/${dir}" ]; then
        echo "  [SKIP]  ${dir} already exists"
    else
        echo "  [CLONE] ${dir}"
        git clone -b "${branch}" "${url}" "${REPOS_DIR}/${dir}"
    fi
}

clone_repo "git://git.yoctoproject.org/poky"             "poky"              "${BRANCH}"
clone_repo "git://git.yoctoproject.org/meta-raspberrypi"  "meta-raspberrypi"  "${BRANCH}"
clone_repo "git://git.openembedded.org/meta-openembedded" "meta-openembedded" "${BRANCH}"
clone_repo "https://github.com/wolfSSL/meta-wolfssl.git"      "meta-wolfssl"      "master"

echo ""

# --------------------------------------------------------------------------
# Create symlinks in layers/
# --------------------------------------------------------------------------

echo "--- Linking repos into layers/ ---"

link_layer() {
    local name="$1"

    if [ -L "${LAYERS_DIR}/${name}" ]; then
        echo "  [SKIP]  layers/${name} symlink exists"
    elif [ -d "${LAYERS_DIR}/${name}" ]; then
        echo "  [SKIP]  layers/${name} is a directory (not a symlink)"
    else
        echo "  [LINK]  layers/${name} -> repos/${name}"
        ln -s "../repos/${name}" "${LAYERS_DIR}/${name}"
    fi
}

link_layer "poky"
link_layer "meta-raspberrypi"
link_layer "meta-openembedded"
link_layer "meta-wolfssl"

# --------------------------------------------------------------------------
# Generate .envrc for direnv
# --------------------------------------------------------------------------

echo "--- Generating .envrc ---"

if [ -f "${SCRIPT_DIR}/.envrc" ]; then
    echo "  [SKIP]  .envrc already exists"
else
    cat > "${SCRIPT_DIR}/.envrc" <<'ENVRC'
# Auto-source the Yocto/bitbake build environment when entering this directory.
# Managed by direnv — run 'direnv allow' after any changes to this file.

POKY_DIR="${PWD}/layers/poky"
BUILD_DIR="${PWD}/build"

if [ ! -d "${POKY_DIR}" ]; then
    echo "direnv: layers/poky not found — run ./setup.sh first"
elif [ ! -d "${BUILD_DIR}/conf" ]; then
    echo "direnv: build/ not initialized — run 'make configure' next"
else
    # Suppress the oe-init-build-env banner output
    source "${POKY_DIR}/oe-init-build-env" "${BUILD_DIR}" > /dev/null 2>&1

    # Stay in the original directory (oe-init-build-env changes cwd)
    cd "${ENVRC_DIR:-${PWD}}"
fi
ENVRC
    echo "  [OK]    .envrc generated"
fi

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Next steps:"
echo "  make configure  - Initialize build dir and write local.conf"
echo "  make build      - Build core-image-base"
echo ""
