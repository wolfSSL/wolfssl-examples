/*
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== EK_TM4C1294XL.c ========
 *  This file is responsible for setting up the board specific items for the
 *  EK_TM4C1294XL board.
 *
 *  The following defines are used to determine which TI-RTOS peripheral drivers
 *  to include:
 *     TI_DRIVERS_EMAC_INCLUDED
 *     TI_DRIVERS_GPIO_INCLUDED
 *     TI_DRIVERS_I2C_INCLUDED
 *     TI_DRIVERS_SDSPI_INCLUDED
 *     TI_DRIVERS_SPI_INCLUDED
 *     TI_DRIVERS_UART_INCLUDED
 *     TI_DRIVERS_USBMSCHFATFS_INCLUDED
 *     TI_DRIVERS_WATCHDOG_INCLUDED
 *     TI_DRIVERS_WIFI_INCLUDED
 *  These defines are created when a useModule is done on the driver in the
 *  application's .cfg file. The actual #define is in the application
 *  generated header file that is brought in via the xdc/cfg/global.h.
 *  For example the following in the .cfg file
 *     var GPIO = xdc.useModule('ti.drivers.GPIO');
 *  Generates the following
 *     #define TI_DRIVERS_GPIO_INCLUDED 1
 *  If there is no useModule of ti.drivers.GPIO, the constant is set to 0.
 *
 *  Note: a useModule is generated in the .cfg file via the graphical
 *  configuration tool when the "Add xxx to my configuration" is checked
 *  or "Use xxx" is selected.
 */

#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_ints.h>
#include <inc/hw_gpio.h>

#include <driverlib/gpio.h>
#include <driverlib/flash.h>
#include <driverlib/sysctl.h>
#include <driverlib/i2c.h>
#include <driverlib/ssi.h>
#include <driverlib/udma.h>
#include <driverlib/pin_map.h>

#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/family/arm/m3/Hwi.h>

#include "EK_TM4C1294XL.h"

#if defined(ccs)
#pragma DATA_ALIGN(EK_TM4C1294XL_DMAControlTable, 1024)
#elif defined(ewarm)
#pragma data_alignment=1024
#elif defined(gcc)
__attribute__ ((aligned (1024)))
#endif
static tDMAControlTable EK_TM4C1294XL_DMAControlTable[32];
static Bool DMA_initialized = false;

/* Hwi_Struct used in the initDMA Hwi_construct call */
static Hwi_Struct dmaHwiStruct;

/* Hwi_Struct used in the usbBusFault Hwi_construct call */
static Hwi_Struct usbBusFaultHwiStruct;

/*
 *  ======== EK_TM4C1294XL_errorDMAHwi ========
 */
static Void EK_TM4C1294XL_errorDMAHwi(UArg arg)
{
    System_printf("DMA error code: %d\n", uDMAErrorStatusGet());
    uDMAErrorStatusClear();
    System_abort("DMA error!!");
}

/*
 *  ======== EK_TM4C1294XL_usbBusFaultHwi ========
 */
static Void EK_TM4C1294XL_usbBusFaultHwi(UArg arg)
{
    /*
     *  This function should be modified to appropriately manage handle
     *  a USB bus fault.
    */
    System_printf("USB bus fault detected.");
    Hwi_clearInterrupt(INT_GPIOQ4);
    System_abort("USB error!!");
}

/*
 *  ======== EK_TM4C1294XL_initDMA ========
 */
void EK_TM4C1294XL_initDMA(void)
{
    Error_Block eb;
    Hwi_Params  hwiParams;

    if(!DMA_initialized){

        Error_init(&eb);

        Hwi_Params_init(&hwiParams);
        Hwi_construct(&(dmaHwiStruct), INT_UDMAERR, EK_TM4C1294XL_errorDMAHwi,
                      &hwiParams, &eb);
        if (Error_check(&eb)) {
            System_abort("Couldn't construct DMA error hwi");
        }

        SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
        uDMAEnable();
        uDMAControlBaseSet(EK_TM4C1294XL_DMAControlTable);

        DMA_initialized = true;
    }
}

