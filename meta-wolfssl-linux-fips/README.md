# meta-wolfssl-linux-fips

Yocto Scarthgap build environment for Raspberry Pi 5 and QEMU (aarch64) with
wolfSSL FIPS Ready cryptography.

## Overview

This project provides a turnkey setup for building a Linux image targeting the
Raspberry Pi 5 or QEMU (aarch64) using the Yocto Project's **Scarthgap** (5.0)
release. The RPi5 base image includes SSH, HDMI console output, Wi-Fi,
Bluetooth, and systemd. The QEMU target allows testing the full FIPS Ready
stack without physical hardware.

When FIPS is enabled, the image includes:

- **wolfSSL FIPS Ready** cryptographic library
- **libgcrypt** with wolfSSL backend
- **GnuTLS** with wolfSSL backend
- **wolfProvider** (OpenSSL 3.x provider, replace-default mode)
- **wolfSSL FIPS kernel module** loaded via initramfs at early boot
- Kernel FIPS mode (`CONFIG_CRYPTO_FIPS=y`) and LKCAPI registration
- wolfcrypttest, wolfcryptbenchmark, ptest suites for validation

## Prerequisites

### Host packages (Ubuntu/Debian)

```bash
sudo apt install gawk wget git diffstat unzip texinfo gcc build-essential \
  chrpath socat cpio python3 python3-pip python3-pexpect xz-utils \
  debianutils iputils-ping python3-git python3-jinja2 python3-subunit \
  zstd liblz4-tool file locales libacl1
sudo locale-gen en_US.UTF-8
```

You can also run `./setup.sh` which will check for all required tools and
report any that are missing. On non-Debian distros where the tool names differ,
use `--ignore-missing-packages` to skip the check:

```bash
./setup.sh --ignore-missing-packages
```

### direnv (recommended)

direnv auto-sources the bitbake environment when you `cd` into the project
directory. `setup.sh` generates the `.envrc` file automatically.

**Step 1 - Install direnv and add the shell hook (one-time, before running
`setup.sh`):**

```bash
sudo apt install -y direnv
echo 'eval "$(direnv hook bash)"' >> ~/.bashrc
```

Open a new terminal (or `exec bash`) so the hook is active. `source ~/.bashrc`
is not sufficient in all shells.

**Step 2 - Run `./setup.sh`** (see Quick Start below). This generates the
`.envrc` file. direnv will report it as blocked on the first run — that's
expected.

**Step 3 - Allow direnv to load the generated `.envrc`:**

```bash
direnv allow
```

direnv will warn that `build/` isn't initialized yet — that's expected too.
It clears once `make configure` runs.

**Step 4 - Run `make configure`.** direnv auto-sources the bitbake
environment on the next prompt.

### System requirements

- **Disk space:** At least 65 GB free (downloads + sstate cache + build output)
- **RAM:** 8 GB minimum, 16 GB+ recommended
- **OS:** Linux (tested on Ubuntu 22.04+, Debian 13, WSL2)

## Quick Start

### 1. Common setup (required for all targets)

```bash
# Clone required Yocto layers and check host tools
./setup.sh

# Initialize build directory and write configuration
make configure
```

### 2a. RPi5 base image (no FIPS)

```bash
# (Optional) Set a static IP - default is DHCP
make ip-192.168.2.227 GW=192.168.2.1 PREFIX=24 DNS=8.8.8.8

# Build the base image
make build

# Copy the image to the current directory for flashing
make move-image
```

### 2b. RPi5 FIPS Ready image

```bash
# Generate FIPS config from your bundle (bundle can live anywhere on disk)
make generate-fips-conf BUNDLE=/path/to/wolfssl-x.x.x-gplv3-fips-ready.zip

# Enable FIPS (adds overrides layer, initramfs, and FIPS conf to local.conf)
make fips-on

# Build the FIPS image (auto-detected when FIPS is enabled)
make build

# Copy and flash
make move-image
```

To disable FIPS and go back to the base image:

