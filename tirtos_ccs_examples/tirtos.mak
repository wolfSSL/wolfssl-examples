#
#  ======== tirtos.mak ========
#

#
# Set location of various cgtools
# These variables can be set here or on the command line.
#
CCS_BUILD ?= true
DEFAULT_INSTALLATION_DIR ?= c:/ti
ti.targets.arm.elf.M4F   ?= $(DEFAULT_INSTALLATION_DIR)/ccsv6/tools/compiler/arm_5.1.5

#
# Enable TI-RTOS to build for IAR.
# Set IAR_ENABLE to true and modify path to toolchain
#
IAR_BUILD ?= false
IAR_COMPILER_INSTALLATION_DIR ?= C:/Program Files (x86)/IAR Systems/Embedded Workbench 6.5
iar.targets.arm.M4F           ?= $(IAR_COMPILER_INSTALLATION_DIR)/arm

#
# Enable TI-RTOS to build for GCC.
# Set IAR_ENABLE to true and modify path to toolchain
#
GCC_BUILD ?= false
GCC_INSTALLATION_DIR ?= $(DEFAULT_INSTALLATION_DIR)/ccsv6/tools/compiler/gcc-arm-none-eabi-4_7-2013q3
gnu.targets.arm.M4F  ?= $(GCC_INSTALLATION_DIR)

#
# TI-RTOS and XDCTools settings
#
XDCTOOLS_INSTALLATION_DIR ?= $(DEFAULT_INSTALLATION_DIR)/xdctools_3_30_01_25_core
export XDCTOOLS_JAVA_HOME ?= $(DEFAULT_INSTALLATION_DIR)/ccsv6/eclipse/jre

TIRTOS_INSTALLATION_DIR   ?= $(DEFAULT_INSTALLATION_DIR)/tirtos_tivac_2_00_02_36
BIOS_INSTALLATION_DIR     ?= $(TIRTOS_INSTALLATION_DIR)/products/bios_6_40_02_27
UIA_INSTALLATION_DIR      ?= $(TIRTOS_INSTALLATION_DIR)/products/uia_2_00_01_34
NDK_INSTALLATION_DIR      ?= $(DEFAULT_INSTALLATION_DIR)/ndk_2_24_00_05_eng
TIVAWARE_INSTALLATION_DIR ?= $(TIRTOS_INSTALLATION_DIR)/products/TivaWare_C_Series-2.1.0.12573c

# Setting this variable to 1 causes only NDK base stack libraries to be built
BUILDMINSTACK_CONFIG     := BUILDMINSTACK=1

# wolfSSL settings
WOLFSSL_INSTALLATION_DIR ?= C:/wolfssl
WOLFSSL_TIRTOS_DIR = $(WOLFSSL_INSTALLATION_DIR)/tirtos

#
# Set XDCARGS to some of the variables above.  XDCARGS are passed
# to the XDC build engine... which will load tirtos.bld... which will
# extract these variables and use them to determine what to build and which
# toolchains to use.
#
# Note that not all of these variables need to be set to something valid.
# Unfortunately, since these vars are unconditionally assigned, your build line
# will be longer and more noisy than necessary (e.g., it will include CC_V5T
# assignment even if you're just building for C64P).
#
# Some background is here:
#     http://rtsc.eclipse.org/docs-tip/Command_-_xdc#Environment_Variables
#
XDCARGS= \
    TivaWareDir='$(TIVAWARE_INSTALLATION_DIR)'

ifeq ("$(CCS_BUILD)", "true")
    XDCARGS += \
       ti.targets.arm.elf.M4F='$(ti.targets.arm.elf.M4F)'
endif
ifeq ("$(IAR_BUILD)", "true")
	XDCARGS += \
	iar.targets.arm.M4F='$(iar.targets.arm.M4F)'
endif
ifeq ("$(GCC_BUILD)", "true")
	XDCARGS += \
	gnu.targets.arm.M4F='$(gnu.targets.arm.M4F)'
endif

export XDCARGS
#
# Set XDCPATH to contain necessary repositories.
#
XDCPATH = $(TIRTOS_INSTALLATION_DIR)/packages;$(UIA_INSTALLATION_DIR)/packages;$(NDK_INSTALLATION_DIR)/packages;$(BIOS_INSTALLATION_DIR)/packages;$(TIVAWARE_INSTALLATION_DIR);
export XDCPATH

#
# Set XDCOPTIONS.  Use -v for a verbose build.
#
#XDCOPTIONS=v
export XDCOPTIONS

