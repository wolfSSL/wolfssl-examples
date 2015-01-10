Instructions for building CyaSSL application for TI-RTOS
---------------------------------------------------------

Installing the software
-----------------------
1. Install the NDK product (http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/tmp/ndk_2_24_00_05_eng.zip) in C:/ti 
2. Open CCS. A prompt opens asking to install discovered product. Select the ndk product and click finish. Restart if prompted.
3. In CCS, open View -> CCS App Center. Search "ti-rtos". Select "TI-RTOS for TivaC" from the results and click Install Software. Follow the instructions.


Building CyaSSL libraries for TI-RTOS
-------------------------------------
1. Replace tirtos.mak in C:\ti\tirtos_tivac_2_00_02_36 with the attached tirtos.mak. Open the tirtos.mak and set CYASSL_INSTALLATION_DIR variable to the correct CyaSSL installation path.
2. Open a command prompt. CD into C:\ti\tirtos_tivac_2_00_02_36.
3. Run the make command to build CyaSSL libaries.
    
   ..\xdctools_3_30_01_25_core\gmake.exe -f tirtos.mak cyassl

4. The libraries should be build without any errors.

Building CyaSSL application (TCP Echo Server application) for TI-RTOS
---------------------------------------------------------------------
1. In CCS, open View -> Resource Explorer.
2. Browse to Tiva TM4C1294NCPDT under TI-RTOS for TivaC.
3. Select  TCP Echo examples under Driver Examples -> EK-TM4C1294 -> TI Target Examples -> Ethernet Examples.
4. From the right panel. Click on Import the example project into CCS.
5. Now, before building the example. We need to do a bunch of steps to set it up for CyaSSL.
   a. In the imported project. Replace the tcpEcho.c and tcpEcho.cfg with attached tcpEcho.c and tcpEcho.cfg.
   b. Right click on the imported project. Select Properties. 
      i.  Under General -> RTSC. Select NDK and 2.24.0.05-eng.
      ii. Under Build -> ARM Compiler -> Include Options. Add the following include paths
          "${NDK_INSTALL_DIR}/packages/ti/ndk/inc"
          "${NDK_INSTALL_DIR}/packages/ti/ndk/inc/bsd"
          "C:/cyassl"  /* Change this if the CyaSSL install path is different */
      iii. Under Build -> ARM Compiler -> Include Options -> Advanced Options -> Predefined Symbols. Add TIRTOS to the defines.
      iv.  Under Build -> XDCtools -> Package Repositories. Add following repositories to xdcpath and move them up to the top of the existing repository paths.
           "${NDK_INSTALL_DIR}/packages"
           "${NDK_INSTALL_DIR}/packages/ti/ndk/inc"
           "C:/cyassl/tirtos/packages" /* Change this if the CyaSSL install path is different */
   c. Click OK.
6. Follow the steps 2, 3 and 4 in TI Resource Explorer to Build, Debugger Configuration and Debug.
7. In the Debugger View, run the application. The IP of the server will be printed on the console.
8. Run the echoclient application of CyaSSL SSL tutorial. Connect to the server using the IP.
9. Test by sending text which should be echoed back.

Running CyaSSL CTaoCrypt benchmark and test application.
--------------------------------------------------------
The CTaoCrypt benchmark and test applications are built along with the CyaSSL libraries. To run them follow the steps.

1. After running the TCP Echo Server application, in the Debugger View, open Run -> Load Program. Browse to the Benchmark application (C:\cyassl\tirtos\packages\ti\net\cyassl\tests\ctaocrypt\benchmark). Select benchmark.xem4f and Click Open and Click OK to load the application.
2. Test by running the application.
3. Repeat the steps 1 and 2 for Test application (C:\cyassl\tirtos\packages\ti\net\cyassl\tests\ctaocrypt\test).


