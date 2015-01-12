Disclaimer
---------------------------------------------------------
These have currently been tested in windows CCS only. Once testing on the linux distribution for CCS is complete this README will be updated.

Setting up the file system
---------------------------------------------------------
ensure that wolfssl is on the C:/ drive and remove any version numbers on the file. For example if your wolfssl directory is named "wolfssl_3_1_0 or wolfssl-3.1.0" rename it to "wolfssl". You should now have C:/wolfssl directory.

Clone this repository using "github for windows". The projects have been made portable so you can run them straight from the github clone repo or you can copy and paste the "tirtos_ccs_examples" folder out of the repo to wherever and 
they should still debug just fine once imported into your workspace (see end of this README).

Instructions for building wolfSSL application for TI-RTOS
---------------------------------------------------------

Installing the software
-----------------------
1. Install the NDK product ndk 2.24.00.05 in C:/ti
2. Open CCS. A prompt opens asking to install discovered product. Select the ndk product and click finish. Restart if prompted.
3. In CCS, open View -> CCS App Center. Search "ti-rtos". Select "TI-RTOS for TivaC" from the results and click Install Software. Follow the instructions.


Building wolfSSL libraries for TI-RTOS
-------------------------------------
1. Replace tirtos.mak in C:\ti\tirtos_tivac_2_00_02_36 with the tirtos.mak from git@github.com:wolfSSL/wolfssl.git. Ensure you have wolfssl in the proper location (should be C:/wolfssl).
2. Open a command prompt. CD into C:\ti\tirtos_tivac_2_00_02_36.
3. Run the make command to build wolfSSL libaries.
    
   ..\xdctools_3_30_01_25_core\gmake.exe -f tirtos.mak wolfssl

Running wolfSSL CTaoCrypt benchmark and test application.
--------------------------------------------------------
1. After running the TCP Echo Server application, in the Debugger View, open Run -> Load Program. Browse to the Benchmark application (C:\wolfssl\tirtos\packages\ti\net\wolfssl\tests\wolfcrypt\benchmark). Select benchmark.xem4f and Click Open and Click OK to load the application.
2. Test by running the application.
3. Repeat the steps 1 and 2 for Test application (C:\wolfssl\tirtos\packages\ti\net\wolfssl\tests\wolfcrypt\test).
4. The example wolfssl_tirtos_benchmark in this repository has all the correct optimization settings and will re-build the relevant c files at compile time for optimal results and additional benchmarks for camellia, rabbit, chacha, poly1305, sha512, and ECC.

Import a project into your workspace.
--------------------------------------------------------
In CCS go to the Project tab. Click "Import CCS Projects...". 
Click "Browse..." and navigate to tirtos_ccs_examples/ file and click "OK". In the "Discovered projects" window you will see all CCS projects. Check the ones you would like to import into the workspace, and check the box at the bottom labled "Copy projects into workspace", now click "Finish". Build and debug the desired projects. 
