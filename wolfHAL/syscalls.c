/* syscalls.c
 *
 * Minimal libc stubs for bare-metal. Routes stdout through wolfHAL UART.
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
 */

#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#include "wolfHAL_board.h"

/* From the board's linker script: the heap runs from the end of .bss up to
 * where the stack is reserved. */
extern uint32_t _ebss;
extern uint32_t _heap_limit;

static char *heap_end;

int _write(int file, const char *ptr, int len)
{
    (void)file;
    if (len > 0) {
        whal_Uart_Send(&g_whalUart, ptr, (size_t)len);
        if (ptr[len - 1] == '\n')
            whal_Uart_Send(&g_whalUart, "\r", 1);
    }
    return len;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    if (st == NULL) {
        errno = EINVAL;
        return -1;
    }
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

void *_sbrk(ptrdiff_t incr)
{
    char *prev;

    if (heap_end == 0)
        heap_end = (char *)&_ebss;
    prev = heap_end;
    if ((heap_end + incr) >= (char *)&_heap_limit) {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_end += incr;
    return prev;
}

void _exit(int status)
{
    (void)status;
    while (1) { }
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}

void _init(void)
{
}

void _fini(void)
{
}