#
# Set XDC executable command
# Note that XDCBUILDCFG points to the tirtos.bld file which uses
# the arguments specified by XDCARGS
#
XDC = $(XDCTOOLS_INSTALLATION_DIR)/xdc XDCARGS="$(XDCARGS)" XDCBUILDCFG=./tirtos.bld
XS =  $(XDCTOOLS_INSTALLATION_DIR)/xs

#######################################################
## Shouldn't have to modify anything below this line ##
#######################################################

help:
	@ echo Makefile to build components within TI-RTOS
	@ echo   goal              description
	@ echo  -----------------------------------------------------------------------------
	@ echo   all               Builds SYS/BIOS, NDK, UIA, and TI-RTOS drivers
	@ echo   drivers           Builds TI-RTOS drivers and other components in /packages
	@ echo   bios              Builds SYS/BIOS
	@ echo   ndk               Builds NDK
	@ echo   uia               Builds UIA
	@ echo   clean             Cleans SYS/BIOS, NDK, UIA, and TI-RTOS drivers
	@ echo   clean-drivers     Cleans TI-RTOS drivers and other components in /packages
	@ echo   clean-bios        Cleans SYS/BIOS
	@ echo   clean-ndk         Cleans NDK
	@ echo   clean-uia         Cleans UIA
	@ echo   examplesgen       Generates TI-RTOS examples and makefiles
	@ echo   help              Displays this description

all: bios ndk uia drivers

clean: clean-ndk clean-uia clean-drivers clean-bios

drivers:
	@ echo building tirtos packages ...
	@ $(XDC) -Pr ./packages

clean-drivers:
	@ echo cleaning tirtos packages ...
	@ $(XDC) clean -Pr ./packages

bios:
	@ echo building bios ...
	@ $(XDCTOOLS_INSTALLATION_DIR)/gmake -f $(BIOS_INSTALLATION_DIR)/bios.mak \
	  XDC_INSTALL_DIR=$(XDCTOOLS_INSTALLATION_DIR) \
	  BIOS_SMPENABLED=0 \
	  $(XDCARGS) -C $(BIOS_INSTALLATION_DIR)

clean-bios:
	@ echo cleaning bios ...
	@ $(XDCTOOLS_INSTALLATION_DIR)/gmake -f $(BIOS_INSTALLATION_DIR)/bios.mak \
	  XDC_INSTALL_DIR=$(XDCTOOLS_INSTALLATION_DIR) \
	  BIOS_SMPENABLED=0 \
	  -C $(BIOS_INSTALLATION_DIR) clean

ndk:
	@ echo building ndk ...
	@ $(XDCTOOLS_INSTALLATION_DIR)/gmake -f $(NDK_INSTALLATION_DIR)/ndk.mak \
	  XDC_INSTALL_DIR=$(XDCTOOLS_INSTALLATION_DIR) \
	  SYSBIOS_INSTALL_DIR=$(BIOS_INSTALLATION_DIR) \
	  $(BUILDMINSTACK_CONFIG) \
	  $(XDCARGS) -C $(NDK_INSTALLATION_DIR)

clean-ndk:
	@ echo cleaning ndk ...
	@ $(XDCTOOLS_INSTALLATION_DIR)/gmake -f $(NDK_INSTALLATION_DIR)/ndk.mak \
	  XDC_INSTALL_DIR=$(XDCTOOLS_INSTALLATION_DIR) \
	  SYSBIOS_INSTALL_DIR=$(BIOS_INSTALLATION_DIR) \
	  $(BUILDMINSTACK_CONFIG) \
	  -C $(NDK_INSTALLATION_DIR) clean

uia:
	@ echo building uia ...
	@ $(XDCTOOLS_INSTALLATION_DIR)/gmake -f $(UIA_INSTALLATION_DIR)/uia.mak \
	  XDC_INSTALL_DIR=$(XDCTOOLS_INSTALLATION_DIR) \
	  BIOS_INSTALL_DIR=$(BIOS_INSTALLATION_DIR) \
	  NDK_INSTALL_DIR=$(NDK_INSTALLATION_DIR) \
	  $(XDCARGS) -C $(UIA_INSTALLATION_DIR)

clean-uia:
	@ echo cleaning uia ...
	@ $(XDCTOOLS_INSTALLATION_DIR)/gmake -f $(UIA_INSTALLATION_DIR)/uia.mak \
	  XDC_INSTALL_DIR=$(XDCTOOLS_INSTALLATION_DIR) \
	  BIOS_INSTALL_DIR=$(BIOS_INSTALLATION_DIR) \
	  NDK_INSTALL_DIR=$(NDK_INSTALLATION_DIR) \
	  -C $(UIA_INSTALLATION_DIR) clean

