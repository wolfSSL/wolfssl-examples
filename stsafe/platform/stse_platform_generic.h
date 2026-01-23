/******************************************************************************
 * \file    stse_platform_generic.h
 * \brief   STSecureElement platform types for Linux/Raspberry Pi
 * \author  wolfSSL Inc.
 *
 ******************************************************************************
 * \attention
 *
 * Copyright 2025 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 ******************************************************************************
 */

#ifndef STSE_PLATFORM_GENERIC_H
#define STSE_PLATFORM_GENERIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Platform type definitions for Linux */
#define PLAT_UI8    uint8_t
#define PLAT_UI16   uint16_t
#define PLAT_UI32   uint32_t
#define PLAT_UI64   uint64_t
#define PLAT_I8     int8_t
#define PLAT_I16    int16_t
#define PLAT_I32    int32_t
#define PLAT_PACKED_STRUCT __attribute__((packed))

/* __WEAK attribute for overridable functions */
#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

#ifdef __cplusplus
}
#endif

#endif /* STSE_PLATFORM_GENERIC_H */
