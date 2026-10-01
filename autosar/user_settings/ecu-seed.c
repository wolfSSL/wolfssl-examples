/* ecu-seed.c
 *
 * Copyright (C) 2006-2026 wolfSSL Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* The entropy source for this build -- the one piece of the port an integrator
 * always has to supply.
 *
 * user_settings.h defines NO_FILESYSTEM, which is right for an ECU and has a
 * consequence that is easy to miss: it compiles out wolfSSL's own /dev/urandom
 * reader (random.c guards it with #ifndef NO_FILESYSTEM). Without a replacement
 * the DRBG cannot seed, and every Csm_RandomGenerate() returns E_NOT_OK -- at
 * runtime, with no build error to warn you. CUSTOM_RAND_GENERATE_SEED is how
 * you plug the gap.
 *
 * What is below reads the host's /dev/urandom directly so that this example can
 * be built and run on a workstation. THIS IS THE PART YOU REPLACE. On a real
 * ECU the seed comes from an on-chip TRNG, an HSM, or SHE, and it must be a
 * genuine entropy source: the DRBG's output is only as unpredictable as what
 * you feed it here, and a counter or a timestamp will make every key and IV on
 * the vehicle predictable.
 *
 * Contract: return 0 on success, non-zero on failure, and fill the whole
 * buffer.
 */

#include <wolfssl/wolfcrypt/settings.h>

#ifdef CUSTOM_RAND_GENERATE_SEED

#include <stdio.h>

/* Not guarded on __linux__: macOS and the BSDs have /dev/urandom too, and this
 * file exists to be replaced on the target anyway. */
#include <fcntl.h>
#include <unistd.h>

int ecu_seed(unsigned char* output, unsigned int sz)
{
    int fd;
    unsigned int got = 0;

    if (output == NULL) {
        return -1;
    }

    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ecu_seed: no entropy source available\n");
        return -1;
    }

    while (got < sz) {
        ssize_t n = read(fd, output + got, (size_t)(sz - got));

        if (n <= 0) {
            close(fd);
            fprintf(stderr, "ecu_seed: short read from entropy source\n");
            return -1;
        }
        got += (unsigned int)n;
    }

    close(fd);
    return 0;
}

#endif /* CUSTOM_RAND_GENERATE_SEED */
