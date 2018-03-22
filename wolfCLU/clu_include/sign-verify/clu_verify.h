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

int wolfCLU_verify_signature(char* , char*, char*, int, int);

int wolfCLU_verify_signature_rsa(byte* , char*, int, char*, int);
int wolfCLU_verify_signature_ecc(byte*, int, byte*, int, char*);
int wolfCLU_verify_signature_ed25519(byte*, word32, char*);