/*
 *  ======== EK_TM4C1294XL_initGeneral ========
 */
void EK_TM4C1294XL_initGeneral(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOR);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOS);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOT);
}

#if TI_DRIVERS_EMAC_INCLUDED
#include <ti/drivers/EMAC.h>
#include <ti/drivers/emac/EMACSnow.h>

/*
 *  Required by the Networking Stack (NDK). This array must be NULL terminated.
 *  This can be removed if NDK is not used.
 *  Double curly braces are needed to avoid GCC bug #944572
 *  https://bugs.launchpad.net/gcc-linaro/+bug/944572
 */
NIMU_DEVICE_TABLE_ENTRY  NIMUDeviceTable[2] = {{EMACSnow_NIMUInit}, {NULL}};

/* EMAC objects */
EMACSnow_Object emacObjects[EK_TM4C1294XL_EMACCOUNT];

/*
 *  EMAC configuration structure
 *  Set user/company specific MAC octates. The following sets the address
 *  to ff-ff-ff-ff-ff-ff. Users need to change this to make the label on
 *  their boards.
 */
unsigned char macAddress[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

const EMACSnow_HWAttrs emacHWAttrs[EK_TM4C1294XL_EMACCOUNT] = {
    {EMAC0_BASE, INT_EMAC0, macAddress}
};

const EMAC_Config EMAC_config[] = {
    {
        &EMACSnow_fxnTable,
        &emacObjects[0],
        &emacHWAttrs[0]
    },
    {NULL, NULL, NULL}
};

/*
 *  ======== EK_TM4C1294XL_initEMAC ========
 */
void EK_TM4C1294XL_initEMAC(void)
{
    uint32_t ulUser0, ulUser1;

    /* Get the MAC address */
    FlashUserGet(&ulUser0, &ulUser1);
    if ((ulUser0 != 0xffffffff) && (ulUser1 != 0xffffffff)) {
        System_printf("Using MAC address in flash\n");
        /*
         *  Convert the 24/24 split MAC address from NV ram into a 32/16 split MAC
         *  address needed to program the hardware registers, then program the MAC
         *  address into the Ethernet Controller registers.
         */
        macAddress[0] = ((ulUser0 >>  0) & 0xff);
        macAddress[1] = ((ulUser0 >>  8) & 0xff);
        macAddress[2] = ((ulUser0 >> 16) & 0xff);
        macAddress[3] = ((ulUser1 >>  0) & 0xff);
        macAddress[4] = ((ulUser1 >>  8) & 0xff);
        macAddress[5] = ((ulUser1 >> 16) & 0xff);
    }
    else if (macAddress[0] == 0xff && macAddress[1] == 0xff &&
             macAddress[2] == 0xff && macAddress[3] == 0xff &&
             macAddress[4] == 0xff && macAddress[5] == 0xff) {
        System_abort("Change the macAddress variable to match your boards MAC sticker");
    }

    GPIOPinConfigure(GPIO_PF0_EN0LED0);  /* EK_TM4C1294XL_USR_D3 */
    GPIOPinConfigure(GPIO_PF4_EN0LED1);  /* EK_TM4C1294XL_USR_D4 */
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);


    /* Once EMAC_init is called, EMAC_config cannot be changed */
    EMAC_init();
}
#endif /* TI_DRIVERS_EMAC_INCLUDED */

#if TI_DRIVERS_GPIO_INCLUDED
#include <ti/drivers/GPIO.h>

/* Callback functions for the GPIO interrupt example. */
void gpioButtonFxn0(void);
void gpioButtonFxn1(void);

/* GPIO configuration structure */
const GPIO_HWAttrs gpioHWAttrs[EK_TM4C1294XL_GPIOCOUNT] = {
    {GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_OUTPUT}, /* EK_TM4C1294XL_USR_D1 */
    {GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_OUTPUT}, /* EK_TM4C1294XL_USR_D2 */
    {GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_INPUT},  /* EK_TM4C1294XL_USR_SW1 */
    {GPIO_PORTJ_BASE, GPIO_PIN_1, GPIO_INPUT}   /* EK_TM4C1294XL_USR_SW2 */
};

