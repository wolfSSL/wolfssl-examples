/* string.c
 *
 * Implementations of standard library functions to eliminate external dependencies.
 *
 *
 * Copyright (C) 2021 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSLL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include <stddef.h>
#include <string.h>
#include <uchar.h>


int islower(int c)
{
    return (c >= 'a' && c <= 'z');
}

int isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}

int tolower(int c)
{
    char16_t wideChar[2] = { (char16_t)c, L'\0' };

    StrLwr(wideChar);

    return (int)wideChar[0];
}

int toupper(int c)
{
    char16_t wideChar[2] = { (char16_t)c, L'\0' };

    StrUpr(wideChar);

    return (int)wideChar[0];
}

int isalpha(int c)
{
    return (isupper(c) || islower(c));
}

char *strcat(char *dest, const char *src)
{
    size_t i = 0;
    size_t j = strlen(dest);

    for (i = 0; i < strlen(src); i++) {
        dest[j++] = src[i];
    }
    dest[j] = '\0';

    return dest;
}

int strcmp(const char *s1, const char *s2)
{
    int diff = 0;
    diff = strcmpa(s1, s2);
    return diff;
}

int strcasecmp(const char *s1, const char *s2)
{
    int diff = 0;

    while (!diff && *s1) {
        diff = (int)*s1 - (int)*s2;

        if ((diff == 'A' - 'a') || (diff == 'a' - 'A'))
            diff = 0;

        s1++;
        s2++;
    }

    return diff;
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    int diff = 0;
    size_t i = 0;

    while (!diff && *s1) {
        diff = (int)*s1 - (int)*s2;

        if ((diff == 'A' - 'a') || (diff == 'a' - 'A'))
            diff = 0;

        s1++;
        s2++;
        if (++i > n)
            break;
    }
    return diff;
}

char *strncat(char *dest, const char *src, size_t n) {
    size_t dest_len = strlen(dest);
    size_t i;

    /* Copy at most n characters from src */
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[dest_len + i] = src[i];
    }

    /* Null-terminate the resulting string */
    dest[dest_len + i] = '\0';

    return dest;
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

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        dst[i] = src[i];
        if (src[i] == '\0')
            break;
    }

    return dst;
}

char *strcpy(char *dst, const char *src)
{
   size_t i = 0;

    while(1) {
        dst[i] = src[i];
        if (src[i] == '\0')
            break;
        i++;
    }

    return dst;
}

int memcmp(const void *_s1, const void *_s2, size_t n)
{
    return CompareMem(_s1, _s2, n);
}

void* memchr(void const *s, int c_in, size_t n)
{
    unsigned char c = (unsigned char)c_in;
    unsigned char *char_ptr = (unsigned char*)s;
    for (; n > 0; --n, ++char_ptr) {
        if (*char_ptr == c) {
            return (void*)char_ptr;
        }
    }
    return NULL;
}

size_t strlen(const char *s)
{
    return strlena(s);
}


size_t strnlen(const char *s, size_t maxlen)
{

    size_t len = 0;

    while (len < maxlen && s[len] != '\0') {
        len++;
    }

    return len;

}

void *memmove(void *dst, const void *src, size_t n)
{
    int i;
    if (dst == src)
        return dst;
    if (src < dst)  {
        const char *s = (const char *)src;
        char *d = (char *)dst;
        for (i = n - 1; i >= 0; i--) {
            d[i] = s[i];
        }
        return dst;
    } else {
        return memcpy(dst, src, n);
    }
}


char* strchr(const char *str, int c)
{
    while (*str != '\0') {
        if (*str == (char)c) {
            return (char *)str;  /* Return pointer to the character */
        }
        str++;
    }

    /* Check for the null terminator if c is '\0' */
    if (c == '\0') {
        return (char *)str;
    }

    return NULL;  /* Character not found */
}

double strtod(const char *str, char **endptr) {
    double result = 0.0;
    int sign = 1;
    double fraction = 0.0;
    int fractional_divisor = 1;
    int has_fraction = 0;

    /* Skip leading whitespace */
    while (*str == ' ' || *str == '\t' || *str == '\n') {
        str++;
    }

    /* Handle sign */
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    /* Convert integer part */
    while (*str >= '0' && *str <= '9') {
        result = result * 10.0 + (*str - '0');
        str++;
    }

    /* Check for fractional part */
    if (*str == '.') {
        str++;
        has_fraction = 1;

        /* Convert fractional part */
        while (*str >= '0' && *str <= '9') {
            fraction = fraction * 10.0 + (*str - '0');
            fractional_divisor *= 10;
            str++;
        }

        result += fraction / fractional_divisor;
    }

    /* Check for scientific notation */
    if (*str == 'e' || *str == 'E') {
        str++;
        int exp_sign = 1;
        int exponent = 0;

        if (*str == '-') {
            exp_sign = -1;
            str++;
        } else if (*str == '+') {
            str++;
        }

        while (*str >= '0' && *str <= '9') {
            exponent = exponent * 10 + (*str - '0');
            str++;
        }

        result *= pow(10, exp_sign * exponent);
    }

    /* Set endptr if provided */
    if (endptr) {
        *endptr = (char *)str;
    }

    return sign * result;
}