```bash
make fips-off
make build
```

### 2c. QEMU FIPS Ready image (no RPi hardware needed)

This builds and runs the same FIPS Ready stack in QEMU, using `linux-yocto`
(6.6) instead of `linux-raspberrypi`. The wolfSSL FIPS kernel module loads
via initramfs at early boot, same as on RPi5.

```bash
# Generate FIPS config and enable FIPS (if not already done)
make generate-fips-conf BUNDLE=/path/to/wolfssl-x.x.x-gplv3-fips-ready.zip
make fips-on

# Build the QEMU FIPS image
make qemu

# Launch in QEMU (nographic/headless, serial on stdio)
make run-qemu
```

Exit QEMU with `Ctrl-A X`.

> **Note:** Both `make build` (RPi5) and `make qemu` (QEMU) can be used in
> the same build directory. FIPS must be enabled (`make fips-on`) for both.
> The `conf/qemu-override.conf` is applied via `bitbake -R` to swap the
> machine and kernel without modifying `local.conf`.

## Build Targets

| Command              | Description                                  |
|----------------------|----------------------------------------------|
| `make configure`     | Initialize build dir and write config        |
| `make build`         | Build image (FIPS or base, auto-detected)    |
| `make build-minimal` | Build `core-image-minimal` for RPi5          |
| `make clean`         | Clean the active image (cleansstate)         |
| `make distclean`     | Remove the entire build directory             |
| `make image-info`    | Show output image location and size          |
| `make move-image`    | Copy latest `.wic` image to current directory|
| `make generate-fips-conf BUNDLE=<path>` | Generate FIPS config from .zip bundle |
| `make fips-on`       | Enable FIPS Ready overrides and initramfs    |
| `make fips-off`      | Disable FIPS Ready overrides                 |
| `make fips-status`   | Show FIPS Ready status                       |
| `make qemu`          | Build FIPS Ready image for QEMU (qemuarm64)  |
| `make run-qemu`      | Launch QEMU image (nographic by default)      |
| `make ip-dhcp`       | Set network to DHCP                          |
| `make ip-<address>`  | Set static IP (e.g. `make ip-192.168.1.100`) |
| `make shell`         | Open a shell with the bitbake env sourced    |

## FIPS Ready Architecture

When FIPS is enabled (`make fips-on`), the overrides layer
(`layers/meta-wolfssl-overrides/`) is added to the build. This layer:

- Provides the `wolfssl-fips-ready-image` recipe for RPi5 and
  `wolfssl-fips-ready-image-qemu` for QEMU (both extend `core-image-base`)
- Configures wolfSSL, libgcrypt, GnuTLS, wolfProvider, OpenSSL, curl, and
  OpenSSH via bbappends
- Enables kernel FIPS mode and the wolfSSL kernel randomness patch
- Bundles the wolfSSL FIPS kernel module into an initramfs for early boot
- Signs the kernel module with the kernel's build key

### Boot sequence (FIPS enabled)

**RPi5:**
```
1. RPi5 firmware loads kernel (with bundled initramfs)
2. Initramfs mounts, modprobe libwolfssl loads FIPS kernel module
3. Root filesystem mounts
4. Userspace applications use wolfSSL FIPS via:
   - wolfProvider (OpenSSL 3.x replace-default)
   - GnuTLS with wolfSSL backend
   - libgcrypt with wolfSSL backend
```

**QEMU (qemuarm64):**
```
1. QEMU loads linux-yocto kernel (with bundled initramfs)
2. Initramfs mounts, modprobe libwolfssl loads FIPS kernel module
3. Root filesystem mounts (ext4 via virtio-blk)
4. Same userspace FIPS stack as RPi5
```

### FIPS bundle

The FIPS Ready bundle `.zip` can live anywhere on disk. Running
`make generate-fips-conf BUNDLE=/path/to/bundle.zip` computes checksums and
generates `conf/wolfssl-fips-ready.conf` pointing to the bundle's location.
The bundle is never copied into the source tree.

