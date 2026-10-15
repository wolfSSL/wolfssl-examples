/* c2000_board.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* Minimal board support for the standalone SHA-2 / SHA-3 examples on the TI
 * LAUNCHXL-F28P55X: clocks, SCIA console and a CPU Timer 0 time base.  The
 * same code lives in wolf_main.c for the full test/benchmark image.
 *
 * Output goes to SCIA (GPIO28/29, XDS110 virtual COM, 115200 8N1) when built
 * with WOLF_C2000_SCI_STDOUT (the Makefile default), otherwise to the CCS
 * console over JTAG (CIO=1).
 */

#include "driverlib.h"
#include "device.h"

#include <stdio.h>
#include <stdint.h>
#ifdef WOLF_C2000_SCI_STDOUT
#include <file.h>
#endif

#include "c2000_board.h"

#ifdef WOLF_C2000_SCI_STDOUT
static void c2000_sciInit(void)
{
    /* RX pin */
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    /* TX pin */
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);

    SCI_performSoftwareReset(SCIA_BASE);
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 115200,
                  (SCI_CONFIG_WLEN_8 | SCI_CONFIG_STOP_ONE |
                   SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);
}

/* Minimal CGT low-level I/O device that writes to SCIA, so stdout can be
 * redirected with add_device()/freopen().  Only write() does real work. */
int    WOLF_open(const char *path, unsigned flags, int llv_fd);
int    WOLF_close(int dev_fd);
int    WOLF_read(int dev_fd, char *buf, unsigned count);
int    WOLF_write(int dev_fd, const char *buf, unsigned count);
fpos_t WOLF_lseek(int dev_fd, fpos_t offset, int origin);
int    WOLF_unlink(const char *path);
int    WOLF_rename(const char *old_name, const char *new_name);

int WOLF_open(const char *path, unsigned flags, int llv_fd)
{
    (void)path;
    (void)flags;
    (void)llv_fd;
    return 0;
}

int WOLF_close(int dev_fd)
{
    (void)dev_fd;
    return 0;
}

int WOLF_read(int dev_fd, char *buf, unsigned count)
{
    (void)dev_fd;
    (void)buf;
    (void)count;
    return 0;
}

int WOLF_write(int dev_fd, const char *buf, unsigned count)
{
    unsigned i;
    (void)dev_fd;
    for (i = 0; i < count; i++) {
        if (buf[i] == '\n') {
            SCI_writeCharBlockingFIFO(SCIA_BASE, (uint16_t)'\r');
        }
        SCI_writeCharBlockingFIFO(SCIA_BASE, (uint16_t)(buf[i] & 0xFF));
    }
    return (int)count;
}

fpos_t WOLF_lseek(int dev_fd, fpos_t offset, int origin)
{
    (void)dev_fd;
    (void)offset;
    (void)origin;
    return (fpos_t)0;
}

int WOLF_unlink(const char *path)
{
    (void)path;
    return 0;
}

int WOLF_rename(const char *old_name, const char *new_name)
{
    (void)old_name;
    (void)new_name;
    return 0;
}

static void c2000_stdoutToSci(void)
{
    add_device("wolf", _SSA, WOLF_open, WOLF_close, WOLF_read, WOLF_write,
               WOLF_lseek, WOLF_unlink, WOLF_rename);
    freopen("wolf:", "w", stdout);
}
#endif /* WOLF_C2000_SCI_STDOUT */

/* CPU Timer 0: free-running 32-bit down-counter at SYSCLK. */
static void c2000_timerInit(void)
{
    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_setPeriod(CPUTIMER0_BASE, 0xFFFFFFFFUL);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0U);
    CPUTimer_setEmulationMode(CPUTIMER0_BASE,
                              CPUTIMER_EMULATIONMODE_RUNFREE);
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    CPUTimer_startTimer(CPUTIMER0_BASE);
}

/* A single measured interval must be shorter than 2^32 / SYSCLK (~28.6 s at
 * 150 MHz) for the wrap handling to be exact. */
double current_time(int reset)
{
    static unsigned long long accum = 0;
    static uint32_t last = 0;
    static int started = 0;
    uint32_t now;

    now = CPUTimer_getTimerCount(CPUTIMER0_BASE);

    if (!started || reset) {
        accum = 0;
        last = now;
        started = 1;
        return 0.0;
    }

    if (last >= now) {
        accum += (unsigned long long)(last - now);
    }
    else {
        /* counter reloaded once since the last sample */
        accum += (unsigned long long)last +
                 (0xFFFFFFFFULL - (unsigned long long)now) + 1ULL;
    }
    last = now;

    return (double)accum / (double)DEVICE_SYSCLK_FREQ;
}

void c2000_board_init(void)
{
    /* PLL to 150 MHz, flash wait states, disable watchdog, enable clocks. */
    Device_init();
    Device_initGPIO();
#ifdef WOLF_C2000_SCI_STDOUT
    c2000_sciInit();
    c2000_stdoutToSci();
#endif
    c2000_timerInit();

    /* Unbuffered stdout so output appears immediately and survives a crash. */
    setvbuf(stdout, NULL, _IONBF, 0);
}