wolfssl:
	@ echo building wolfssl ...
	@ $(XDCTOOLS_INSTALLATION_DIR)/gmake -f $(WOLFSSL_TIRTOS_DIR)/wolfssl.mak \
	  XDC_INSTALL_DIR=$(XDCTOOLS_INSTALLATION_DIR) \
	  SYSBIOS_INSTALL_DIR=$(BIOS_INSTALLATION_DIR) \
	  NDK_INSTALL_DIR=$(NDK_INSTALLATION_DIR) \
	  WOLFSSL_INSTALL_DIR=$(WOLFSSL_INSTALLATION_DIR) \
	  TIRTOS_INSTALLATION_DIR=$(TIRTOS_INSTALLATION_DIR) \
	  $(XDCARGS) -C $(WOLFSSL_TIRTOS_DIR)

clean-wolfssl:
	@ echo cleaning wolfssl ...
	@ $(XDCTOOLS_INSTALLATION_DIR)/gmake -f $(WOLFSSL_TIRTOS_DIR)/wolfssl.mak \
	  XDC_INSTALL_DIR=$(XDCTOOLS_INSTALLATION_DIR) \
	  SYSBIOS_INSTALL_DIR=$(BIOS_INSTALLATION_DIR) \
	  NDK_INSTALL_DIR=$(NDK_INSTALLATION_DIR) \
	  WOLFSSL_INSTALL_DIR=$(WOLFSSL_INSTALLATION_DIR) \
	  TIRTOS_INSTALLATION_DIR=$(TIRTOS_INSTALLATION_DIR) \
	  -C $(WOLFSSL_TIRTOS_DIR) clean

examplesgen:
ifneq ("$(DEST)","")
	@ echo generating examples in $(DEST) ...
ifeq ("$(CCS_BUILD)", "true")
	@ $(XS) examples.examplesgen \
		--tirtos "$(TIRTOS_INSTALLATION_DIR)" \
		--productGroup "TivaC" \
		--toolchain TI \
		--output "$(DEST)" \
		--xdctools="$(XDCTOOLS_INSTALLATION_DIR)" \
		--bios="$(BIOS_INSTALLATION_DIR)" \
		--uia="$(UIA_INSTALLATION_DIR)" \
		--ndk="$(NDK_INSTALLATION_DIR)" \
		--wolfssl="$(WOLFSSL_TIRTOS_DIR)" \
		--tivaware="$(TIVAWARE_INSTALLATION_DIR)" \
		--toolChaindir="$(ti.targets.arm.elf.M4F)"
endif
ifeq ("$(IAR_BUILD)", "true")
	@ $(XS) examples.examplesgen \
		--tirtos "$(TIRTOS_INSTALLATION_DIR)" \
		--productGroup "TivaC" \
		--toolchain IAR \
		--output "$(DEST)" \
		--xdctools="$(XDCTOOLS_INSTALLATION_DIR)" \
		--bios="$(BIOS_INSTALLATION_DIR)" \
		--uia="$(UIA_INSTALLATION_DIR)" \
		--ndk="$(NDK_INSTALLATION_DIR)" \
		--wolfssl="$(WOLFSSL_TIRTOS_DIR)" \
		--tivaware="$(TIVAWARE_INSTALLATION_DIR)" \
		--toolChaindir="$(iar.targets.arm.M4F)"
endif
ifeq ("$(GCC_BUILD)", "true")
	@ $(XS) examples.examplesgen \
		--tirtos "$(TIRTOS_INSTALLATION_DIR)" \
		--productGroup "TivaC" \
		--toolchain GNU \
		--output "$(DEST)" \
		--xdctools="$(XDCTOOLS_INSTALLATION_DIR)" \
		--bios="$(BIOS_INSTALLATION_DIR)" \
		--uia="$(UIA_INSTALLATION_DIR)" \
		--ndk="$(NDK_INSTALLATION_DIR)" \
		--wolfssl="$(WOLFSSL_TIRTOS_DIR)" \
		--tivaware="$(TIVAWARE_INSTALLATION_DIR)" \
		--toolChaindir="$(gnu.targets.arm.M4F)"
endif
	@ echo ***********************************************************
	@ echo Please refer to "Examples for TI-RTOS" section in the TI-RTOS
	@ echo "Getting Started Guide" for details on how to build and load the examples.
else
	@ echo Specify destination path like this: DEST="YOURPATH". Use UNIX style path "C:/examples"
endif
