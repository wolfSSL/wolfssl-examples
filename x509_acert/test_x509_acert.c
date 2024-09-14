/* glibc includes */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(USE_WOLFSSL)
  /* wolfssl includes */
  #include <wolfssl/options.h>
  #include <wolfssl/openssl/bio.h>
  #include <wolfssl/openssl/evp.h>
  #include <wolfssl/openssl/pem.h>
  #include <wolfssl/openssl/ssl.h>
  #include <wolfssl/ssl.h>
#else
  /* openssl includes */
  #include <openssl/err.h>
  #include <openssl/pem.h>
  #include <openssl/rsa.h>
  #include <openssl/x509_acert.h>
#endif

static int          acert_check_opts(const char * file, const char * cert,
                                     const char * pkey_file);
static void         acert_err(const char * what, int rc);
static int          acert_print_usage_and_die(void) __attribute__((noreturn));
static int          acert_do_test(const char * file, const char * cert,
                                  const char * pkey_file);
static X509_ACERT * acert_read(const char * file);
#if !defined(USE_WOLFSSL)
static int          acert_write(const char * file, const X509_ACERT * acert);
static int          acert_write_pubkey(const char * file, EVP_PKEY * pkey);
#endif /* if !USE_WOLFSSL */
static EVP_PKEY *   acert_read_pubkey(const char * file);
static int          acert_print(X509_ACERT * x509);
static EVP_PKEY *   acert_read_x509_pubkey(const char * cert);
static int          acert_test_api_misc(X509_ACERT * x509);
#if defined(USE_WOLFSSL)
static int          acert_parse_attr(const X509_ACERT * x509);
static void         acert_dump_hex(const char * what, const byte * data,
                                   size_t len);
#endif /* if USE_WOLFSSL */

static int          dump = 0;
static int          parse = 0;
static const char * mdname = "SHA2-256";
static const char * mgf1_mdname = "SHA1";
static int          salt_len = 0;
static int          rsa_pss = 0;
static int          print = 0;
static int          sign = 0;
static int          write_acert = 0;
static int          verbose = 0;

