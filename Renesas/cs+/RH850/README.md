# wolfCrypt Examples on Renesas RH850

This repository contains the Renesas RH850 examples.

# Building wolfCrypt and sample code

Before you begin, make sure you have the necessary tools and libraries installed.
## Tools and Libraries
| Tool/Library| Version| Description||
|:--|:--|:--|:--|
|Board||Renesas RH850/F1KM-S4||
|Device||DR7F701649||
|Toolchain|V2.06.00  [28 Nov 2023]|CC-RH RH850 Family Compiler||
|IDE|V8.11.00 [30 Nov 2023]|CS+ for CC||
|<a id="sw_package"></a> Software Package||[Y-ASK-RH850F1KM-S4-V3 Software Package](https://www.renesas.com/en/document/sws/y-ask-rh850f1km-s4-v3-software-package?r=1261056)||

### Prepare boot.asm, cstart.asm and iodefine.h
1. Open CS+
2. Menu `File` -> `Create New Project`. Select `R7F01649`. Input `Dummy_Project` as project name. Click `Create` button.
3. Copy `boot.asm`, `cstart.asm` and `iodefine.h` to `wolfssl-examples\Renesas\cs+\RH850\rsapss_sign_verify`

### Prepare modules and peripherals by [Y-ASK-RH850F1KM-S4-V3 Software Package](https://www.renesas.com/en/document/sws/y-ask-rh850f1km-s4-v3-software-package?r=1261056)
1. Download the package from [https://www.renesas.com/en/document/sws/y-ask-rh850f1km-s4-v3-software-package?r=1261056](https://www.renesas.com/en/document/sws/y-ask-rh850f1km-s4-v3-software-package?r=1261056)
2. Unzip the package
3. Copy the folder `Y-ASK-RH850F1KM-S4-V3_sample_V101\source\modules and peripherals` to the folder `<wolfssl-examples>\Renesas\cs+\RH850`
4. Copy the folder `Y-ASK-RH850F1KM-S4-V3_sample_V101\device\r_typedefs.h, r_device.h, intVecNumF1KM.h and dr7f701649.dvf.h` to the folder `<wolfssl-examples>\Renesas\cs+\RH850`

### Build RSA PSS Sign and Verify Sample
1. Open [CS+](#DeviceFile) project file under `<wolfssl-examples>\Renesas\cs+\RH850\rsapss_sign_verify\rh850_rsapss_sign.mtpj`
2. Modify `boot.asm`
   2-1. Uncomment `USE_TABLE_REFERENCE_METHOD` so that the sample program is able to use table reference for interrupt
   2-2. Change `section "EINTTBL", const` as follows:
   ```
   .section "EIINTTBL", const
	.align	512
	.offset (0x22*4)
	.dw	#_INTRLIN30UR0
	.offset (0x51*4)
	.dw	#_INTTAUJ0I1
	.offset (0x54*4)
	.dw	#_INTCOSTM0
	;.dw	#_Dummy_EI ; INT0
	;.dw	#_Dummy_EI ; INT1
	;.dw	#_Dummy_EI ; INT2
	;.rept	512 - 0x54
	;.dw	#_Dummy_EI ; INTn
	;.endm
    ```
    2-3. Specify RAM addresses as follows:
    ```
    GLOBAL_RAM_ADDR	.set	0xFEEE8000
	GLOBAL_RAM_END	.set	0xFEEE7FFF
	LOCAL_RAM_ADDR	.set	0xFEBC0000
    LOCAL_RAM_END	.set	0xFEBFFFFF
    ```
3. Modify `cstart.asm`
   3-1. Change `STACKSIZE` from `0x200` to `0XA000`

2. Build project, Menu `Build` -> `Build Project`
3. Download the project to target device, `Debug` -> `Download...`

The result can be observed through UART. Please connect to the cable to the board and open terminal with the following properties.

|||
|:-|:-|
|baud rate | 9600|
|data | 8|
|parity | none|
|stop bit | 1|
|flow control | none|

Example output
```
System Initializetion finish.
RSA Sign and Verify Example
Hashing message: This is the string to be signed
Signing hash of message
Hashing message: This is the string to be signed
Verify hash of message
RSA PSS verify success
```
