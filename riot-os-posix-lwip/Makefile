APPLICATION = posix-tls

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# lwIP's memory management doesn't seem to work on non 32-bit platforms at the
# moment.
BOARD_BLACKLIST := arduino-duemilanove arduino-leonardo \
                   arduino-mega2560 arduino-nano \
                   arduino-uno chronos esp8266-esp-12x esp8266-olimex-mod \
                   esp8266-sparkfun-thing jiminy-mega256rfr2 mega-xplained \
                   msb-430 msb-430h telosb waspmote-pro \
                   wsn430-v1_3b wsn430-v1_4 z1
BOARD_INSUFFICIENT_MEMORY = blackpill bluepill i-nucleo-lrwan1 \
                            nucleo-f031k6 nucleo-f042k6 \
                            nucleo-l031k6 nucleo-f030r8 nucleo-f302r8 \
                            nucleo-f303k8 nucleo-f334r8 nucleo-l053r8 \
                            saml10-xpro saml11-xpro stm32f0discovery

LWIP_IPV4 ?= 1
CFLAGS += -DTHREAD_STACKSIZE_MAIN=20000

ifneq (0, $(LWIP_IPV4))
  USEMODULE += ipv4_addr
  USEMODULE += lwip_arp
  USEMODULE += lwip_ipv4
  CFLAGS += -DETHARP_SUPPORT_STATIC_ENTRIES=1
  LWIP_IPV6 ?= 0
else
  LWIP_IPV6 ?= 1
endif

ifneq (0, $(LWIP_IPV6))
  USEMODULE += ipv6_addr
  USEMODULE += lwip_ipv6_autoconfig
endif

USEMODULE += inet_csum
USEMODULE += lwip_netdev
USEMODULE += netdev_eth
USEMODULE += netdev_default
USEMODULE += ps
USEMODULE += sock_tcp

USEPKG += wolfssl
USEMODULE+=wolfssl_socket
USEMODULE+=wolfcrypt_rsa
USEMODULE+=wolfcrypt_ecc
USEMODULE+=wolfcrypt_aes
USEMODULE+=wolfcrypt_asn
USEMODULE+=wolfcrypt_hmac
USEMODULE+=wolfcrypt_md5
USEMODULE+=wolfcrypt_sha
USEMODULE+=wolfcrypt_random
USEMODULE+=wolfssl_internal
USEMODULE+=wolfssl_wolfio
USEMODULE+=wolfssl_keys
USEMODULE+=wolfssl_ssl
USEMODULE+=wolfssl_tls
USEMODULE+=wolfcrypt_aes_gcm

CFLAGS+=-DHAVE_AESGCM -DWC_RSA_PSS -DHAVE_HKDF


USEMODULE += posix_sockets
DISABLE_MODULE += auto_init_lwip

CFLAGS += -DSO_REUSE
CFLAGS += -DLWIP_SO_RCVTIMEO
CFLAGS += -DLWIP_SOCK_TCP_ACCEPT_TIMEOUT=500
CFLAGS += -DLWIP_NETIF_LOOPBACK=1
CFLAGS += -DLWIP_HAVE_LOOPIF=1


# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands

# Uncomment next line to enable debug symbols
CFLAGS+=-g -ggdb3

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1


include $(RIOTBASE)/Makefile.include
