/* gnu c and posix includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

/* wolfssl includes */
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/dilithium.h>
#include <wolfssl/wolfcrypt/random.h>

struct ml_dsa_len_t {
    uint8_t      sec_cat;
    const char * name;
    uint16_t     priv_len;
    uint16_t     pub_len;
    uint16_t     sig_len;
};

/* ML-DSA supported parameter sets, and key and signature sizes.
 * From tables 1 and 2 of:
 *   - https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.204.pdf
 * */
static struct ml_dsa_len_t ml_dsa_len[] =
    { {2, "ML-DSA-44", 2560, 1312, 2420},
      {3, "ML-DSA-65", 4032, 1952, 3309},
      {5, "ML-DSA-87", 4896, 2592, 4627} };

static int  ml_dsa_sec_valid(void);
static int  ml_dsa_valid_len(word32 priv_len, word32 pub_len, word32 sig_len);
static void ml_dsa_dump_hex(const uint8_t * data, size_t len,
                            const char * what);
static int  ml_dsa_dump_file(const uint8_t * data, size_t len,
                             const char * what);
static void ml_dsa_free(void ** p);
static void ml_dsa_print_usage_and_die(void) __attribute__((noreturn));
static void ml_dsa_print_parms_and_die(void) __attribute__((noreturn));

static const char * prog_name = "ml_dsa_test";
static const char * default_msg = "wolfssl ml-dsa test!";
static int          print_parms = 0;
static int          sec_cat = 2 /* ML-DSA security category */;
static int          verbose = 0;
static int          write_to_file = 0;

