

## Steps for building

1) Open Xilinx SDK with working directory in wolfssl-examples/Xilinx/FreeRTOS/
2) File->New->Board Support Package
    a) select "Specify"
    b) make "Project name" be ultrazed
    c) under "Target Hardware Specification" select "Browse.." and choose the file wolfssl-examples/Xilinx/FreeRTOS/ultrazed_wrapper.hdf"
    d) select "Finish"

3) In Xilinx Board Support Package Project
    a) select Board Supoprt Package OS as "freertos901_xilinx"
    b) leave default project name and target hardware as "ultrazed"
    c) select "Finish"

    d) check "lwip141" and "xilsecure" boxes
    c) change lwip141 "api_mode" value from "RAW API" to "SOCKET API"
    e) chang freertos901_xilinx->kernel_behavior->tick_rate from 100 to 1000
    f) increas heap size freertos901_xilinx->kernel_behavior->total_heap_size from 65536 to 200000
    g) select OK
4) File->New->Application Project
    a) set project name to "fsbl"
    b) select "next"
    c) select "Zynq MP FSBL" from "Available Templates"
    d) select "Finsih"
5) File->Import->General->Existing Projects into Workspace
    a) select "Browse..." and go to directory wolfssl-examples/Xilinx/FreeRTOS
    b) select "Finish"

## Build and Run wolfcrypt_test
1) right click on wolfcrypt_test project and select "Build Project"
2) select wolfcrypt_test so it is highlighted then select Xilinx Tools->Create Boot Image
3) select "Create Image"
4) put the files created in wolfssl-examples/Xilinx/FreeRTOS/wolfcrypt_test/bootimage/ onto an SD card and run it on the board

(Note if error -1303 comes up from when running the image, just restart the board. It is because the time was not set yet)
