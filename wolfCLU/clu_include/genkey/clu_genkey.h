/* clu_genkey.h
 *
 * Copyright (C) 2006-2017 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
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

#ifndef CLU_GENKEY_H
#define CLU_GENKEY_H

#include <wolfssl/options.h>
#ifdef HAVE_ED25519
    #include <wolfssl/wolfcrypt/ed25519.h>
#endif

#define SALT_SIZE       8

enum {
    PRIV_ONLY,
    PUB_ONLY,
    PRIV_AND_PUB
};

/* handles incoming arguments for certificate generation */
int wolfCLU_genKeySetup(int argc, char** argv);

#ifdef HAVE_ED25519
/* Generate an ED25519 key */
int wolfCLU_genKey_ED25519(WC_RNG* rng, char* fOutNm, int directive,
                                                                    int format);
#endif

/* Generate an ECC key */
int wolfCLU_genKey_ECC(void);

/* Generate an RSA key */
int wolfCLU_genKey_RSA(void);

/* generates key based on password provided 
 * 
 * @param rng the random number generator
 * @param pwdKey the password based key as provided by the user
 * @param size size as determined by wolfCLU_GetAlgo
 * @param salt the buffer to store the resulting salt after it's generated
 * @param pad a flag to let us know if there are padded bytes or not
 */
int wolfCLU_genKey_PWDBASED(RNG* rng, byte* pwdKey, int size, byte* salt,
                                                                       int pad);

#endif /* CLU_GENKEY_H */