int
main(int    argc,
     char * argv[])
{
    int          rc = 0;
    int          opt = 0;
    MlDsaKey     key;
    WC_RNG       rng;
    byte *       priv = NULL;
    byte *       pub = NULL;
    byte *       sig = NULL;
    word32       priv_len = 0;
    word32       pub_len = 0;
    word32       sig_len = 0;
    int          ml_dsa_priv_len = 0;
    int          ml_dsa_pub_len = 0;
    int          ml_dsa_sig_len = 0;
    const char * msg = default_msg;
    int          verify_res = 0;

    if (argc <= 1) { ml_dsa_print_usage_and_die(); }

    while ((opt = getopt(argc, argv, "c:m:pvw?")) != -1) {
        switch (opt) {
        case 'c':
            sec_cat = atoi(optarg);
            break;
        case 'm':
            msg = optarg;
            break;
        case 'p':
            print_parms = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'w':
            write_to_file = 1;
            break;
        case '?':
        default:
            ml_dsa_print_usage_and_die();
            break;
        }
    }

    if (print_parms) {
        ml_dsa_print_parms_and_die();
    }

    wc_InitRng(&rng);

    rc = ml_dsa_sec_valid();
    if (rc < 0) {
        goto ml_dsa_exit;
    }

    rc = wc_MlDsaKey_Init(&key, NULL, INVALID_DEVID);
    if (rc != 0) {
        printf("error: wc_MlDsaKey_Init returned %d\n", rc);
        goto ml_dsa_exit;
    }

    rc = wc_MlDsaKey_SetParams(&key, sec_cat);
    if (rc != 0) {
        printf("error: wc_MlDsaKey_SetParams(%d) returned %d\n",
               sec_cat, rc);
        goto ml_dsa_exit;
    }

    printf("info: making key\n");

    /* Make the key pair. */
    rc = wc_MlDsaKey_MakeKey(&key, &rng);
    if (rc != 0) {
        printf("error: wc_MlDsaKey_MakeKey returned %d\n", rc);
        goto ml_dsa_exit;
    }

    /* Get the ML-DSA signature length. */
    rc = wc_MlDsaKey_GetSigLen(&key, &ml_dsa_sig_len);
    if (rc != 0 || ml_dsa_sig_len <= 0) {
        printf("error: wc_MlDsaKey_GetSigLen returned %d\n",
                rc);
        goto ml_dsa_exit;
    }

    sig_len = (word32) ml_dsa_sig_len;

    /* Get the ML-DSA public key length. */
    rc = wc_MlDsaKey_GetPubLen(&key, &ml_dsa_pub_len);
    if (rc != 0 || ml_dsa_pub_len <= 0) {
        printf("error: wc_MlDsaKey_GetPrivLen returned %d\n",
                rc);
        goto ml_dsa_exit;
    }

    /* Get the ML-DSA private key length. This API returns
     * the public + private length. */
    rc = wc_MlDsaKey_GetPrivLen(&key, &ml_dsa_priv_len);
    if (rc != 0 || ml_dsa_priv_len <= 0) {
        printf("error: wc_MlDsaKey_GetPrivLen returned %d\n",
                rc);
        goto ml_dsa_exit;
    }

    if (ml_dsa_priv_len <= ml_dsa_pub_len) {
        printf("error: ml-dsa: unexpected key lengths: %d, %d",
               ml_dsa_priv_len, ml_dsa_pub_len);
        goto ml_dsa_exit;
    }
    else {
        ml_dsa_priv_len -= ml_dsa_pub_len;
    }

    priv = malloc(ml_dsa_priv_len);
    if (priv == NULL) {
        printf("error: malloc(%d) failed\n", ml_dsa_priv_len);
        goto ml_dsa_exit;
    }

    pub = malloc(ml_dsa_pub_len);
    if (pub == NULL) {
        printf("error: malloc(%d) failed\n", ml_dsa_pub_len);
        goto ml_dsa_exit;
    }

    sig = malloc(ml_dsa_sig_len);
    if (sig == NULL) {
        printf("error: malloc(%d) failed\n", ml_dsa_sig_len);
        goto ml_dsa_exit;
    }

    /* Set the expected key lengths. */
    pub_len = (word32) ml_dsa_pub_len;
    priv_len = (word32) ml_dsa_priv_len;

    rc = wc_MlDsaKey_ExportPubRaw(&key, pub, &pub_len);
    if (rc != 0) {
        printf("error: wc_MlDsaKey_ExportPubRaw returned %d\n", rc);
        goto ml_dsa_exit;
    }

    ml_dsa_dump_hex(pub, pub_len, "pub key");
    ml_dsa_dump_file(pub, pub_len, "pub.key");

    rc = wc_MlDsaKey_ExportPrivRaw(&key, priv, &priv_len);
    if (rc != 0) {
        printf("error: wc_MlDsaKey_ExportPrivRaw returned %d\n", rc);
        goto ml_dsa_exit;
    }

    if ((int) priv_len != ml_dsa_priv_len) {
        printf("error: ml_dsa priv key mismatch: got %d " \
               "bytes, expected %d\n", priv_len, ml_dsa_priv_len);
        goto ml_dsa_exit;
    }

    ml_dsa_dump_hex(priv, priv_len, "priv key");
    ml_dsa_dump_file(priv, priv_len, "priv.key");

    rc = ml_dsa_valid_len(priv_len, pub_len, sig_len);
    if (rc != 0) {
        printf("error: ml_dsa_valid_len returned %d\n", rc);
        goto ml_dsa_exit;
    }

    rc = wc_MlDsaKey_Sign(&key, sig, &sig_len, (const byte *) msg, strlen(msg),
                          &rng);

    if (rc != 0) {
        printf("error: wc_MlDsaKey_Sign returned %d\n", rc);
        goto ml_dsa_exit;
    }

    printf("info: signed message\n");

    ml_dsa_dump_hex((const uint8_t *) msg, strlen(msg), "msg");
    ml_dsa_dump_hex(sig, sig_len, "signature");
    ml_dsa_dump_file((const uint8_t *) msg, strlen(msg), "msg.bin");
    ml_dsa_dump_file(sig, sig_len, "signature.bin");

    rc = wc_MlDsaKey_Verify(&key, sig, sig_len,
                            (const byte *) msg, strlen(msg),
                            &verify_res);

    if (rc == 0 && verify_res == 1) {
        printf("info: verify message good\n");
    }
    else {
        printf("error: wc_MlDsaKey_Verify returned: ret=%d, "
                        "res=%d\n", rc, verify_res);
        rc = -1;
        goto ml_dsa_exit;
    }

    printf("info: done\n");

    ml_dsa_exit:

    wc_MlDsaKey_Free(&key);
    wc_FreeRng(&rng);

    ml_dsa_free((void *) &sig);
    ml_dsa_free((void *) &pub);
    ml_dsa_free((void *) &priv);

    return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int
ml_dsa_sec_valid(void)
{
    int rc = 0;

    switch (sec_cat) {
    case 2:
    case 3:
    case 5:
        rc = 0;
        break;
    default:
        printf("error: invalid security category: %d\n", sec_cat);
        rc = -1;
        break;
    }

    return rc;
}

static int
ml_dsa_valid_len(word32 priv_len,
                 word32 pub_len,
                 word32 sig_len)
{
    size_t i = 0;

    if (ml_dsa_sec_valid() != 0) {
        return -1;
    }

    switch(sec_cat) {
    case 2:
        i = 0; break;
    case 3:
        i = 1; break;
    case 5:
        i = 2; break;
    default:
        return -1;
    }

    if (ml_dsa_len[i].priv_len != priv_len) {
        printf("error: priv_len: got %d, expected %d\n",
               ml_dsa_len[i].priv_len,
               priv_len);
        return -1;
    }

    if (ml_dsa_len[i].pub_len != pub_len) {
        printf("error: pub_len: got %d, expected %d\n",
               ml_dsa_len[i].pub_len,
               pub_len);
        return -1;
    }

    if (ml_dsa_len[i].sig_len != sig_len) {
        printf("error: sig_len: got %d, expected %d\n",
               ml_dsa_len[i].sig_len,
               sig_len);
        return -1;
    }

    //printf("info: ML-DSA lens good\n");
    printf("info: using %s, Security Category %d: "
           "priv_len %d, pub_len %d, sig_len %d\n",
           ml_dsa_len[i].name, ml_dsa_len[i].sec_cat,
           ml_dsa_len[i].priv_len,
           ml_dsa_len[i].pub_len,
           ml_dsa_len[i].sig_len);

    return 0;
}

static void
ml_dsa_dump_hex(const uint8_t * data,
                size_t          len,
                const char *    what)
{
    if (!verbose) { return; }

    printf("%s (%zu):\n", what, len);

    for (size_t i = 0; i < len; ++i) {
        printf("0x%02x ", data[i]);

        if (((i + 1) % 8 == 0)) {
            printf("\n");
        }
    }

    printf("\n");

    return;
}

static int
ml_dsa_dump_file(const uint8_t * data,
                 size_t          len,
                 const char *    what)
{
    FILE * file = NULL;
    int    n_write = 0;
    int    err = 0;

    if (!write_to_file) { return 0; }

    file = fopen(what, "w+");

    if (file == NULL) {
        printf("error: fopen(%s, \"w+\") failed\n", what);
        return -1;
    }

    n_write = fwrite(data, 1, len, file);

    if (n_write != (int) len) {
        printf("error: fwrite(%p, 1, %zu, %p) returned %d, expected %zu\n",
               data, len, file, n_write, len);
        fclose(file);
        return -1;
    }

    err = fclose(file);
    file = NULL;

    if (err) {
        printf("error: fclose returned: %d\n", err);
        return -1;
    }

    return 0;
}

static void
ml_dsa_free(void ** p)
{
  if (p && *p) {
    free(*p);
    *p = NULL;
  }
}

static void
ml_dsa_print_usage_and_die(void)
{
    printf("usage:\n");
    printf("  ./%s [-pvw] -c <security category> [-m <message>]\n", prog_name);
    printf("\n");
    printf("parms:\n");
    printf("  -m <message>               the message to sign\n");
    printf("  -c <security category>     set the security category {2,3,5}\n");
    printf("  -p                         print FIPS 204 parameters and exit\n");
    printf("  -v                         verbose\n");
    printf("  -w                         write keys, msg, and sig to file\n");
    printf("  -?                         show this help\n");
    exit(EXIT_FAILURE);
}

static void
ml_dsa_print_parms_and_die(void)
{
    printf("ML-DSA parameters and key/sig sizes*\n");
    printf("\n");
    printf("                Private Key   Public Key   Signature Size   Security Strength\n");
    printf("    ML-DSA-44      2560          1312         2420            Category 2\n");
    printf("    ML-DSA-65      4032          1952         3309            Category 3\n");
    printf("    ML-DSA-87      4896          2592         4627            Category 5\n");
    printf("\n");
    printf("\n");
    printf("* from Tables 1 & 2 of FIPS 204:\n");
    printf("    https://csrc.nist.gov/pubs/fips/204/final\n");

    exit(EXIT_FAILURE);
}