## Network Configuration

The image uses systemd-networkd for network management. Set the IP configuration
**before** building the image - it gets baked in.

### DHCP (default)

```bash
make ip-dhcp
make configure && make build
```

### Static IP

```bash
make ip-192.168.2.227 GW=192.168.2.1 PREFIX=24 DNS=8.8.8.8
make configure && make build
```

| Parameter | Default                  | Description              |
|-----------|--------------------------|--------------------------|
| `GW`      | Auto (IP with `.1`)      | Gateway address          |
| `PREFIX`  | `24`                     | Subnet prefix length     |
| `DNS`     | `8.8.8.8`               | DNS server               |

## Flashing and Booting

### RPi5: Flash to SD card

```bash
make move-image
lsblk  # find your SD card device

# Using bmaptool (recommended)
sudo bmaptool copy wolfssl-fips-ready-image-raspberrypi5.rootfs.wic.bz2 /dev/sdX

# Using dd
bzcat wolfssl-fips-ready-image-raspberrypi5.rootfs.wic.bz2 | sudo dd of=/dev/sdX bs=4M status=progress
```

### RPi5: Boot and connect

**Login:** `root` with no password (`debug-tweaks` is enabled).

**SSH:**
```bash
ssh root@192.168.2.227
```

### QEMU: Launch and connect

```bash
make run-qemu
```

Boots directly to a login prompt on stdio. Login as `root` (no password).
Exit with `Ctrl-A X`.

## Testing

These tests apply to both RPi5 and QEMU. Run them after booting the image.

### 1. Kernel Module Verification

Verify that the wolfSSL FIPS kernel module loaded via initramfs:

```bash
lsmod | grep libwolfssl
dmesg | grep -i wolfssl
```

Expected dmesg output includes:
```
FIPS 140-3 wolfCrypt-fips ... startup self-test succeeded.
wolfCrypt self-test passed.
wolfCrypt: changing fips_enabled from 0 to 1 for FIPS module.
```

Followed by LKCAPI algorithm registrations (AES-CBC, AES-GCM, HMAC-SHA*, SHA*, RSA, ECDSA, ECDH, DRBG).

Verify kernel crypto API registration:
```bash
cat /proc/crypto | grep -A 5 wolfcrypt
```

### 2. Kernel Randomness

If the kernel randomness patch is applied, verify the wolfSSL DRBG is integrated:

```bash
dmesg | grep "kernel global random_bytes handlers installed"
```

Test the DRBG under load:
```bash
# Basic test (2 GB)
dd if=/dev/urandom bs=1M count=2000 status=progress of=/dev/null
```

### 3. Cryptographic Benchmarking

Test kernel crypto via cryptsetup:

```bash
cryptsetup benchmark --cipher aes-cbc --key-size 256
cryptsetup benchmark
```

### 4. wolfCrypt Tests

```bash
wolfcrypttest
wolfcryptbenchmark
```

### 5. Testing libgcrypt

libgcrypt is configured to use wolfSSL as its crypto backend.

```bash
ptest-runner libgcrypt
```

Expected output: all tests PASS (basic, mpitests, curves, fips186-dsa, etc.).

Verify library linking:
```bash
ldd /usr/lib/libgcrypt.so.20  # should show libwolfssl
```

### 6. Testing GnuTLS

GnuTLS uses wolfSSL via the wolfssl-gnutls-wrapper.

```bash
cd /opt/wolfssl-gnutls-wrapper/tests/
make run_fips
```

All tests should pass with wrapper log messages showing crypto operations routed to wolfSSL.

Verify wrapper linking:
```bash
ldd /opt/wolfssl-gnutls-wrapper/lib/libgnutls-wolfssl-wrapper.so
# should show both libgnutls.so and libwolfssl.so
```

### 7. Testing wolfProvider

wolfProvider is an OpenSSL 3.x provider using wolfSSL as the crypto backend, configured in replace-default mode.

