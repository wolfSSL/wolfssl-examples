/*
 * Copyright (C) 2024 wolfSSL Inc.
 *
 * This file is part of wolfHSM.
 *
 * wolfHSM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfHSM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with wolfHSM.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * wolfhsm_cfg.h
 *
 * wolfHSM compile-time options for DTLS client example
 */

#ifndef WOLFHSM_CFG_H_
#define WOLFHSM_CFG_H_

#include "wolfhsm/port/posix/posix_time.h"

#define WOLFHSM_CFG_PORT_GETTIME posixGetTime

/** wolfHSM settings */
#define WOLFHSM_CFG_ENABLE_CLIENT
#define WOLFHSM_CFG_COMM_DATA_LEN 5000

#endif /* WOLFHSM_CFG_H_ */