/* Memory for the GPIO module to construct a Hwi */
Hwi_Struct callbackPortJHwi;

const GPIO_Callbacks EK_TM4C1294XL_gpioPortJCallbacks = {
    GPIO_PORTJ_BASE, INT_GPIOJ, &callbackPortJHwi,
    {gpioButtonFxn0, gpioButtonFxn1, NULL, NULL, NULL, NULL, NULL, NULL}
};

const GPIO_Config GPIO_config[] = {
    {&gpioHWAttrs[0]},
    {&gpioHWAttrs[1]},
    {&gpioHWAttrs[2]},
    {&gpioHWAttrs[3]},
    {NULL},
};

/*
 *  ======== EK_TM4C1294XL_initGPIO ========
 */
void EK_TM4C1294XL_initGPIO(void)
{
    /* Setup the LED GPIO pins used */
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* Setup the button GPIO pins used */
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);

    /* Once GPIO_init is called, GPIO_config cannot be changed */
    GPIO_init();
}
#endif /* TI_DRIVERS_GPIO_INCLUDED */

#if TI_DRIVERS_I2C_INCLUDED
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CTiva.h>

/* I2C objects */
I2CTiva_Object i2cTivaObjects[EK_TM4C1294XL_I2CCOUNT];

/* I2C configuration structure, describing which pins are to be used */
const I2CTiva_HWAttrs i2cTivaHWAttrs[EK_TM4C1294XL_I2CCOUNT] = {
    {I2C0_BASE, INT_I2C0},
    {I2C2_BASE, INT_I2C2},
    {I2C7_BASE, INT_I2C7},
    {I2C8_BASE, INT_I2C8}
};

const I2C_Config I2C_config[] = {
    {&I2CTiva_fxnTable, &i2cTivaObjects[0], &i2cTivaHWAttrs[0]},
    {&I2CTiva_fxnTable, &i2cTivaObjects[1], &i2cTivaHWAttrs[1]},
    {&I2CTiva_fxnTable, &i2cTivaObjects[2], &i2cTivaHWAttrs[2]},
    {&I2CTiva_fxnTable, &i2cTivaObjects[2], &i2cTivaHWAttrs[3]},
    {NULL, NULL, NULL}
};

/*
 *  ======== EK_TM4C1294XL_initI2C ========
 */
void EK_TM4C1294XL_initI2C(void)
{
    /* I2C0 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    /* I2C2 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PN5_I2C2SCL);
    GPIOPinConfigure(GPIO_PN4_I2C2SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTN_BASE, GPIO_PIN_5);
    GPIOPinTypeI2C(GPIO_PORTN_BASE, GPIO_PIN_4);

    /* I2C7 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C7);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PD0_I2C7SCL);
    GPIOPinConfigure(GPIO_PD1_I2C7SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
    GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);

    /* I2C8 Init */
    /* Enable the peripheral */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C8);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    GPIOPinConfigure(GPIO_PA2_I2C8SCL);
    GPIOPinConfigure(GPIO_PA3_I2C8SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_2);
    GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_3);

    I2C_init();
}
#endif /* TI_DRIVERS_I2C_INCLUDED */

#if TI_DRIVERS_SDSPI_INCLUDED
#include <ti/drivers/SDSPI.h>
#include <ti/drivers/sdspi/SDSPITiva.h>

/* SDSPI objects */
SDSPITiva_Object sdspiTivaobjects[EK_TM4C1294XL_SDSPICOUNT];