```bash
wolfprovidertest
```

Expected output:
```
Setting up environment...
Detected replace-default mode (from config file)
Detected wolfSSL FIPS build (from config file)
...
###### TESTSUITE SUCCESS
==========================================
Unit tests PASSED!
==========================================
```

### 8. Testing libssh (via libgcrypt)

```bash
ptest-runner libssh
```

Some tests may fail due to MD5 not being available in FIPS mode. Expected failures:
- `torture_keyfiles` - legacy key formats using MD5
- `torture_pki_rsa` - legacy RSA key formats using MD5
- `torture_threads_pki_rsa` - multi-threaded legacy RSA using MD5

## Project Structure

```
meta-wolfssl-linux-fips/
├── README.md
├── .gitignore
├── .envrc                 # (generated by setup.sh) direnv config
├── setup.sh               # Clone repos, create symlinks, check host tools
├── configure.sh           # Initialize build dir and write config
├── generate-fips-conf.sh  # Generate FIPS config from bundle .zip
├── set-network.sh         # Write network config (called by make ip-*)
├── Makefile               # Build/clean/utility targets
├── conf/
│   ├── wolfssl-fips-ready.conf    # (generated) FIPS bundle config
│   └── qemu-override.conf        # QEMU machine overrides for bitbake -R
├── layers/
│   ├── poky -> ../repos/poky                          # (symlink)
│   ├── meta-raspberrypi -> ../repos/meta-raspberrypi  # (symlink)
│   ├── meta-openembedded -> ../repos/meta-openembedded # (symlink)
│   ├── meta-wolfssl -> ../repos/meta-wolfssl          # (symlink)
│   └── meta-wolfssl-overrides/   # FIPS overrides layer (tracked)
│       ├── conf/layer.conf
│       ├── recipes-core/images/
│       │   ├── wolfssl-fips-ready-image.bb       # RPi5 FIPS image
│       │   ├── wolfssl-fips-ready-image-qemu.bb  # QEMU FIPS image
│       │   └── core-image-minimal-initramfs.bbappend
│       ├── recipes-kernel/linux/
│       │   ├── linux-raspberrypi_%.bbappend  # RPi kernel patches
│       │   ├── linux-yocto_%.bbappend        # QEMU kernel patches
│       │   └── files/fips-crypto.cfg
│       ├── recipes-wolfssl/
│       │   ├── wolfssl/wolfssl-fips-ready.bbappend
│       │   ├── wolfssl/wolfssl-linuxkm-fips-ready.bbappend
│       │   └── wolfprovider/wolfprovider_%.bbappend
│       ├── recipes-support/
│       │   ├── curl/curl_%.bbappend
│       │   ├── gnupg/gnupg_%.bbappend
│       │   ├── gnutls/gnutls_%.bbappend
│       │   ├── libgcrypt/libgcrypt_%.bbappend
│       │   └── nettle/nettle_%.bbappend
│       ├── recipes-connectivity/
│       │   ├── openssh/openssh_%.bbappend
│       │   └── openssl/openssl_%.bbappend
│       └── recipes-core/network-config/  # Network config recipe
│           ├── network-config_1.0.bb
│           └── files/20-wired.network    # (generated by make ip-*)
├── repos/                 # Cloned git repos (git-ignored)
├── build/                 # Build directory (git-ignored)
├── downloads/             # Shared download cache (git-ignored)
└── sstate-cache/          # Shared state cache (git-ignored)
```

## Image Configuration

### RPi5

| Setting     | Value                                |
|-------------|--------------------------------------|
| Machine     | `raspberrypi5`                       |
| Kernel      | `linux-raspberrypi` (6.6)            |
| Base image  | `core-image-base`                    |
| FIPS image  | `wolfssl-fips-ready-image`           |
| SSH         | OpenSSH server enabled               |
| Display     | vc4graphics / HDMI 1080p             |
| Wi-Fi       | Enabled (linux-firmware-rpidistro)   |
| Bluetooth   | Enabled (bluez5)                     |
| Init system | systemd                              |
| U-Boot      | Disabled (direct kernel boot)        |
| Login       | `root` / no password (debug-tweaks)  |
| Branch      | Scarthgap (Yocto 5.0)               |

