/* clu_cert_setup.c
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
#include <unistd.h>
#include "clu_include/clu_header_main.h"
#include <wolfssl/wolfcrypt/types.h>
#include "clu_include/clu_error_codes.h"
#include "clu_include/x509/clu_cert.h"
#include "clu_include/x509/clu_parse.h"

enum {
    INPEM_OUTPEM  = 1,
    INPEM_OUTDER  = 2,
    INDER_OUTPEM  = 3,
    INDER_OUTDER  = 4,
    INPEM_OUTTEXT = 5,
    NOOUT_SET     = 6,
    OUTPUBTEXT    = 7,
};

int wolfCLU_certSetup(int argc, char** argv)
{
    int i, ret;
    int text_flag    = 0;   /* does user desire human readable cert info */
    int text_pubkey  = 0;   /* does user desire human readable pubkey info */
    int noout_flag   = 0;   /* are we outputting a file */
    int inder_flag   = 0;   /* is the incoming file in der format */
    int inpem_flag   = 0;   /* is the incoming file in pem format */
    int outder_flag  = 0;   /* is the output file in der format */
    int outpem_flag  = 0;   /* is the output file in pem format */
    int infile_flag  = 0;   /* set if passing in file argument */
    int outfile_flag = 0;   /* set if passing out file argument */
    int silent_flag  = 0;   /* set to disable echo to command line */

    char* infile  = NULL;   /* pointer to the infile name */
    char* outfile = NULL;   /* pointer to the outfile name */
    int   inform  = PEM_FORM; /* the input format */
    char* outform;          /* the output format */


/*---------------------------------------------------------------------------*/
/* help */
/*---------------------------------------------------------------------------*/
    ret = wolfCLU_checkForArg("-h", 2, argc, argv);
    if (ret > 0) {
        wolfCLU_certHelp();
        return 0;
    }
/*---------------------------------------------------------------------------*/
/* text */
/*---------------------------------------------------------------------------*/
    ret = wolfCLU_checkForArg("-text", 5, argc, argv);
    if (ret > 0) {
        /* set flag for converting to human readable.
         */
        text_flag = 1;
    } /* Optional flag do not return error */
/*---------------------------------------------------------------------------*/
/* pubkey */
/*---------------------------------------------------------------------------*/
    ret = wolfCLU_checkForArg("-pubkey", 7, argc, argv);
    if (ret > 0) {
        /* set flag for converting to human readable.
         */
        text_pubkey = 1;
    } /* Optional flag do not return error */
/*---------------------------------------------------------------------------*/
/* inform pem/der/??OTHER?? */
/*---------------------------------------------------------------------------*/
    ret = wolfCLU_checkForArg("-inform", 7, argc, argv);
    if (ret > 0) {
        inform = wolfCLU_checkInform(argv[ret+1]);
        if (inform == PEM_FORM) {
            inpem_flag = 1;
        } else if (inform == DER_FORM) {
            inder_flag = 1;
        } else {
            return inform;
        }
    }
    else if (ret == 0) {
        /* assume is PEM if not set */
        inpem_flag = 1;
    }
    else {
        return ret;
    }

/*---------------------------------------------------------------------------*/
/* outform pem/der/??OTHER?? */
/*---------------------------------------------------------------------------*/
    ret = wolfCLU_checkForArg("-outform", 8, argc, argv);
    if (ret > 0) {
        outform = argv[ret+1];
        ret = wolfCLU_checkOutform(outform);
        if (ret == PEM_FORM) {
            outpem_flag = 1;
        } else if (ret == DER_FORM) {
            outder_flag = 1;
        } else {
            return ret;
        }
    } else if (text_flag == 0 && text_pubkey == 0) {
        return ret;
    }

    

/*---------------------------------------------------------------------------*/
/* in file */
/*---------------------------------------------------------------------------*/
    ret = wolfCLU_checkForArg("-in", 3, argc, argv);
    if (ret > 0) {
       /* set flag for in file and flag for input file OK if exists
        * check for error case below. If no error then read in file */
       infile = argv[ret+1];
    } else {
        return ret;
    }

    if (access(infile, F_OK) != -1) {
        printf("input file is \"%s\"\n", infile);
        infile_flag = 1;
    } else {
        printf("ERROR: input file \"%s\" does not exist\n", infile);
        return INPUT_FILE_ERROR;
    }
/*---------------------------------------------------------------------------*/
/* out file */
/*---------------------------------------------------------------------------*/
    ret = wolfCLU_checkForArg("-out", 4, argc, argv);
    if (ret > 0) {
        /* set flag for out file, check for error case below. If no error
         * then write outfile */
        outfile_flag = 1;
        outfile = argv[ret+1];
    } else if (text_flag == 0 && text_pubkey == 0) {
        return ret;
    }

    if (outfile != NULL) {
        if (access(outfile, F_OK) != -1) {
            printf("output file set: \"%s\"\n", outfile);
        } else {
            printf("output file \"%s\"did not exist, it will be created.\n",
                                                                       outfile);
        }
    }