/* SDSPI configuration structure, describing which pins are to be used */
const SDSPITiva_HWAttrs sdspiTivaHWattrs[EK_TM4C1294XL_SDSPICOUNT] = {
    {
        SSI2_BASE,          /* SPI base address */

        GPIO_PORTD_BASE,    /* The GPIO port used for the SPI pins */
        GPIO_PIN_3,         /* SCK */
        GPIO_PIN_0,         /* MISO */
        GPIO_PIN_1,         /* MOSI */

        GPIO_PORTC_BASE,    /* Chip select port */
        GPIO_PIN_7,         /* Chip select pin */

        GPIO_PORTB_BASE,    /* GPIO TX port */
        GPIO_PIN_1,         /* GPIO TX pin */
    },
    {
        SSI3_BASE,          /* SPI base address */

        GPIO_PORTQ_BASE,    /* The GPIO port used for the SPI pins */
        GPIO_PIN_0,         /* SCK */
        GPIO_PIN_3,         /* MISO */
        GPIO_PIN_2,         /* MOSI */

        GPIO_PORTP_BASE,    /* Chip select port */
        GPIO_PIN_4,         /* Chip select pin */

        GPIO_PORTQ_BASE,    /* GPIO TX port */
        GPIO_PIN_2,         /* GPIO TX pin */
    }
};

const SDSPI_Config SDSPI_config[] = {
    {&SDSPITiva_fxnTable, &sdspiTivaobjects[0], &sdspiTivaHWattrs[0]},
    {&SDSPITiva_fxnTable, &sdspiTivaobjects[1], &sdspiTivaHWattrs[1]},
    {NULL, NULL, NULL}
};

/*
 *  ======== EK_TM4C1294XL_initSDSPI ========
 */
void EK_TM4C1294XL_initSDSPI(void)
{
    /* SDSPI0 configuration */
    /* Enable the peripherals used by the SD Card */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);

    /* Configure pad settings */
    GPIOPadConfigSet(GPIO_PORTD_BASE,
                     GPIO_PIN_3 | GPIO_PIN_1,
                     GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);

    GPIOPadConfigSet(GPIO_PORTD_BASE,
                     GPIO_PIN_0,
                     GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOPadConfigSet(GPIO_PORTC_BASE,
                     GPIO_PIN_7,
                     GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);

    GPIOPinConfigure(GPIO_PD3_SSI2CLK);
    GPIOPinConfigure(GPIO_PD0_SSI2XDAT1);
    GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);

    /* SDSPI1 configuration */
    /* Enable the peripherals used by the SD Card */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);

    /* Configure pad settings */
    GPIOPadConfigSet(GPIO_PORTQ_BASE,
                     GPIO_PIN_0 | GPIO_PIN_2,
                     GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);

    GPIOPadConfigSet(GPIO_PORTQ_BASE,
                     GPIO_PIN_3,
                     GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOPadConfigSet(GPIO_PORTP_BASE,
                     GPIO_PIN_4,
                     GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);

    GPIOPinConfigure(GPIO_PQ0_SSI3CLK);
    GPIOPinConfigure(GPIO_PQ3_SSI3XDAT1);
    GPIOPinConfigure(GPIO_PQ2_SSI3XDAT0);

    /*
     *  These GPIOs are connected to PA2 and PA3 and need to be brought into a
     *  GPIO input state so they don't interfere with SPI communications.
     */
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_2);
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_3);

    SDSPI_init();
}
#endif /* TI_DRIVERS_SDSPI_INCLUDED */

#if TI_DRIVERS_SPI_INCLUDED
#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPITivaDMA.h>

/* SPI objects */
SPITivaDMA_Object spiTivaDMAobjects[EK_TM4C1294XL_SPICOUNT];
#if defined(ccs)
#pragma DATA_ALIGN(spiTivaDMAscratchBuf, 32)
#elif defined(ewarm)
#pragma data_alignment=32
#elif defined(gcc)
__attribute__ ((aligned (32)))
#endif
uint32_t spiTivaDMAscratchBuf[EK_TM4C1294XL_SPICOUNT];

