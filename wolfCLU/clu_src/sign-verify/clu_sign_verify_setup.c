#include "clu_include/clu_header_main.h"
#include "clu_include/sign-verify/clu_sign.h"
#include "clu_include/sign-verify/clu_verify.h"

int wolfCLU_sign_verify_setup(int argc, char** argv)
{
    int     ret        =   0;   /* return variable, counter */
    int     i          =   0;   /* loop variable */
    char*   in;                 /* input variable */
    char*   out;                /* output variable */
    char*   priv;               /* private key variable */

    char*   alg;                /* algorithm being used */
    int     algCheck=   -1;      /* acceptable algorithm check */
    int     inCheck =   0;      /* input check */
    int     privCheck = 0;      /* private key check */
    int     signCheck = 0;
    int     verifyCheck = 0;
    int     pubInCheck = 0;
    
    /* help checking
    ret = wolfCLU_checkForArg("-help", 5, argc, argv);
    if (ret > 0) {
        wolfCLU_hashHelp();
        return 0;
    }
    */
    
    ret = wolfCLU_checkForArg("-rsa", 4, argc, argv);
    if (ret <= 0) {
        ret = wolfCLU_checkForArg("-ecc", 4, argc, argv);
        if (ret <= 0) {
            ret = wolfCLU_checkForArg("-ed25519", 8, argc, argv);
            if (ret <= 0) {
                printf("Please specify key type.\n");
                return FATAL_ERROR;
            }
            else {
                algCheck = ED25519_SIGN;
            }
        }
        else {
            algCheck = ECC_SIGN;
        }
    }
    else {
        algCheck = RSA_SIGN;
    }
    
    ret = wolfCLU_checkForArg("-sign", 5, argc, argv);
    if (ret > 0) {
        /* output file */
        signCheck = 1;
    }
    
    ret = wolfCLU_checkForArg("-verify", 7, argc, argv);
    if (ret > 0) {
        /* output file */
        verifyCheck = 1;
    }

    ret = wolfCLU_checkForArg("-inkey", 6, argc, argv);
    if (ret > 0) {
        priv = XMALLOC(strlen(argv[ret+1]), HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (priv == NULL)
            return MEMORY_E;

        XSTRNCPY(priv, &argv[ret+1][0], XSTRLEN(argv[ret+1]));
        priv[XSTRLEN(argv[ret+1])] = '\0';
        privCheck = 1;        
    }
    
    ret = wolfCLU_checkForArg("-pubin", 6, argc, argv);
    if (ret > 0) {
        /* output file */
        pubInCheck = 1;
    }
    
    ret = wolfCLU_checkForArg("-in", 3, argc, argv);
    if (ret > 0) {
        /* input file/text */
        in = XMALLOC(strlen(argv[i+1])+1, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);
        if (in == NULL)
            return MEMORY_E;

        XSTRNCPY(in, &argv[ret+1][0], XSTRLEN(argv[ret+1]));
        in[XSTRLEN(argv[ret+1])] = '\0';
        inCheck = 1;
    }
    
    ret = wolfCLU_checkForArg("-out", 4, argc, argv);
    if (ret > 0) {
        /* output file */
        out = argv[ret+1];
    } else {
        if(algCheck == RSA_SIGN) {
            printf("Please specifiy and output file when"
                   "signing or verifing with RSA.\n");
            return ret;
        }
    }

    if (inCheck == 0) {
        printf("Must have input as either a file or standard I/O\n");
        return FATAL_ERROR;
    }
    
    if (signCheck == 1) {
        ret = wolfCLU_sign_data(in, out, priv, algCheck);
    }
    else if (verifyCheck == 1) {
        ret = wolfCLU_verify_signature(in, out, priv, algCheck, pubInCheck);
        if(ret >= 0) {
            return 0;
        }
    }
    
    XFREE(in, HEAP_HINT, DYNAMIC_TYPE_TMP_BUFFER);

    return ret;
}