/*---------------------------------------------------------------------------*/
/* noout */
/*---------------------------------------------------------------------------*/
    ret = wolfCLU_checkForArg("-noout", 6, argc, argv);
    if (ret > 0) {
        /* set flag for no output file */
        noout_flag = 1;
    } /* Optional flag do not return error */
/*---------------------------------------------------------------------------*/
/* silent */
/*---------------------------------------------------------------------------*/
    ret = wolfCLU_checkForArg("-silent", 7, argc, argv);
    if (ret > 0) {
        /* set flag for converting to human readable.
         * return NOT_YET_IMPLEMENTED error
         */
        silent_flag = 1;
    } /* Optional flag do not return error */
/*---------------------------------------------------------------------------*/
/* END ARG PROCESSING */
/*---------------------------------------------------------------------------*/
    ret = 0;
    switch (error_check(inpem_flag, inder_flag, outpem_flag, outder_flag,
                      text_flag, text_pubkey, noout_flag)) {
        case INPEM_OUTPEM:
            if (infile_flag) wolfCLU_inpemOutpem(infile, outfile, silent_flag);
            else return INPUT_FILE_ERROR;
            break;
        case INPEM_OUTDER:
            if (infile_flag) wolfCLU_inpemOutder(infile, outfile, silent_flag);
            else return INPUT_FILE_ERROR;
            break;
        case INDER_OUTPEM:
            if (infile_flag) wolfCLU_inderOutpem(infile, outfile, silent_flag);
            else return INPUT_FILE_ERROR;
            break;
        case INDER_OUTDER:
            if (infile_flag) wolfCLU_inderOutder(infile, outfile, silent_flag);
            else return INPUT_FILE_ERROR;
            break;
        case INPEM_OUTTEXT:
            if (outfile_flag) {
                ret = wolfCLU_inpemOuttext(infile, outfile, silent_flag);
            } else {
                printf("Outfile not set, using stdout\n");
                outfile = "stdout";
                ret = wolfCLU_inpemOuttext(infile, outfile, silent_flag);
            }
            break;
        case OUTPUBTEXT:
            ret = wolfCLU_printX509PubKey(infile, inform, outfile, silent_flag);
            break;
        case NOOUT_SET:
            break;
        default:
            printf("Error case\n");
            ret = -1;
            break;
    }

    return ret;
}

/*
 * @arg inpem_flag: is inform set to pem
 * @arg inder_flag: is inform set to der
 * @arg outpem_flag: is outform set to pem
 * @arg outder_flag: is outform set to der
 */
int error_check(int inpem_flag, int inder_flag, 
                int outpem_flag, int outder_flag, 
                int text_flag, int text_pubkey, int noout_flag)
{
    int ret = USER_INPUT_ERROR;
    int tmp;
    ret = ( inpem_flag & inder_flag);
    if (ret) {
        printf("ERROR: inform set to both PEM and DER format\n");
        return USER_INPUT_ERROR;
    }
    ret = ( inpem_flag & outpem_flag);
    if (ret) {
        tmp = ret;
        ret = (tmp & noout_flag);
        if (ret) {
            printf("ERROR: noout set when output format is specified");
            return USER_INPUT_ERROR;
        }
        return INPEM_OUTPEM;
   }
    ret = (inpem_flag & outder_flag);
    if (ret) {
        tmp = ret;
        ret = (tmp & noout_flag);
        if (ret) {
            printf("ERROR: noout set when output format is specified");
            return USER_INPUT_ERROR;
        }
        return INPEM_OUTDER;
    }
    ret = (inder_flag & outpem_flag);
    if (ret) {
        tmp = ret;
        ret = (tmp & noout_flag);
        if (ret) {
            printf("ERROR: noout set when output format is specified");
            return USER_INPUT_ERROR;
        }
        return INDER_OUTPEM;
    }
    ret = (inder_flag & outder_flag);
    if (ret) {
        tmp = ret;
        ret = (tmp & noout_flag);
        if (ret) {
            printf("ERROR: noout set when output format is specified");
            return USER_INPUT_ERROR;
        }
        return INDER_OUTDER;
    }
    ret = (inpem_flag & text_flag);
    if (ret) {
        return INPEM_OUTTEXT; 
    }
    if (text_pubkey) {
        return OUTPUBTEXT;
    }
    ret = (outder_flag & outpem_flag);
    if (ret) {
        printf("ERROR: outform set to both DER and PEM format\n");
        return USER_INPUT_ERROR;
    }
    if (!ret) {
        ret = USER_INPUT_ERROR;
        if ( !inpem_flag && !inder_flag)
            printf("ERROR: Failed to specify input format: -inform not set\n");
        else {
            ret = (inder_flag & noout_flag) | (inpem_flag & noout_flag);
            if (ret) {
                return NOOUT_SET;
            }
            else
                printf("ERROR: Failed to specify -outform or -noout\n");
        }
    }
    return ret;
}