/* SPI configuration structure, describing which pins are to be used */
const SPITivaDMA_HWAttrs spiTivaDMAHWAttrs[EK_TM4C1294XL_SPICOUNT] = {
    {
        SSI2_BASE,
        INT_SSI2,
        &spiTivaDMAscratchBuf[0],
        0,
        UDMA_SEC_CHANNEL_UART2RX_12,
        UDMA_SEC_CHANNEL_UART2TX_13,
        uDMAChannelAssign,
        UDMA_CH12_SSI2RX,
        UDMA_CH13_SSI2TX
    },
    {
        SSI3_BASE,
        INT_SSI3,
        &spiTivaDMAscratchBuf[1],
        0,
        UDMA_SEC_CHANNEL_TMR2A_14,
        UDMA_SEC_CHANNEL_TMR2B_15,
        uDMAChannelAssign,
        UDMA_CH14_SSI3RX,
        UDMA_CH15_SSI3TX
    }
};

const SPI_Config SPI_config[] = {
    {&SPITivaDMA_fxnTable, &spiTivaDMAobjects[0], &spiTivaDMAHWAttrs[0]},
    {&SPITivaDMA_fxnTable, &spiTivaDMAobjects[1], &spiTivaDMAHWAttrs[1]},
    {NULL, NULL, NULL}
};

/*
 *  ======== EK_TM4C1294XL_initSPI ========
 */
