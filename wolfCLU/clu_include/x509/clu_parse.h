/* clu_parse.h
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

#ifndef WOLFCLU_PARSE_H
#define WOLFCLU_PARSE_H

/* a helper function for wolfCLU_parse_file */
int wolfCLU_inpemOutpem(char* infile, char* outfile, int silent_flag);
/* a helper function for wolfCLU_parse_file */
int wolfCLU_inpemOutder(char* infile, char* outfile, int silent_flag);
/* a helper function for wolfCLU_parse_file */
int wolfCLU_inderOutpem(char* infile, char* outfile, int silent_flag);
/* a helper function for wolfCLU_parse_file */
int wolfCLU_inderOutder(char* infile, char* outfile, int silent_flag);
/* a helper function for wolfCLU_parse_file */
int wolfCLU_inpemOuttext(char* infile, char* outfile, int silent_flag);

/**
 * @brief Function to print out DER public key
 *
 * @param bio the bio to print to
 * @param der der buffer to print out
 * @param derSz size of 'der' buffer
 *
 * @return returns 0 on success
 */
int wolfCLU_printDerPubKey(WOLFSSL_BIO* bio, unsigned char* der, int derSz);

/**
 * @brief prints out the public key from a certificate
 *
 * @param infile file to read from
 * @param inform PEM_FORM/DER_FORM of input
 * @param outfile name of the file to write to
 * @param silent_flag if should be silent instead of printout
 *
 * @return returns 0 on success
 */
int wolfCLU_printX509PubKey(char* infile, int inform, char* outfile,
        int silent_flag);
/* function for processing input/output based on format requests from user */
int wolfCLU_parseFile(char* infile, int inform, char* outfile, int outform,
                                                               int silent_flag);

#endif /* WOLFCLU_PARSE_H */
