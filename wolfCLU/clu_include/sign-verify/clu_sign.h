#include <wolfssl/options.h>
#ifdef HAVE_ED25519
    #include <wolfssl/wolfcrypt/ed25519.h>
#endif
#ifndef NO_RSA
    #include <wolfssl/wolfcrypt/rsa.h>
#endif
#ifdef HAVE_ECC
    #include <wolfssl/wolfcrypt/ecc.h>
    #include <wolfssl/wolfcrypt/asn_public.h>
#endif

enum {
    RSA_SIGN,
    ECC_SIGN,
    ED25519_SIGN,
};

int wolfCLU_sign_data(char*, char*, char*, int);


int wolfCLU_sign_data_rsa(byte*, char*, word32, char*);
int wolfCLU_sign_data_ecc(byte*, char*, word32, char*);
int wolfCLU_sign_data_ed25519(byte*, char*, word32, byte*, word32, char*);