void EK_TM4C1294XL_initSPI(void)
{
    /* SSI2 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);

    GPIOPinConfigure(GPIO_PD3_SSI2CLK);
    GPIOPinConfigure(GPIO_PD2_SSI2FSS);
    GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
    GPIOPinConfigure(GPIO_PD0_SSI2XDAT1);

    GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 |
                                    GPIO_PIN_2 | GPIO_PIN_3);

    /* SSI3 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);

    GPIOPinConfigure(GPIO_PQ0_SSI3CLK);
    GPIOPinConfigure(GPIO_PQ1_SSI3FSS);
    GPIOPinConfigure(GPIO_PQ2_SSI3XDAT0);
    GPIOPinConfigure(GPIO_PQ3_SSI3XDAT1);

    GPIOPinTypeSSI(GPIO_PORTQ_BASE, GPIO_PIN_0 | GPIO_PIN_1 |
                                    GPIO_PIN_2 | GPIO_PIN_3);

    EK_TM4C1294XL_initDMA();
    SPI_init();
}
#endif /* TI_DRIVERS_SPI_INCLUDED */

#if TI_DRIVERS_UART_INCLUDED
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTTiva.h>

/* UART objects */
UARTTiva_Object uartTivaObjects[EK_TM4C1294XL_UARTCOUNT];

/* UART configuration structure */
const UARTTiva_HWAttrs uartTivaHWAttrs[EK_TM4C1294XL_UARTCOUNT] = {
    {UART0_BASE, INT_UART0}, /* EK_TM4C1294XL_UART0 */
    {UART2_BASE, INT_UART2}, /* EK_TM4C1294XL_UART2 */
    {UART6_BASE, INT_UART6}, /* EK_TM4C1294XL_UART6 */
    {UART7_BASE, INT_UART7}  /* EK_TM4C1294XL_UART7 */
};

const UART_Config UART_config[] = {
    {&UARTTiva_fxnTable, &uartTivaObjects[0], &uartTivaHWAttrs[0]},
    {&UARTTiva_fxnTable, &uartTivaObjects[1], &uartTivaHWAttrs[1]},
    {&UARTTiva_fxnTable, &uartTivaObjects[2], &uartTivaHWAttrs[2]},
    {&UARTTiva_fxnTable, &uartTivaObjects[3], &uartTivaHWAttrs[3]},
    {NULL, NULL, NULL}
};

/*
 *  ======== EK_TM4C1294XL_initUART ========
 */
void EK_TM4C1294XL_initUART(void)
{
    /* Enable and configure the peripherals used by the UART0 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* Enable and configure the peripherals used by the UART2 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    GPIOPinConfigure(GPIO_PD4_U2RX);
    GPIOPinConfigure(GPIO_PD5_U2TX);
    GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    /* Enable and configure the peripherals used by the UART6 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART6);
    GPIOPinConfigure(GPIO_PP0_U6RX);
    GPIOPinConfigure(GPIO_PP1_U6TX);
    GPIOPinTypeUART(GPIO_PORTP_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    /* Enable and configure the peripherals used by the UART7 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART7);
    GPIOPinConfigure(GPIO_PC4_U7RX);
    GPIOPinConfigure(GPIO_PC5_U7TX);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    /* Initialize the UART driver */
    UART_init();
}
#endif /* TI_DRIVERS_UART_INCLUDED */

/*
 *  ======== EK_TM4C1294XL_initUSB ========
 *  This function just turns on the USB
 */
void EK_TM4C1294XL_initUSB(EK_TM4C1294XL_USBMode usbMode)
{
    Error_Block eb;
    Hwi_Params  hwiParams;

    /* Enable the USB peripheral and PLL */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
    SysCtlUSBPLLEnable();

    /* Setup pins for USB operation */
    GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    /* Additional configurations for Host mode */
    if (usbMode == EK_TM4C1294XL_USBHOST) {
        /* Configure the pins needed */
        HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
        HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = 0xff;
        GPIOPinConfigure(GPIO_PD6_USB0EPEN);
        GPIOPinTypeUSBDigital(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);

        /*
         *  USB bus fault is routed to pin PQ4.  We create a Hwi to allow us
         *  to detect power faults and recover gracefully or terminate the
         *  program.  PQ4 is active low; set the pin as input with a weak
         *  pull-up.
         */
        GPIOPadConfigSet(GPIO_PORTQ_BASE, GPIO_PIN_4,
                         GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
        GPIOIntTypeSet(GPIO_PORTQ_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
        GPIOIntClear(GPIO_PORTQ_BASE, GPIO_PIN_4);

        /* Create a Hwi for PQ4 pin. */
        Error_init(&eb);
        Hwi_Params_init(&hwiParams);
        Hwi_construct(&(usbBusFaultHwiStruct), INT_GPIOQ4,
                      EK_TM4C1294XL_usbBusFaultHwi, &hwiParams, &eb);
        if (Error_check(&eb)) {
            System_abort("Couldn't construct USB bus fault hwi");
        }
    }
}

#if TI_DRIVERS_USBMSCHFATFS_INCLUDED
#include <ti/drivers/USBMSCHFatFs.h>
#include <ti/drivers/usbmschfatfs/USBMSCHFatFsTiva.h>

/* USBMSCHFatFs objects */
USBMSCHFatFsTiva_Object usbmschfatfstivaObjects[EK_TM4C1294XL_USBMSCHFatFsCOUNT];

/* USBMSCHFatFs configuration structure, describing which pins are to be used */
const USBMSCHFatFsTiva_HWAttrs usbmschfatfstivaHWAttrs[EK_TM4C1294XL_USBMSCHFatFsCOUNT] = {
    {INT_USB0}
};

const USBMSCHFatFs_Config USBMSCHFatFs_config[] = {
    {
        &USBMSCHFatFsTiva_fxnTable,
        &usbmschfatfstivaObjects[0],
        &usbmschfatfstivaHWAttrs[0]
    },
    {NULL, NULL, NULL}
};

/*
 *  ======== EK_TM4C1294XL_initUSBMSCHFatFs ========
 */
void EK_TM4C1294XL_initUSBMSCHFatFs(void)
{
    /* Initialize the DMA control table */
    EK_TM4C1294XL_initDMA();

    /* Call the USB initialization function for the USB Reference modules */
    EK_TM4C1294XL_initUSB(EK_TM4C1294XL_USBHOST);
    USBMSCHFatFs_init();
}
#endif /* TI_DRIVERS_USBMSCHFATFS_INCLUDED */

#if TI_DRIVERS_WATCHDOG_INCLUDED
#include <ti/drivers/Watchdog.h>
#include <ti/drivers/watchdog/WatchdogTiva.h>

/* Watchdog objects */
WatchdogTiva_Object watchdogTivaObjects[EK_TM4C1294XL_WATCHDOGCOUNT];

/* Watchdog configuration structure */
const WatchdogTiva_HWAttrs watchdogTivaHWAttrs[EK_TM4C1294XL_WATCHDOGCOUNT] = {
    /* EK_LM4F120XL_WATCHDOG0 with 1 sec period at default CPU clock freq */
    {WATCHDOG0_BASE, INT_WATCHDOG, 80000000},
};

const Watchdog_Config Watchdog_config[] = {
    {&WatchdogTiva_fxnTable, &watchdogTivaObjects[0], &watchdogTivaHWAttrs[0]},
    {NULL, NULL, NULL},
};

/*
 *  ======== EK_TM4C1294XL_initWatchdog ========
 *
 * NOTE: To use the other watchdog timer with base address WATCHDOG1_BASE,
 *       an additional function call may need be made to enable PIOSC. Enabling
 *       WDOG1 does not do this. Enabling another peripheral that uses PIOSC
 *       such as ADC0 or SSI0, however, will do so. Example:
 *
 *       SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
 *       SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG1);
 *
 *       See the following forum post for more information:
 *       http://e2e.ti.com/support/microcontrollers/stellaris_arm_cortex-m3_microcontroller/f/471/p/176487/654390.aspx#654390
 */
void EK_TM4C1294XL_initWatchdog(void)
{
    /* Enable peripherals used by Watchdog */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);

    /* Initialize the Watchdog driver */
    Watchdog_init();
}
#endif /* TI_DRIVERS_WATCHDOG_INCLUDED */

#if TI_DRIVERS_WIFI_INCLUDED
#include <ti/drivers/WiFi.h>

#if TI_DRIVERS_WIFI_CC3100
#include <ti/drivers/wifi/WiFiCC3100.h>

/* WiFi objects */
WiFiCC3100_Object wiFiCC3100Objects[EK_TM4C1294XL_WIFICOUNT];

/* WiFi configuration structure */
const WiFiCC3100_HWAttrs wiFiCC3100HWAttrs[EK_TM4C1294XL_WIFICOUNT] = {
    {
        GPIO_PORTM_BASE, /* IRQ port */
        GPIO_PIN_3,      /* IRQ pin */
        INT_GPIOM,       /* IRQ port interrupt vector */

        GPIO_PORTH_BASE, /* CS port */
        GPIO_PIN_2,      /* CS pin */

        GPIO_PORTC_BASE, /* WLAN EN port */
        GPIO_PIN_6       /* WLAN EN pin */
    }
};

const WiFi_Config WiFi_config[] = {
    {
        &WiFiCC3100_fxnTable,
        &wiFiCC3100Objects[0],
        &wiFiCC3100HWAttrs[0]
    },
    {NULL,NULL, NULL},
};

/*
 *  ======== EK_TM4C1294XL_initWiFi ========
 */
void EK_TM4C1294XL_initWiFi(void)
{
    /* Configure EN & CS pins to disable CC3100 */
    GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, GPIO_PIN_2);
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_6);
    GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_2, GPIO_PIN_2);
    GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0);

    /* Configure SSI2 for CC3100 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
    GPIOPinConfigure(GPIO_PD3_SSI2CLK);
    GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
    GPIOPinConfigure(GPIO_PD0_SSI2XDAT1);
    GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3);

    /* Configure IRQ pin */
    GPIOPinTypeGPIOInput(GPIO_PORTM_BASE, GPIO_PIN_3);
    GPIOPadConfigSet(GPIO_PORTM_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPD);
    GPIOIntTypeSet(GPIO_PORTM_BASE, GPIO_PIN_3, GPIO_RISING_EDGE);

    SPI_init();
    EK_TM4C1294XL_initDMA();

    WiFi_init();
}
#endif /* TI_DRIVERS_WIFI_CC3100 */

#endif /* TI_DRIVERS_WIFI_INCLUDED */