int
main(int    argc,
     char * argv[])
{
  const char * cert = NULL;
  const char * file = NULL;
  const char * pkey_file = NULL;
  int          opt = 0;
  int          rc = 0;

  while ((opt = getopt(argc, argv, "c:f:g:k:l:m:dpqrsvw?")) != -1) {
    switch (opt) {
    case 'c':
      cert = optarg;
      break;

    case 'd':
      dump = 1;
      break;

    case 'f':
      file = optarg;
      break;

    case 'k':
      pkey_file = optarg;
      break;

    case 'l':
      salt_len = atoi(optarg);

      if (salt_len <= 0) {
        printf("error: invalid -s salt_len: %d\n", salt_len);
        return EXIT_FAILURE;
      }

      break;

    case 'm':
      mgf1_mdname = optarg;
      break;

    case 'p':
      print = 1;
      break;

    case 'q':
      parse = 1;
      break;

    case 'r':
      rsa_pss = 1;
      break;

    case 's':
      sign = 1;
      break;

    case 'v':
      verbose = 1;
      break;

    case 'w':
      write_acert = 1;
      break;

    case '?':
    default:
      acert_print_usage_and_die();
      break;
    }
  }

  #if defined(USE_WOLFSSL)
  wolfSSL_Init();
  if (verbose) {
    wolfSSL_Debugging_ON();
  }
  #endif /* if USE_WOLFSSL */

  rc = acert_check_opts(file, cert, pkey_file);

  if (rc < 0) {
    return EXIT_FAILURE;
  }

  if (file == NULL) {
    printf("error: file: NULL\n");
    return EXIT_FAILURE;
  }

  rc = acert_do_test(file, cert, pkey_file);

  if (rc == 0) {
    printf("info: acert_do_test: good\n");
    printf("success\n");
  }
  else {
    printf("error: acert_do_test returned: %d\n", rc);
    printf("fail\n");
  }

  return (rc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int
acert_check_opts(const char * file,
                 const char * cert,
                 const char * pkey_file)
{
  if (file == NULL) {
    printf("error: acert file required\n");
    return -1;
  }

  printf("info: using acert file: %s\n", file);

  if (cert != NULL) {
    printf("info: using cert file: %s\n", cert);
  }

  if (pkey_file != NULL) {
    printf("info: using pubkey file: %s\n", pkey_file);
  }

  if (pkey_file != NULL && cert != NULL) {
    printf("error: -c and -k are mutually exclusive\n");
    return -1;
  }

  if ((pkey_file != NULL || cert != NULL) && sign != 0) {
    printf("error: -c[k] and -s are mutually exclusive\n");
    return -1;
  }

  if (rsa_pss) {
    printf("info: using rsa_pss\n");
    printf("info: using mdname: %s\n", mdname);
    printf("info: using mask alg: mgf1 with %s\n", mgf1_mdname);
  }

  return 0;
}

static int
acert_do_test(const char * file,
              const char * cert,
              const char * pkey_file)
{
  EVP_PKEY *      pkey = NULL;
  X509_ACERT *    x509 = NULL;
  uint8_t         fail = 0;
  int             rc = 0;
#if !defined(USE_WOLFSSL)
  EVP_PKEY_CTX *  pctx = NULL;
#endif /* ! USE_WOLFSSL */

  x509 = acert_read(file);

  if (x509 == NULL) {
    printf("error: acert_read returned: NULL\n");
    fail = 1;
    goto end_acert_do_test;
  }

  rc = acert_print(x509);

  if (rc) {
    printf("error: acert_print returned: %d\n", rc);
    fail = 1;
    goto end_acert_do_test;
  }

  rc = acert_test_api_misc(x509);

  if (rc) {
    printf("error: acert_test_api_misc returned: %d\n", rc);
    fail = 1;
    goto end_acert_do_test;
  }

  #if defined(USE_WOLFSSL)
  rc = acert_parse_attr(x509);

  if (rc) {
    printf("error: acert_parse_attr returned: %d\n", rc);
    fail = 1;
    goto end_acert_do_test;
  }
  #endif /* if USE_WOLFSSL */

  if (cert) {
    pkey = acert_read_x509_pubkey(cert);

    if (pkey == NULL) {
      printf("error: acert_read_print_pubkey returned: NULL\n");
      fail = 1;
      goto end_acert_do_test;
    }
  }
  else if (pkey_file) {
    pkey = acert_read_pubkey(pkey_file);

    if (pkey == NULL) {
      printf("error: acert_read_pubkey returned: NULL\n");
      fail = 1;
      goto end_acert_do_test;
    }
  }

  #if !defined(USE_WOLFSSL)
  /* todo: wolfssl sign acert support */
  if (sign) {
    /* Generate a new pkey, and sign the x509 acert.
     * The pkey should not be loaded yet. */
    if (pkey != NULL) {
      printf("error: pkey already exists: %p\n", pkey);
      fail = 1;
      goto end_acert_do_test;
    }

    if (!rsa_pss) {
      /* Use normal boring RSA keygen. */
      pkey = EVP_RSA_gen(2048);

      if (pkey != NULL) {
        printf("info: EVP_RSA_gen(2048): good\n");
      }
      else {
        printf("error: EVP_RSA_gen returned: NULL\n");
        fail = 1;
        goto end_acert_do_test;
      }
    }
    else {
      int pss_rc = 0;

      pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA_PSS, NULL);

      if (pctx == NULL) {
        printf("error: EVP_PKEY_CTX_new_id returned: NULL\n");
        fail = 1;
        goto end_acert_do_test;
      }

      pss_rc = EVP_PKEY_keygen_init(pctx);

      if (pss_rc <= 0) {
        printf("error: EVP_PKEY_keygen_init returned: %d\n", pss_rc);
        fail = 1;
        goto end_acert_do_test;
      }

      pss_rc = EVP_PKEY_CTX_set_rsa_pss_keygen_md_name(pctx, mdname, NULL);
      if (pss_rc <= 0) {
        acert_err("EVP_PKEY_CTX_set_rsa_pss_keygen_md_name", pss_rc);
        fail = 1;
        goto end_acert_do_test;
      }

      pss_rc = EVP_PKEY_CTX_set_rsa_pss_keygen_mgf1_md_name(pctx, mgf1_mdname);
      if (pss_rc <= 0) {
        acert_err("EVP_PKEY_CTX_set_rsa_pss_keygen_mgf1_md_name", pss_rc);
        fail = 1;
        goto end_acert_do_test;
      }

      if (salt_len) {
        pss_rc = EVP_PKEY_CTX_set_rsa_pss_keygen_saltlen(pctx, salt_len);
        if (pss_rc <= 0) {
          acert_err("EVP_PKEY_CTX_set_rsa_pss_keygen_saltlen", pss_rc);
          fail = 1;
          goto end_acert_do_test;
        }
      }

      pss_rc = EVP_PKEY_keygen(pctx, &pkey);

      if (pss_rc <= 0) {
        printf("error: EVP_PKEY_keygen returned: %d\n", pss_rc);
        fail = 1;
        goto end_acert_do_test;
      }

      if (pkey == NULL) {
        printf("error: EVP_PKEY_keygen returned: NULL\n");
        fail = 1;
        goto end_acert_do_test;
      }
    }
  }

  if (sign && pkey) {
    int sign_rc = X509_ACERT_sign(x509, pkey, EVP_sha256());

    if (sign_rc > 0) {
      printf("info: X509_ACERT_sign: good\n");
    }
    else {
      acert_err("X509_ACERT_sign", sign_rc);
      fail = 1;
      goto end_acert_do_test;
    }
  }

  if (write_acert) {
    /* Save the signed acert to file. */
    int write_rc = acert_write("acert_new.pem", x509);
    if (write_rc) {
      fail = 1;
      goto end_acert_do_test;
    }
  }

  if (write_acert && pkey) {
    /* Save the new pubkey to file. */
    int write_rc = acert_write_pubkey("pkey_new.pem", pkey);
    if (write_rc) {
      fail = 1;
      goto end_acert_do_test;
    }
  }
  #endif /* if !USE_WOLFSSL */

  if (pkey) {
    int verify_rc = X509_ACERT_verify(x509, pkey);

    if (verify_rc == 1) {
      printf("info: X509_ACERT_verify: good\n");
    }
    else {
      acert_err("X509_ACERT_verify", verify_rc);
      fail = 1;
      goto end_acert_do_test;
    }
  }

  end_acert_do_test:

  if (x509 != NULL) {
    X509_ACERT_free(x509);
    x509 = NULL;
  }

#if !defined(USE_WOLFSSL)
  if (pctx) {
    EVP_PKEY_CTX_free(pctx);
    pctx = NULL;
  }
#endif /* ! USE_WOLFSSL */

  if (pkey) {
    EVP_PKEY_free(pkey);
    pkey = NULL;
  }

  return fail ? -1 : 0;
}

static int
acert_test_api_misc(X509_ACERT * x509)
{
  int rc = 0;
  int ver = 0;

  if (x509 == NULL) {
    return -1;
  }

  ver = X509_ACERT_get_version(x509);

  if (ver <= 0) {
    printf("error: X509_ACERT_get_version returned: %d\n", ver);
    return -1;
  }

  printf("info: acert version: %x\n", (uint8_t) ver);

  return rc;
}

#if defined(USE_WOLFSSL)
/* Given an x509 acert, retrieve the raw attributes buffer and
 * length, and then parses it a little.
 *
 * Returns   0  on success.
 * Returns < 0  on error.
 * */
static int
acert_parse_attr(const X509_ACERT * x509)
{
  const byte * attr = NULL;
  word32       attr_len = 0;
  word32       idx = 0;
  word32       max_idx = 0;
  byte         tag;
  int          rc = 0;
  int          len = 0;

  rc = wolfSSL_X509_ACERT_get_attr_buf(x509, &attr, &attr_len);

  if (rc != SSL_SUCCESS) {
    printf("error: wolfSSL_X509_ACERT_get_attr_buf returned: %d\n", rc);
    return -1;
  }

  if (attr == NULL || attr_len <= 0) {
    printf("error: attr = %p, attr_len = %d\n", attr, attr_len);
    return -1;
  }

  if (!parse) {
    /* Just verify we could get the buffer and length and return early. */
    return 0;
  }

  /* Try to parse the attributes. This is WIP. */
  acert_dump_hex("Attributes", attr, attr_len);

  max_idx = attr_len;

  rc = GetASNTag(attr + idx, &idx, &tag, max_idx);

  if (rc < 0) {
    printf("error: GetASNTag(%p, %d, %d, %d) returned: %d\n", attr + idx,
           idx, tag, max_idx, tag);
    return -1;
  }

  printf("info: GetASNTag(%p, %d, %d, %d): found tag: 0x%0x\n", attr + idx,
         idx, tag, max_idx, tag);

  len = GetLength(attr + idx, &idx, &len, max_idx);

  if (len <= 0) {
    printf("error: GetLength(%p, %d, %d, %d) returned: %d\n", attr + idx,
           idx, len, max_idx, len);
    return -1;
  }

  printf("info: GetLength(%p, %d, %d, %d) returned: %d\n", attr + idx,
         idx, len, max_idx, len);

  return rc;
}

#define BOLDRED    "\033[1m\033[31m"
#define BOLDGREEN  "\033[1m\033[32m"
#define BOLDWHITE  "\033[1m\033[37m"
#define BOLDBLUE   "\033[1m\033[34m"
#define BOLDYELLOW "\033[1m\033[33m"
#define RESET      "\033[0m"

static void
acert_print_data(const byte * data,
                 size_t       i,
                 size_t       j)
{
  if (isprint(data[i + 2 + j])) {
    printf("%c", data[i + 2 + j]);
  }
  else {
    printf(".");
  }

  return;
}

/* Dump data as hex, with some pretty color coding.
 * Kind of a silly work in progress, for debugging use.
 * */
static void
acert_dump_hex(const char * what,
               const byte * data,
               size_t       len)
{
  uint8_t  seq_list[1024];
  uint16_t n_seq = 0;
  uint8_t  str_list[1024];
  uint16_t n_str = 0;

  memset(str_list, 0, sizeof(str_list));

  if (!dump) {
    return;
  }

  printf("\ninfo: %s\n", what);

  for (size_t i = 0; i < len; ++i) {
    if (i % 8 == 0) {
      /* indent first element */
      printf("  ");
    }

    if (data[i] == (ASN_SEQUENCE | ASN_CONSTRUCTED)) {
      seq_list[n_seq] = i;
      n_seq++;

      printf(BOLDRED "0x%02x " RESET, data[i]);

      if ((i + 1) % 8 == 0) {
        printf("\n");
      }

      ++i;

      printf(BOLDGREEN "0x%02x " RESET, data[i]);
    }
    else if (data[i] == ASN_PRINTABLE_STRING) {
      str_list[n_str] = i;
      n_str++;

      printf(BOLDBLUE "0x%02x " RESET, data[i]);

      if ((i + 1) % 8 == 0) {
        printf("\n");
      }

      ++i;

      printf(BOLDYELLOW "0x%02x " RESET, data[i]);
    }
    else {
      printf("0x%02x ", data[i]);
    }

    if ((i + 1) % 8 == 0) {
      printf("\n");
    }
  }

  printf("\n\n");

  if (n_seq) {
    printf("constructed sequences\n");

    for (size_t n = 0; n < n_seq; ++n) {
      size_t  i = seq_list[n];
      uint8_t seq_len = data[i + 1];

      printf(BOLDRED "  0x%02x " RESET, data[i]);
      printf(BOLDGREEN "0x%02x " RESET, data[i + 1]);

      for (size_t j = 0; j < seq_len; ++j) {
        acert_print_data(data, i, j);
      }
      printf("\n");
    }
  }

  printf("\n");

  if (n_str) {
    printf("printable strings\n");

    for (size_t n = 0; n < n_str; ++n) {
      size_t  i = str_list[n];
      uint8_t str_len = data[i + 1];

      printf(BOLDBLUE "  0x%02x " RESET, data[i]);
      printf(BOLDYELLOW "0x%02x " RESET, data[i + 1]);

      for (size_t j = 0; j < str_len; ++j) {
        printf("%c", data[i + 2 + j]);
        acert_print_data(data, i, j);
      }

      printf("\n");
    }
  }

  printf("\n");

  return;
}
#endif /* if USE_WOLFSSL */

/* Reads and print pubkey certificate.
 * */
static EVP_PKEY *
acert_read_x509_pubkey(const char * cert)
{
  BIO *      bp = NULL;
  BIO *      bout = NULL;
  X509 *     x509 = NULL;
  EVP_PKEY * pkey = NULL;
  int        rc = -1;

  bp = BIO_new_file(cert, "r");

  if (bp == NULL) {
    printf("error: BIO_new_file returned: NULL\n");
    goto end_cert_read;
  }

  bout = BIO_new_fp(stderr, BIO_NOCLOSE);

  if (bout == NULL) {
    printf("error: BIO_new_fp returned: NULL\n");
    goto end_cert_read;
  }

  x509 = PEM_read_bio_X509(bp, NULL, NULL, NULL);

  if (x509 == NULL) {
    printf("error: PEM_read_bio_X509 returned: NULL\n");
    goto end_cert_read;
  }

  printf("info: PEM_read_bio_X509: good\n");

  if (print) {
    rc = X509_print(bout, x509);

    if (rc != 1) {
      printf("error: X509_print returned: %d\n", rc);
    }
  }

  pkey = X509_get_pubkey(x509);

  if (pkey == NULL) {
    printf("error: X509_get_pubkey(%p) returned: NULL\n", x509);
    goto end_cert_read;
  }

  end_cert_read:

  if (bp != NULL) {
    BIO_free(bp);
    bp = NULL;
  }

  if (bout != NULL) {
    BIO_free(bout);
    bout = NULL;
  }

  if (x509 != NULL) {
    X509_free(x509);
    x509 = NULL;
  }

  return pkey;
}

/* Reads an x509 acert from file.
 *
 * Returns: X509_ACERT *   on success.
 * Returns: NULL           on failure.
 * */
static X509_ACERT *
acert_read(const char * file)
{
  BIO *        bp = NULL;
  X509_ACERT * x509 = NULL;

  bp = BIO_new_file(file, "r");

  if (bp == NULL) {
    printf("error: BIO_new_file returned: NULL\n");
    return NULL;
  }

  x509 = PEM_read_bio_X509_ACERT(bp, NULL, NULL, NULL);
  BIO_free(bp);
  bp = NULL;

  if (x509 == NULL) {
    printf("error: PEM_read_bio_X509_ACERT returned: NULL\n");
    return NULL;
  }

  printf("info: PEM_read_bio_X509_ACERT: good\n");

  return x509;
}

#if !defined(USE_WOLFSSL)
/* Writes an x509 acert to file.
 *
 * Not supported in wolfSSL yet.
 * */
static int
acert_write(const char * file,
            const X509_ACERT * acert)
{
  BIO * bp = NULL;
  int   ret = 0;

  bp = BIO_new_file(file, "w");

  if (bp == NULL) {
    printf("error: BIO_new_file returned: NULL\n");
    return -1;
  }

  ret = PEM_write_bio_X509_ACERT(bp, acert);
  BIO_free(bp);
  bp = NULL;

  if (ret != 1) {
    printf("error: PEM_write_bio_X509_ACERT: %d\n", ret);
    return -1;
  }

  printf("info: wrote acert to file: %s\n", file);

  return 0;
}

static int
acert_write_pubkey(const char * file,
                   EVP_PKEY *   pkey)
{
  BIO * bp = NULL;
  int   ret = 0;

  bp = BIO_new_file(file, "w");

  if (bp == NULL) {
    printf("error: BIO_new_file returned: NULL\n");
    return -1;
  }

  ret = PEM_write_bio_PUBKEY(bp, pkey);
  BIO_free(bp);
  bp = NULL;

  if (ret != 1) {
    printf("error: PEM_write_bio_PUBKEY: %d\n", ret);
    return -1;
  }

  printf("info: wrote pubkey to file: %s\n", file);

  return 0;
}
#endif /* if !USE_WOLFSSL */

static EVP_PKEY *
acert_read_pubkey(const char * file)
{
  BIO *      bp = NULL;
  EVP_PKEY * pkey = NULL;

  bp = BIO_new_file(file, "r");

  if (bp == NULL) {
    printf("error: BIO_new_file returned: NULL\n");
    return NULL;
  }

  pkey = PEM_read_bio_PUBKEY(bp, &pkey, NULL, NULL);
  BIO_free(bp);
  bp = NULL;

  if (pkey == NULL) {
    printf("error: PEM_read_bio_PUBKEY returned: NULL\n");
    return NULL;
  }

  printf("info: PEM_read_bio_PUBKEY: good\n");
  return pkey;
}

static int
acert_print(X509_ACERT * x509)
{
  BIO * bout = NULL;
  int   print_rc = 0;

  if (!print) {
    /* Nothing to do. */
    return 0;
  }

  bout = BIO_new_fp(stderr, BIO_NOCLOSE);

  if (bout == NULL) {
    printf("error: BIO_new_fp returned: NULL\n");
    return -1;
  }

  print_rc = X509_ACERT_print(bout, x509);
  BIO_free(bout);
  bout = NULL;

  if (print_rc == 1) {
    printf("info: X509_ACERT_print: good\n");
  }
  else {
    printf("error: X509_ACERT_print returned: %d\n", print_rc);
    return -1;
  }

  return 0;
}

static void
acert_err(const char * what,
          int          rc)
{
    unsigned long err = ERR_get_error();
    printf("error: %s returned: %d: %lu, %s\n",
            what, rc, err, ERR_error_string(err, NULL));

    return;
}

static int
acert_print_usage_and_die(void)
{
  printf("options:\n");
  printf(" -c <path to pub key cert>\n");
  printf(" -f <path to acert file\n");
  printf(" -k <path to pub key file>\n");
  printf(" -m <mask function>                  (rsa_pss only)\n");
  printf(" -l <salt len>                       (rsa_pss only)\n");
  printf(" -r                                  (use rsa_pss)\n");
  printf(" -s                                  (resign and verify)\n");
  printf(" -v                                  (verbose)\n");
  printf(" -w                                  (write signed acert to file)\n");
  printf("\n");
  printf("usage:\n");
  printf("  verifying acert with pub key file:\n");
  printf("  $./test/test_acert -f certs/signed/acert.pem -k certs/signed/acert_pubkey.pem\n");
  printf("  info: using acert file: certs/signed/acert.pem\n");
  printf("  info: using pubkey file: certs/signed/acert_pubkey.pem\n");
  printf("  info: PEM_read_bio_X509_ACERT: good\n");
  printf("  info: acert version: 1\n");
  printf("  info: PEM_read_bio_PUBKEY: good\n");
  printf("  info: X509_ACERT_verify: good\n");
  printf("  info: acert_do_test: good\n");
  printf("  success\n");
  exit(EXIT_FAILURE);
}
