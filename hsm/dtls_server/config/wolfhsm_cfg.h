/*
 * Copyright (C) 2024 wolfSSL Inc.
 *
 * This file is part of wolfHSM.
 *
 * wolfhsm_cfg.h - wolfHSM configuration for DTLS server demo
 *
 * This configures the wolfHSM client to connect to the HSM server
 * for crypto operations.
 */

#ifndef WOLFHSM_CFG_H_
#define WOLFHSM_CFG_H_

#include "port/posix/posix_time.h"

/* Time function for wolfHSM */
#define WOLFHSM_CFG_PORT_GETTIME posixGetTime

/* Enable wolfHSM client */
#define WOLFHSM_CFG_ENABLE_CLIENT

/* DMA transport is enabled via Makefile -DWOLFHSM_CFG_DMA */
/* Debug output is enabled via Makefile when DEBUG=1 */

/* Verbose debug output (uncomment for detailed tracing) */
/* #define WOLFHSM_CFG_DEBUG_VERBOSE */

/* Enable hexdump utility */
#define WOLFHSM_CFG_HEXDUMP

/* Communication data length - must match server configuration */
#define WOLFHSM_CFG_COMM_DATA_LEN 5000

/* Enable key wrapping support */
#define WOLFHSM_CFG_KEYWRAP

/* Enable global keys feature */
#define WOLFHSM_CFG_GLOBAL_KEYS

#endif /* WOLFHSM_CFG_H_ */
