__Part 1: Download CCS__

(If you already have Code Composer Studio (CCS) installed skip to step 2)<br/>
Download CCS here: http://www.ti.com/tool/ccstudio<br/>
<br/>
Installer will walk you through setting up CCS. Choose options for your platform and desired compiler(s).<br/>
<br/>
expected install location: C:\ti<br/>
<br/>

----

__Part 2: Download tivac__

(If you already have TIRTOS for TivaC skip to step 3)<br/>
Now in Code Composer Studio, find CCS App Center it will be an option on the "Getting Started"<br/>
page or click View -> CCS App Center<br/>
Under the title "Code Composer Studio Add-ons" check the box under "TI-RTOS for TivaC"<br/>
Now on the top of the page click "Install Software"<br/>
<br/>
expected install location: C:\ti<br/>
<br/>

----

__Part 3: Download wolfSSL__

Download wolfSSL from here: https://wolfssl.com/wolfSSL/download/downloadForm.php<br/>
or<br/>
clone wolfSSL development branch from here: https://github.com/wolfSSL/wolfssl<br/>
<br/>
expected install location: C:\ti\wolfssl<br/>
<br/>

----

__Part 4: Build the libraries__

At this point we are ready to build wolfssl libraries with TIRTOS<br/>
<br/>
This process will be easiest if you set your environment up in the following<br/>
way.<br/>

`C:\ti`<br/>
`C:\ti\wolfssl`<br/>
`C:\ti\ccsv6`<br/>
`C:\ti\xdctools..`<br/>.
`C:\ti\tirtos_tivac...`<br/>

File: `C:\ti\wolfssl\tirtos\products.mak`<br/>
Instructions:<br/>
Copy and paste the following contents into the `C:\ti\wolfssl\tirtos\products.mak`<br/>
file then browse to C:/ti/ and update all lines with `"X_XX_XX_XX"` to the correct<br/>
version on your system. Also update the "X.X.X" and "X.X.X.XXx"<br/>

NOTE: DO NOT forget the `"_core"` on the end of `XDC_INSTALL_DIR` if it is present<br/>
in your environment, must be exact.<br/>
<br/>
################################################################################<br/>
Contents of products.mak BEGIN<br/>
################################################################################<br/>

```
#
#  ======== products.mak ========
#
#
# Read the http://processors.wiki.ti.com/index.php/Using_wolfSSL_with_TI-RTOS
# for instructions to download the software required.

export XDCTOOLS_JAVA_HOME = c:/ti/ccsv6/eclipse/jre

# XDC_INSTALL_DIR is the path to XDCtools directory.
XDC_INSTALL_DIR        = c:/ti/xdctools_X_XX_XX_XX_core

# BIOS_INSTALL_DIR is the path to TI-RTOS Kernel (SYS/BIOS) directory. If you
# have installed TI-RTOS, it is located in the products/bios_* path.
BIOS_INSTALL_DIR       = c:/ti/tirtos_tivac_X_XX_XX_XX/products/bios_X_XX_XX_XX

# NDK_INSTALL_DIR is the path to TI-RTOS NDK directory. If you have
# installed TI-RTOS, it is located in the products/ndk_* path.
NDK_INSTALL_DIR        = c:/ti/tirtos_tivac_X_XX_XX_XX/products/ndk_X_XX_XX_XX

# TIVAWARE_INSTALL_DIR is the path to Tivaware driverlib directory. If you have
# installed TI-RTOS, it is located in the products/TivaWare_* path.
TIVAWARE_INSTALL_DIR   = c:/ti/tirtos_tivac_X_XX_XX_XX/products/TivaWare_C_Series-X.X.X.XXx

# Define the code generation tools path for TI, IAR and GCC ARM compilers.
# If you have installed Code Composer Studio, the TI and GCC compiler are
# located in the ccsv*/tools/compiler/* path.
#
# Leave assignment empty to disable any toolchain.
ti.targets.arm.elf.M4F = c:/ti/ccsv6/tools/compiler/ti-cgt-arm_X.X.X
```

################################################################################<br/>
Contents of products.mak END<br/>
################################################################################<br/>

Finally from the wolfssl/tirtos directory run this command:<br/>
`..\..\xdctools_X_XX_XX_XX_core\gmake.exe -f wolfssl.mak`<br/>

<br/>
    The libraries should build without issues.<br/>
<br/>

----

__Part 5: Loading first example__

   In CCS click dropdown for "Project" and select "Examples..."<br/>
   <pre>
    Locate: TI-RTOS for TivaC<br/>
              -> Tiva C Series<br/>
                 -> Tiva TM4C1294NCPDT<br/>
                   -> EK-TM4C1294XL Evaluation Kit<br/>
                     -> Network Examples<br/>
                       -> TI Network Examples<br/>
                         -> Ethernet Examples<br/>
                           -> TCP Echo with TLS<br/>
