/* rh_string.c
 *
 * Copyright (C) 2006-2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */
#include "rh_string.h"

void PrintTextln(char* TextArray);

/* Provide a limit of the printf format string to limit execution time */
#define MAX_FMT_LEN 64
int snprintf(char* buffer, size_t bufsz, const char* format, ...);

static int _my_write(const char *buf, unsigned int sz);

static char *my_buffer = NULL;
static size_t *my_offset = 0;
static size_t my_size = 0;

typedef int (*write_func)(const char* buf, unsigned int sz);
static int my_writenum(write_func f, int num, int base);
static int _my_vprintf(write_func f, const char* fmt, va_list argp);

static void _set_string_write(char* buffer, unsigned int sz);
static int _string_write(const char* buf, unsigned int sz);

static char* _string_write_buffer = NULL;
static unsigned int _string_write_space = 0;
#if 0
int strcmp(const char *s1, const char *s2)
{
    int diff = 0;

    while (!diff && *s1) {
        diff = (int)*s1 - (int)*s2;
        s1++;
        s2++;
    }

    return diff;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    int diff = 0;

    while (n > 0) {
        diff = (unsigned char)*s1 - (unsigned char)*s2;
        if (diff || !*s1)
            break;
        s1++;
        s2++;
        n--;
    }

    return diff;
}
#endif

static void _set_string_write(char* buffer, unsigned int sz)
{
    _string_write_buffer = buffer;
    _string_write_space = sz;
}

static int _string_write(const char* buf, unsigned int sz)
{
    if (    (buf == NULL) ||
            (_string_write_buffer == NULL) ||
            (sz == 0) ||
            (_string_write_space == 0) ) {
        /* Nothing to do */
        return 0;
    }

    if (sz > _string_write_space) {
        sz = _string_write_space;
    }
    memcpy(_string_write_buffer, buf, sz);
    _string_write_buffer += sz;
    _string_write_space -= sz;
    return sz;
}

static int my_writenum(write_func f, int num, int base)
{
    int i = 0;
    char buf[sizeof(int)*2+1];
    const char* kDigitLut = "0123456789ABCDEF";
    unsigned int val = (unsigned int)num;
    int sz = 0;

    if(f == NULL) {
        return 0;
    }
    if (base == 10 && num < 0) { /* handle negative */
        buf[i++] = '-';
        val = -num;
    }
    do {
        buf[sizeof(buf)-sz-1] = kDigitLut[(val % base)];
        sz++;
        val /= base;
    } while (val > 0U);
    memmove(&buf[i], &buf[sizeof(buf)-sz], sz);
    i+=sz;
    return f(buf, i);
}

static int _my_vprintf(write_func f, const char* fmt, va_list argp)
{
    char* fmtp = (char*)fmt;
    char  c;
    int ret = 0;
#if defined(USE_RH_UART) || defined(RH_PRINTF)
    *my_offset = 0;
    memset(my_buffer, 0, my_size);
#endif
    if (f == NULL) {
        return 0;
    }

    while (fmtp != NULL && *fmtp != '\0' && ((fmtp - fmt)< MAX_FMT_LEN)) {
        /* print non formatting characters */
        if (*fmtp != '%') {
#if defined(USE_RH_UART)
        if (f == _my_write && (*fmtp == '\r' || *fmtp == '\n'))
            fmtp++;
        else
#endif
            ret += f(fmtp++, 1);
            continue;
        }

        fmtp++; /* skip % */

        /* find formatters */
        while (*fmtp != '\0') {
            if (*fmtp >= '0' && *fmtp <= '9') {
                /* length formatter keep only one digit */
                fmtp++;
            }
            else if (*fmtp == 'l') {
                /* long - skip */
                fmtp++;
            }
            else if (*fmtp == 'z') {
                /* auto type - skip */
                fmtp++;
            }
            else if (*fmtp == '-') {
                /* left indent - skip */
                fmtp++;
            }
            else if (*fmtp == '*') {
                /* valiable width */
                c = '0';
                int i;
                int w = (char)va_arg(argp, int);
                for(i = 0; i < w; i++) {
                    ret += f(&c, 1);
                }
                fmtp++;
            }
            else {
                break;
            }
        }

        switch (*fmtp) {
            case '%':
                ret += f(fmtp, 1);
                break;
            case 'u':
            case 'i':
            case 'd':
            {
                int n = (int)va_arg(argp, int);
                ret += my_writenum(f, n, 10);
                break;
            }
            case 'x':
            case 'p':
            {
                int n = (int)va_arg(argp, int);
                ret += my_writenum(f, n, 16);
                break;
            }
            case 's':
            {
                char* str = (char*)va_arg(argp, char*);
                ret += f(str, (uint32_t)strlen(str));
                break;
            }
            case 'c':
            {
                char c = (char)va_arg(argp, int);
                ret += f(&c, 1);
                break;
            }
            default:
                break;
        }
        fmtp++;
    }
#if defined(USE_RH_UART)
    if (f == _my_write) {
        my_buffer[*my_offset + 1] = '\0';
        PrintTextln(my_buffer);
    }
#elif defined(RH_PRINTF)
    if (f == _my_write) {
        my_buffer[*my_offset + 1] = '\0';
        dummy_printf(my_buffer);
    }
#endif
    return ret;
}

int snprintf(char* buffer, size_t bufsz, const char* format, ...)
{
    int ret = 0;
    va_list argp;
    va_start(argp,format);
    _set_string_write(buffer, bufsz);
    ret = _my_vprintf(_string_write, format, argp);
    va_end(argp);
    return ret;
}

int printf(const char* fmt, ...)
{
    int ret = 0;
    va_list argp;
    va_start(argp, fmt);
    ret = _my_vprintf(_my_write, fmt, argp);
    va_end(argp);
    return ret;
}

int hex_dump(uint32_t size, const uint8_t* data )
{
    int ret = 0;
    while (size > 0) {
        if (data[0] < 0x10) {
            ret += printf("0%x ", data[0]);
        } else {
            ret += printf("%x ");
        }
        data++;
        size--;
    }
    return ret;
}





/* Called by printf */
static int _my_write(const char *buf, unsigned int sz) {
    
    if (buf == NULL)
            return 0;
    if (my_buffer == NULL)
            return 0;
    if (my_offset == NULL)
            return 0;
    if (sz == 0)
            return 0;
    if (sz > my_size)
            return 0;
    unsigned int offset = 0;
    if (*my_offset + sz > my_size) {
        offset = my_size - *my_offset;
        memcpy(&my_buffer[*my_offset], buf, offset);
        sz -= offset;
        *my_offset = 0;
 #if defined(USE_RH_UART)
        my_buffer[my_size + 1] = '\0';
        PrintTextln(my_buffer);
 #elif defined(RH_PRINTF)
        my_buffer[my_size + 1] = '\0';
 #endif
    }
    memcpy(&my_buffer[*my_offset], buf + offset, sz);
    *my_offset += sz;
    return sz;
}

void rh_string_init(size_t size, size_t* offset, char* buffer)
{
    my_size = size;
    my_offset = offset;
    my_buffer = buffer;

    /* Reset my_offset to be valid */
    if (    (my_offset != NULL) &&
            (my_buffer != NULL) &&
            (my_size != 0)) {
        *my_offset = 0;
    }
}

int strncasecmp(const char *s1, const char * s2, unsigned int sz)
{
    for( ; sz>0; sz--, s1++, s2++){
            if(toupper(*s1) < toupper(*s2)){
            return -1;
        }
        if(toupper(*s1) > toupper(*s2)){
            return 1;
        }
    }
    return 0;
}