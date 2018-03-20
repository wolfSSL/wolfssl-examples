int wolfCLU_verify_signature(char* , char*, char*, int, int);

int wolfCLU_verify_signature_rsa(byte* , char*, int, char*, int);
int wolfCLU_verify_signature_ecc(byte*, int, char*);
int wolfCLU_verify_signature_ed25519(byte*, word32, char*);