</pre>
    In the "TI Resource Explorer window" Click on the link in "Step 1" to import the project.<br/>
    NOTE: Do not try to build the project yet. It will fail.<br/>
<br/>

----

__Part 6: Setting up the environment__


   Before building we have to do a little setup.<br/>
   <br/>
   <b>Part 6.a</b><br/>
        Right click on the imported project and select `Properties`<br/>
        <br/>
        Under `Build -> ARM Compiler -> Include Options`<br/>
        You will see a little file with a green "plus" symbol in the browser window<br/>
        (NOTE: include the quotes)<br/>
        Click on that and add this line `"C:/ti/wolfssl"`<br/>
   <br/>
   <b>Part 6.b</b><br/>
        Now still in the Properties window<br/>
        Under `Build -> ARM Linker -> File Search Path`<br/>
        <br/>
        In the browser window on the right under "Include library file or command file as input"<br/>
        Click the add button and add this line `"C:\ti\wolfssl\tirtos\packages\ti\net\wolfssl\lib\wolfssl.aem4f"`<br/>
        <br/>
        In the browser window on the right under "Add dir to library search path"<br/>
        Click the add button and add this line `"C:\ti\wolfssl\tirtos\packages\ti\net\wolfssl\lib"`<br/>
<br/>
  Hit OK and we're ready to build the example project.<br/>
<br/>

----

__Part 7: Build the project__

   In the "TI Resource Explorer window" Click on the link in "Step 2" to build the project.<br/>
<br/>

----

Part 8: Prepare for debugging

   (If you have already hooked up your board skip to step 9)<br/>
   Take a Micro-USB cable and plug the micro-USB end into the port on the Tiva Board labeled "DEBUG"<br/>
   (This is the Micro-USB port on the opposite end of the board from the Ethernet port)<br/>
   Insert the USB end into your PC<br/>
   Plug one end of an Ethernet cable into the Tiva board Ethernet port and the other end<br/>
   into your local network.<br/>
<br/>

----

__Part 9: Get Debug Working__

   In the "TI Resource Explorer window" Click on the link in "Step 3"<br/>
   Click the drop down options and select "Stellaris In-Circuit Debug Interface"<br/>
   <br/>
   In the "TI Resource Explorer window" Click on the link in "Step 4"<br/>
   This should alert you to the need for some additional setup. Click OK and a new tab will open.<br/>
   <br/>
   In the field "Board or Device" type in `Tiva TM4C1294NCPDT` and check the box<br/>
   <br/>
   In the right-hand side of this same window click "save" to save the configuration.<br/>
   Close this tab.<br/>
   <br/>
   In the "TI Resource Explorer window" Click on the link in "Step 4" a second time.<br/>
   <br/>
   The project will be flashed onto the Tiva Board. Wait for a few seconds.<br/>
   <br/>
   Now hit the key "F8" or in the top left of CCS click the green "Play" Button<br/>
   (The button says "Resume" in the drop down if you scroll over it)<br/>
   <br/>
   You will see output in the "Console" Window at the bottom of CCS.<br/>
   A line will read `Network Added: If-1:192.168.1.xxx` or some other IP<br/>
   <br/>
   This is the IP of the tcpEchoTLS server that is now running on the board in step 10<br/>
   we will refer to this IP as `TCP_ECHO_IP`<br/>
<br/>

----

__Part 10: Test the server__

   Option 1:<br/>
        To test the server here at wolfSSL we either build the client example in Microsoft Visual Studios<br/>
        and run it against the IP above and using port 1000 (the default port for the tcpEchoTLS example)<br/>
        Instructions for building the Visual Studio Client can be found here:
        http://wolfssl.com/wolfSSL/Docs-wolfssl-visual-studio.html<br/>
<br/>
    Option 2:<br/>
        Using Linux or Unix build our libraries and run the following command from the wolfssl root<br/>
        directory from any computer running on the local network<br/>
        `./examples/client/client -h TCP_ECHO_IP -p 1000`<br/>
<br/>
    The client should receive the following response:<br/>
    <br/>
    `SSL version is TLS1.2`<br/>
    `SSL cipher suite is <whichever suite is set>`<br/>
    `Server response: hello wolfssl!`<br/>
<br/>

----

__Part 11: Success__

   If you received that response your server is working correctly.<br/>
   You are now ready to begin development for your project!<br/>
<br/>
    Contact us anytime with questions:<br/>
    `info@wolfssl.com`<br/>
    `support@wolfssl.com`<br/>
<br/>
    
    
    
    
        