### QEMU (aarch64)

| Setting     | Value                                |
|-------------|--------------------------------------|
| Machine     | `qemuarm64`                          |
| Kernel      | `linux-yocto` (6.6)                  |
| FIPS image  | `wolfssl-fips-ready-image-qemu`      |
| CPU         | `-cpu max` (supports A76 binaries)   |
| Console     | nographic (serial on stdio)          |
| Networking  | TAP via virtio-net-pci               |
| Init system | systemd                              |
| Login       | `root` / no password (debug-tweaks)  |
| Branch      | Scarthgap (Yocto 5.0)               |

## What Each Step Does (Manual Equivalent)

If you want to integrate wolfSSL FIPS Ready into your own Yocto project, here
is what each make target does under the hood and the equivalent manual steps.

### `./setup.sh`

Clones the required Yocto layers into `repos/` and creates symlinks in
`layers/` so `bblayers.conf` has a single directory to reference:

```bash
git clone -b scarthgap git://git.yoctoproject.org/poky repos/poky
git clone -b scarthgap git://git.yoctoproject.org/meta-raspberrypi repos/meta-raspberrypi
git clone -b scarthgap git://git.openembedded.org/meta-openembedded repos/meta-openembedded
git clone -b master https://github.com/wolfSSL/meta-wolfssl.git repos/meta-wolfssl

ln -s ../repos/poky layers/poky
ln -s ../repos/meta-raspberrypi layers/meta-raspberrypi
ln -s ../repos/meta-openembedded layers/meta-openembedded
ln -s ../repos/meta-wolfssl layers/meta-wolfssl
```

Also checks that required host tools (gcc, python3, etc.) are installed and
generates a `.envrc` for direnv.

### `make configure`

Runs `configure.sh`, which:

1. Sources `oe-init-build-env` to create the `build/` directory
2. Writes `build/conf/bblayers.conf` with all layer paths (poky, meta-oe,
   meta-raspberrypi, meta-wolfssl, meta-network-overrides)
3. Writes an RPi5-specific config block into `build/conf/local.conf`:
   - `MACHINE = "raspberrypi5"`
   - SSH, HDMI, Wi-Fi, Bluetooth, systemd
   - Shared download/sstate cache directories

Manual equivalent: run `source layers/poky/oe-init-build-env build` and edit
`build/conf/bblayers.conf` and `build/conf/local.conf` by hand.

### `make generate-fips-conf BUNDLE=<path>`

Runs `generate-fips-conf.sh`, which inspects a wolfSSL FIPS Ready `.zip`
bundle and generates `conf/wolfssl-fips-ready.conf` containing:

- `PREFERRED_PROVIDER_virtual/wolfssl = "wolfssl-fips-ready"` (and linuxkm)
- `WOLFSSL_VERSION`, `WOLFSSL_SRC_SHA` (SHA256 of the zip)
- `WOLFSSL_LICENSE_MD5` (MD5 of the COPYING file inside the zip)
- `WOLFSSL_SRC_DIR` (absolute path to the directory containing the zip)
- `FIPS_HASH` placeholder (auto-extracted during build via QEMU)

Manual equivalent: compute the SHA256 and license MD5 yourself and write the
conf file with the correct variable assignments.

### `make fips-on`

Enables the FIPS overrides by modifying two files:

1. **`build/conf/bblayers.conf`** - adds `layers/meta-wolfssl-overrides` to
   the `BBLAYERS` list (after `meta-wolfssl`)
2. **`build/conf/local.conf`** - appends a config block:
   ```
   require conf/wolfssl-fips-ready.conf
   INITRAMFS_IMAGE = "core-image-minimal-initramfs"
   INITRAMFS_IMAGE_BUNDLE = "1"
   ```

