/* gen_key_files.c
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
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

#include <stdio.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/ssl.h>

#ifndef HAVE_ED25519
    #error "Please build wolfSSL with the --enable-ed25519 option"
#endif

int create_and_output_ed_key(void);
static int err_sys(const char* msg, int es);

static int err_sys(const char* msg, int es)
{
    printf("%s error = %d\n", msg, es);

    exit(-1);
}

int
main(int argc, char* argv[])
{
    int ret = -1001;
    FILE* file;
    char edPrivFName[] = "./ed_priv_test_key.der";
    char edPubFName[] = "./ed_pub_test_key.der";
    byte exportPrivKey[ED25519_KEY_SIZE*2];
    byte exportPubKey[ED25519_KEY_SIZE];
    word32 exportPrivSz;
    word32 exportPubSz;
    WC_RNG rng;
    ed25519_key edKeyOut;


    wolfSSL_Debugging_ON();
/*--------------- INIT ---------------------*/
    ret = wc_InitRng(&rng);
    if (ret != 0) err_sys("wc_InitRng err: ", ret); /* replace all go-to's with err_sys  */

    ret = wc_ed25519_init(&edKeyOut);
    if (ret != 0) err_sys("wc_ed25519_init err: ", ret);

/*--------------- MAKE KEY ---------------------*/
    ret = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, &edKeyOut);
    if (ret != 0) err_sys("wc_ed25519_make_key err: ", ret);

/*--------------- GET KEY SIZES ---------------------*/
    exportPrivSz = wc_ed25519_priv_size(&edKeyOut);
    if (exportPrivSz <= 0) err_sys("wc_ed25519_priv_size err: ", exportPrivSz);

    exportPubSz = wc_ed25519_pub_size(&edKeyOut);
    if (exportPubSz <= 0) err_sys("wc_ed25519_pub_size err: ", exportPubSz);

/*--------------- EXPORT KEYS TO BUFFERS  ---------------------*/
    ret = wc_ed25519_export_key(&edKeyOut, exportPrivKey, &exportPrivSz,
                                                     exportPubKey, &exportPubSz);
    if (ret != 0) err_sys("wc_ed25519_export_key err: ", ret);

/*--------------- OUTPUT KEYS TO FILES ---------------------*/
    file = fopen(edPrivFName, "wb");
    if (!file) err_sys("error opening edPrivFName file", -1002);

    ret = (int) fwrite(exportPrivKey, 1, exportPrivSz, file);
    if (ret <= 0) {
        fclose(file);
        err_sys("Failed to write to edPrivFName file", -1003);
    }

    fclose(file);

    file = fopen(edPubFName, "wb");
    if (!file) err_sys("error opening edPubFName file", -1004);

    ret = (int) fwrite(exportPubKey, 1, exportPubSz, file);
    if (ret <= 0) {
        fclose(file);
        err_sys("Failed to write to edPubFName file", -1005);
    }

    fclose(file);

    if (ret > 0) ret = 0;

    return ret;
}
