Part 1: Download CCS
--
(If you already have Code Composer Studio (CCS) installed skip to step 2)<br/>
Download CCS here: http://www.ti.com/tool/ccstudio<br/>
<br/>
Installer will walk you through setting up CCS. Choose options for your platform and desired compiler(s).<br/>
<br/>
expected install location: C:\ti<br/>
<br/>
Part 2: Download tivac
--
(If you already have TIRTOS for TivaC skip to step 3)<br/>
Now in Code Composer Studio, find CCS App Center it will be an option on the "Getting Started"<br/>
page or click View -> CCS App Center<br/>
Under the title "Code Composer Studio Add-ons" check the box under "TI-RTOS for TivaC"<br/>
Now on the top of the page click "Install Software"<br/>
<br/>
expected install location: C:\ti<br/>
<br/>
Part 3: Download wolfSSL
--
Download wolfSSL from here: https://wolfssl.com/wolfSSL/download/downloadForm.php<br/>
or<br/>
clone wolfSSL development branch from here: https://github.com/wolfSSL/wolfssl<br/>
<br/>
expected install location: C:\wolfssl<br/>
<br/>
Part 4: Build the libraries
--
At this point we are ready to build wolfssl libraries with TIRTOS<br/>
<br/>
WINDOWS:<br/>
    In the file browser navigate to: `C:\ti\ccsv6\tools\compiler` and see which<br/>
    compiler you have installed `ti-cgt-arm_x.x.x` and make a note of it.<br/>
    <br/>
    Now in the file browser navigate to `C:\ti\tirtos_tivac_xxxxxxx` open `tirtos.mak` in your<br/>
    preferred text editor<br/>
    <br/>
    Locate the line specifying the compiler. You should find this on or around line 11 and it will look<br/>
    something like this:<br/>
    `ti.targets.arm.elf.M4F   ?= $(DEFAULT_INSTALLATION_DIR)/ccsv6/tools/compiler/ti-cgt-arm_5.2.5`<br/>
    <br/>
    Make sure you are using the `ti-cgt-agrm_x.x.x` that you noted previously<br/>
    <br/>
    Next locate the wolfSSL section should be on or around line 45 and looks like this:<br/>
    `WolfSSL settings`<br/>
    `WOLFSSL_INSTALLATION_DIR ?= C:\wolfssl`<br/>
    `WOLFSSL_TIRTOS_DIR = $(WOLFSSL_INSTALLATION_DIR)/tirtos`<br/>
    <br/>
    Modify WOLFSSL_INSTALLATION_DIR to be the location of your wolfssl directory. Should be C:\wolfssl.<br/>
    <br/>
    Open a command promp and navigate to `C:\ti\tirtos_tivac_xxxxxxx`<br/>
    <br/>
    Use the following command to build TIRTOS and wolfssl<br/>
    `..\xdctools_3_31_01_33_core\gmake.exe -f tirtos.mak wolfssl`<br/>
<br/>
    The libraries should build without issues.<br/>
<br/>
Part 5: Loading first example
--
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
<br/>
Part 6: Setting up the environment
--
   Before building we have to do a little setup.<br/>
   <br/>
   <b>Part 6.a</b><br/>
        Right click on the imported project and select `Properties`<br/>
        <br/>
        Under `Build -> ARM Compiler -> Include Options`<br/>
        You will see a little file with a green "plus" symbol in the browser window<br/>
        (NOTE: include the quotes)<br/>
        Click on that and add this line `"C:/wolfssl"`<br/>
   <br/>
   <b>Part 6.b</b><br/>
        Now still in the Properties window<br/>
        Under `Build -> ARM Linker -> File Search Path`<br/>
        <br/>
        In the browser window on the right under "Include library file or command file as input"<br/>
        Click the add button and add this line `"C:\wolfssl\tirtos\packages\ti\net\wolfssl\lib\wolfssl.aem4f"`<br/>
        <br/>
        In the browser window on the right under "Add dir to library search path"<br/>
        Click the add button and add this line `"C:\wolfssl\tirtos\packages\ti\net\wolfssl\lib"`<br/>
<br/>
  Hit OK and we're ready to build the example project.<br/>
<br/>
Part 7: Build the project
--
   In the "TI Resource Explorer window" Click on the link in "Step 2" to build the project.<br/>
<br/>
Part 8: Prepare for debugging
--
   (If you have already hooked up your board skip to step 9)<br/>
   Take a Micro-USB cable and plug the micro-USB end into the port on the Tiva Board labeled "DEBUG"<br/>
   (This is the Micro-USB port on the opposite end of the board from the Ethernet port)<br/>
   Insert the USB end into your PC<br/>
   Plug one end of an Ethernet cable into the Tiva board Ethernet port and the other end<br/>
   into your local network.<br/>
<br/>
Part 9: Get Debug Working
--
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
Part 10: Test the server
--
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
Part 11: Success
--
   If you received that response your server is working correctly.<br/>
   You are now ready to begin development for your project!<br/>
<br/>
    Contact us anytime with questions:<br/>
    `info@wolfssl.com`<br/>
    `support@wolfssl.com`<br/>
<br/>
    
    
    
    
        