The overrides layer (`meta-wolfssl-overrides`) provides:
- Image recipes (`wolfssl-fips-ready-image`, `wolfssl-fips-ready-image-qemu`)
- Kernel bbappends that apply the wolfSSL randomness patches and FIPS crypto
  config (`CONFIG_CRYPTO_FIPS=y`, module signing)
- An initramfs bbappend that adds `wolfssl-linuxkm` to the initramfs and
  injects `modprobe libwolfssl` into the init script
- bbappends for libgcrypt, GnuTLS, wolfProvider, OpenSSL, curl, and OpenSSH
  to use wolfSSL as the crypto backend

### `make build` (RPi5)

Sources the bitbake environment and runs:

```bash
source layers/poky/oe-init-build-env build
bitbake wolfssl-fips-ready-image    # if FIPS enabled
bitbake core-image-base             # if FIPS disabled
```

This builds the full image for `MACHINE = "raspberrypi5"` using
`linux-raspberrypi` (6.6). The kernel is bundled with the initramfs
containing the wolfSSL FIPS kernel module. Output is a `.wic.bz2` image
in `build/tmp/deploy/images/raspberrypi5/`.

### `make qemu`

Sources the bitbake environment and runs:

```bash
source layers/poky/oe-init-build-env build
bitbake -R conf/qemu-override.conf wolfssl-fips-ready-image-qemu
```

The `-R conf/qemu-override.conf` flag layers additional config on top of
`local.conf` without modifying it. The override conf:

- Sets `MACHINE = "qemuarm64"`
- Switches kernel to `linux-yocto` (since `linux-raspberrypi` doesn't
  support QEMU)
- Sets `INITRAMFS_IMAGE` and `INITRAMFS_IMAGE_BUNDLE` for the QEMU kernel
- Clears RPi device trees (`KERNEL_DEVICETREE = ""`)
- Adds `IMAGE_CLASSES += "qemuboot"` and `IMAGE_FSTYPES += "ext4"` (normally
  provided by `qemuarm64.conf`, but `-R` is parsed too late for machine
  config files to take effect)
- Provides all `QB_*` variables (`QB_MACHINE`, `QB_CPU`, `QB_SERIAL_OPT`,
  etc.) that `runqemu` needs to launch the VM
- Sets `console=ttyAMA0` (QEMU's virtual PL011 UART, replacing RPi's
  `ttyAMA10`)
- Removes RPi-specific packages (`linux-firmware-rpidistro-bcm43455`)

Output is an `.ext4` rootfs and `.qemuboot.conf` in
`build/tmp/deploy/images/qemuarm64/`.

### `make run-qemu`

Sources the bitbake environment and runs:

```bash
source layers/poky/oe-init-build-env build
runqemu /path/to/wolfssl-fips-ready-image-qemu-qemuarm64.qemuboot.conf nographic
```

The `.qemuboot.conf` file contains all the QEMU launch parameters (machine
type, CPU, memory, drives, network, serial). `runqemu` reads it and
constructs the full `qemu-system-aarch64` command line. The `nographic` flag
redirects the VM console to stdio.

### `make fips-off`

Reverses `make fips-on`:

1. Removes `meta-wolfssl-overrides` from `build/conf/bblayers.conf`
2. Removes the FIPS config block (including `INITRAMFS_IMAGE` settings) from
   `build/conf/local.conf`

After this, `make build` produces a plain `core-image-base` without wolfSSL.

### `make ip-<address>` / `make ip-dhcp`

Runs `set-network.sh` to write a systemd-networkd config file at
`layers/meta-network-overrides/recipes-core/network-config/files/20-wired.network`.
This file is baked into the image at build time - it cannot be changed after
flashing without rebuilding.

## License

This layer configuration is provided under the MIT license. wolfSSL itself is
dual-licensed under GPLv2 and a commercial license. See
[wolfSSL licensing](https://www.wolfssl.com/license/) for details.
