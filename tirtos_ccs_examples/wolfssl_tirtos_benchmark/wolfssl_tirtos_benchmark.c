/*
 * This file contains contributions from both TI and wolfSSL
 *
 * Copyright (c) 2014, Texas Instruments Incorporated
 *
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
 *
 * Copyright (c) 2006-2020, wolfSSL Inc.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 *    ======== benchmarkTest.c ========
 */

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Swi.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/hal/Timer.h>

static int initialized = 0;
static double msTicks = 0;
static Timer_Handle hdl = NULL;

double current_time(int reset);
static void tick(unsigned int arg0);
void msTimer_init(void);
void runBenchmarks(UArg arg0, UArg arg1);
extern int benchmark_test(void* args);

/*
 *  ======== runBenchmarks ========
 *  Run the wolfSSL benchmark application
 */
void runBenchmarks(UArg arg0, UArg arg1)
{
    void *args = NULL;
    msTimer_init();

    System_printf("Running benchmarks...\n");
    System_flush();
    benchmark_test(args);
    System_printf("Benchmarks completed.\n");

    BIOS_exit(0);
}

/*
 *  ======== ticks ========
 *  Keeps track of time in millisec
 */
static void tick(unsigned int arg0)
{
    Swi_disable();
    msTicks++;
    Swi_enable();
}

/*
 *  ======== current_time ========
 *  Returns the time in sec (double precision)
 */
double current_time(int reset)
{
    if (reset) {
        msTicks = 0;
    }

    return (msTicks/1000);
}

/*
 *  ======== msTimer_init ========
 *  Sets up a BIOS timer with millisec period
 */
void msTimer_init(void)
{
    Timer_Params params;

    if (initialized) {
        return;
    }

    Timer_Params_init(&params);
    params.period = 1000;
    params.periodType = Timer_PeriodType_MICROSECS;
    params.runMode = Timer_RunMode_CONTINUOUS;
    params.startMode = Timer_StartMode_AUTO;
    hdl = Timer_create(-1, (ti_sysbios_hal_Timer_FuncPtr)tick,
             &params, NULL);
    if (!hdl) {
        System_abort("msTimer_init: Timer creation failed.\n");
    }

    /* Set flag indicating that initialization has completed */
    initialized = 1;
}

/*
 *  ======== main ========
 */
int main(int argc, char** argv)
{
    /* Initialize the defaults and set the parameters. */
    Task_Handle handle;
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = 65535;
    handle = Task_create(runBenchmarks, &taskParams, NULL);
    if (handle == NULL) {
        System_printf("main: Failed to create new Task.\n");
    }

    BIOS_start();
}
