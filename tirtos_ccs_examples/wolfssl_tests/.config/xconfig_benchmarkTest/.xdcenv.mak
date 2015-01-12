#
_XDCBUILDCOUNT = 
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = C:/wolfssl/tirtos/packages;C:/ti/ndk_2_24_00_05_eng/packages;C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc;C:/ti/tirtos_tivac_2_00_02_36/packages;C:/ti/tirtos_tivac_2_00_02_36/products/bios_6_40_02_27/packages;C:/ti/tirtos_tivac_2_00_02_36/products/ndk_2_23_01_01/packages;C:/ti/tirtos_tivac_2_00_02_36/products/uia_2_00_01_34/packages;C:/ti/ccsv6/ccs_base;C:/Users/khimes/workspace_v6_0/benchmarkTest/.config
override XDCROOT = c:/ti/xdctools_3_30_01_25_core
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = C:/wolfssl/tirtos/packages;C:/ti/ndk_2_24_00_05_eng/packages;C:/ti/ndk_2_24_00_05_eng/packages/ti/ndk/inc;C:/ti/tirtos_tivac_2_00_02_36/packages;C:/ti/tirtos_tivac_2_00_02_36/products/bios_6_40_02_27/packages;C:/ti/tirtos_tivac_2_00_02_36/products/ndk_2_23_01_01/packages;C:/ti/tirtos_tivac_2_00_02_36/products/uia_2_00_01_34/packages;C:/ti/ccsv6/ccs_base;C:/Users/khimes/workspace_v6_0/benchmarkTest/.config;c:/ti/xdctools_3_30_01_25_core/packages;..
HOSTOS = Windows
endif
