### Steps to setup

- Download the NXP SDK for the board, in this case SDK_2.12.1_MIMXRT1170-EVK
(use the NXP SDK builder for creating the SDK https://mcuxpresso.nxp.com)
. Note to make sure that the SDK contains armgcc build examples.
- unzip / untar the SDK into this example directory
- make sure that arm cross compiler is available (on ubuntu this is sudo apt-get install gcc-arm-none-eabi)
- set the ARMGCC_DIR env. with `export ARMGCC_DIR=/usr`

### Steps that the build.sh script does

(note that you can rung build.sh to handle these steps)

- place wolfssl directory into the example directory
- patch CAAM driver files for ECC/Blob use with wolfssl/IDE/MCUEXPRESSO/RT1170/fsl_caam_c.patch and wolfssl/IDE/MCUEXPRESSO/RT1170/fsl_caam_h.patch
- mkdir build && cd build
- copy over example linker files from boards/evkmimxrt1170/rtos_examples/freertos_hello/cm7/armgcc/ 
- cmake .. -DCMAKE_TOOLCHAIN_FILE=../tools/cmake_toolchain_files/armgcc.cmake  -DCMAKE_C_FLAGS=-I../wolfssl/IDE/MCUEXPRESSO/RT1170/ -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=debug
- make

### Run the application
The resulting elf file will be placed in ./build/debug/
    
MCUExpresso documentation goes over multiple ways to flash an elf to the board, please see the documentation in `<SDK>/docs/*` for guidance.


### Results of application

```
Successfully created a new key

Exporting Key
DER of Key : 3077 2 1 1 420C61B5B3B7C8CEB D2DFC6217BCEA6F4686A9 52C 2892F6EEB4525EECDD6D9BFA0 A 6 82A8648CE3D 3 1 7A144 342 0 443BA77DCB38136 53758CBB35926DCEAD08E 997996184636DB23E43 099167BAC193BADE5A4 E1BDED17441 F247835DD363C624B2442771AA8 D8F3F77B8D7
BLACK BLOB [80] :D0858F1E12E68BEE3D17F9567E3E 9DD772B83F388E45725 06C8086 8BD4A8C55C15E82928618D821ECA7298C6B4D3290F37DE042EE471F1184EC4EEC918D83 8B6C4BEAFA8BF63 73598D5DFFEC528

CSR : 3082 14B3081F2 2 1 030818F31 B30 9 6 355 4 613 2555331 B30 9 6 355 4 8 C 24D54311030 E 6 355 4 7 C 7426F7A656D616E311030 E 6 355 4 A C 7776F6C6653534C31143012 6 355 4 B C B446576656C6F706D656E7431183016 6 355 4 3 C F7777772E776F6C6673736C2E636F6D311F301D 6 92A864886F7 D 1 9 11610696E666F40776F6C6673736C2E636F6D30593013 6 72A8648CE3D 2 1 6 82A8648CE3D 3 1 7 342 0 443BA77DCB38136 53758CBB35926DCEAD08E 997996184636DB23E43 099167BAC193BADE5A4 E1BDED17441 F247835DD363C624B2442771AA8 D8F3F77B8D7A0 030 A 6 82A8648CE3D 4 3 2 348 03045 2203DE519A7942DF8607A613D90A432F7B8BAB8813B12 4442D137B 7DA A10779D 221 0B9E2 5B08DB0AC49B8AD45258460 E 2BC9CEB98CD1D69697856BF 88E4AAA78
Done
```
