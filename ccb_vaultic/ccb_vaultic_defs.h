/*
 * ccb_vaultic_defs.h
 *
 * Copyright (C) 2023 wolfSSL Inc.
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

#ifndef CCB_VAULTIC_DEFS_H
#define CCB_VAULTIC_DEFS_H

/* VaultIC DevID MSBs are ASCII "VI" */
#define CCBVAULTIC_DEVID (0x56490000ul)
#define CCBVAULTIC420_DEVID (CCBVAULTIC_DEVID + 0x0420)

/* Key/Group ID's to support temporary wolfSSL usage */
#define CCBVAULTIC_WOLFSSL_GRPID 0xBB
#define CCBVAULTIC_TMPAES_KEYID 0x01
#define CCBVAULTIC_TMPHMAC_KEYID 0x02
#define CCBVAULTIC_TMPRSA_KEYID 0x03

#endif
