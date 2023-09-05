#include <jni.h>

/* wolfSSL configuration */
#ifndef WOLFSSL_USER_SETTINGS
    #include "wolfssl/options.h"
#endif
#include "wolfssl/wolfcrypt/settings.h"

/* wolfCrypt includes */
#include "wolfssl/wolfcrypt/cryptocb.h"    /* For wc_cryptInfo */
#include "wolfssl/wolfcrypt/error-crypt.h" /* For error values */

#include "ccb_vaultic.h"

#ifdef HAVE_CCBVAULTIC
static char defaultAuthStorage1[CCBVAULTIC_AUTH_KDF_KEY_LEN_MAX];
static char defaultAuthStorage2[CCBVAULTIC_AUTH_KDF_LABEL_LEN_MAX];
static char fileNames[3][CCBVAULTIC_FILE_NAME_LEN_MAX];
#endif

/* Prototypes are in ccbVaultIc.java */

JNIEXPORT jint JNICALL Java_com_wolfssl_ccbvaultic_ccbVaultIc_GetInfoText (
        JNIEnv* jenv, jobject jcl,
        jint devId, jobject out, jlongArray outSz)
{
#ifndef HAVE_CCBVAULTIC
    (void)jenv; (void)jcl; (void)devId; (void)out; (void)outSz;
    return (jint)CRYPTOCB_UNAVAILABLE;
#else
    (void)jcl;
    int rc = 0;
    jlong jSize = 0;
    ccbVaultIc_Info info;

    XMEMSET(&info, 0, sizeof(info));

    /* Ok if this buffer is NULL */
    info.text = (*jenv)->GetDirectBufferAddress(jenv, out);

    /* Ok if this length is 0 */
    (*jenv)->GetLongArrayRegion(jenv, outSz, 0, 1, &jSize);
    info.text_len = (int)jSize;

    /* Invoke the callback */
    rc = wc_CryptoCb_Command((int)devId, CCBVAULTIC_CMD_INFO, (void*) &info, NULL);
    if(rc == 0) {
        /* Update the output length */
        jSize = info.text_len;
        (*jenv)->SetLongArrayRegion(jenv, outSz, 0, 1, (jlong*)&jSize);
    }
    return (jint)rc;
#endif
}


JNIEXPORT jint JNICALL Java_com_wolfssl_ccbvaultic_ccbVaultIc_UpdateDefaultAuth(
        JNIEnv *jenv, jobject jcl,
        jint id, jint role, jint kind,
        jobject in1, jlongArray in1_sz,
        jobject in2, jlongArray in2_sz)
{
#ifndef HAVE_CCBVAULTIC
    (void)jenv; (void)jcl; (void)id; (void)role; (void)kind;
    (void)in1; (void)in1_sz; (void)in2; (void)in2_sz;
    return (jint)CRYPTOCB_UNAVAILABLE;
#else
    int rc = 0;

    char *in1p = NULL;
    char *in2p = NULL;
    jlong in1Len = 0;
    jlong in2Len = 0;

    if (in1 != NULL) in1p = (*jenv)->GetDirectBufferAddress(jenv, in1);
    if (in2 != NULL) in2p = (*jenv)->GetDirectBufferAddress(jenv, in2);

    /* Ok if this length is 0  for now */
    if (in1_sz != NULL) (*jenv)->GetLongArrayRegion(jenv, in1_sz, 0, 1, &in1Len);
    if (in2_sz != NULL) (*jenv)->GetLongArrayRegion(jenv, in2_sz, 0, 1, &in2Len);

    switch((int)kind) {
        case CCBVAULTIC_AUTH_KIND_NONE:
        {
            gDefaultConfig.auth.id = (int)id;
            gDefaultConfig.auth.role = (int)role;
            gDefaultConfig.auth.kind = CCBVAULTIC_AUTH_KIND_NONE;
            rc = 0;
        }; break;
        case CCBVAULTIC_AUTH_KIND_PIN: {
            if((in1p == NULL) || (in1Len < CCBVAULTIC_AUTH_PIN_LEN_MIN) ||
                    (in1Len > CCBVAULTIC_AUTH_PIN_LEN_MAX)) {
                rc = BAD_FUNC_ARG;
                break;
            }
            /* Save updated info */
            XMEMCPY(defaultAuthStorage1, in1p, in1Len);

            gDefaultConfig.auth.id = (int)id;
            gDefaultConfig.auth.role = (int)role;
            gDefaultConfig.auth.kind = CCBVAULTIC_AUTH_KIND_PIN;
            gDefaultConfig.auth.auth.pin.pin = defaultAuthStorage1;
            gDefaultConfig.auth.auth.pin.pin_len = in1Len;
            rc = 0;
        };break;
        case CCBVAULTIC_AUTH_KIND_SCP03: {
            if((in1p == NULL) || (in1Len != CCBVAULTIC_AUTH_MAC_LEN) ||
                    (in2p == NULL) || (in2Len != CCBVAULTIC_AUTH_ENC_LEN)) {
                rc = BAD_FUNC_ARG;
                break;
            }
            /* Save updated info */
            XMEMCPY(defaultAuthStorage1, in1p, CCBVAULTIC_AUTH_MAC_LEN);
            XMEMCPY(defaultAuthStorage2, in2p, CCBVAULTIC_AUTH_ENC_LEN);

            gDefaultConfig.auth.id = (int)id;
            gDefaultConfig.auth.role = (int)role;
            gDefaultConfig.auth.kind = CCBVAULTIC_AUTH_KIND_SCP03;
            gDefaultConfig.auth.auth.scp03.mac = defaultAuthStorage1;
            gDefaultConfig.auth.auth.scp03.mac_len = in1Len;
            gDefaultConfig.auth.auth.scp03.enc = defaultAuthStorage2;
            gDefaultConfig.auth.auth.scp03.enc_len = in2Len;
            rc = 0;
        };break;
        case CCBVAULTIC_AUTH_KIND_KDF: {
            if((in1p == NULL) || (in1Len < CCBVAULTIC_AUTH_KDF_KEY_LEN_MIN) ||
                    (in1Len > CCBVAULTIC_AUTH_KDF_KEY_LEN_MAX) ||
               (in2p == NULL) || (in2Len != CCBVAULTIC_AUTH_KDF_LABEL_LEN_MIN) ||
                    (in2Len > CCBVAULTIC_AUTH_KDF_LABEL_LEN_MAX)) {
                rc = BAD_FUNC_ARG;
                break;
            }
            /* Save updated info */
            XMEMCPY(defaultAuthStorage1, in1p, in1Len);
            XMEMCPY(defaultAuthStorage2, in2p, in2Len);

            gDefaultConfig.auth.id = (int)id;
            gDefaultConfig.auth.role = (int)role;
            gDefaultConfig.auth.kind = CCBVAULTIC_AUTH_KIND_KDF;
            gDefaultConfig.auth.auth.scp03.mac = defaultAuthStorage1;
            gDefaultConfig.auth.auth.scp03.mac_len = in1Len;
            gDefaultConfig.auth.auth.scp03.enc = defaultAuthStorage2;
            gDefaultConfig.auth.auth.scp03.enc_len = in2Len;
            rc = 0;
        };break;
        default:
            rc = BAD_FUNC_ARG;
    }
    return (jint)rc;
#endif
}

JNIEXPORT jint JNICALL Java_com_wolfssl_ccbvaultic_ccbVaultIc_ProvisionAction(
        JNIEnv *jenv, jobject jcl,
       jint devId, jboolean selftest,
       jint id, jint role, jint kind,
        jobject userin1, jlongArray userin1_sz,
        jobject userin2, jlongArray userin2_sz,
       jstring in_name1, jobject in1, jlongArray in1_sz,
       jstring in_name2, jobject in2, jlongArray in2_sz,
       jstring in_name3, jobject in3, jlongArray in3_sz)
{
#ifndef HAVE_CCBVAULTIC
    (void)jenv; (void)jcl; (void)devId; (void)selftest;
    (void)id; (void)role; (void)kind;

    (void)in_name1; (void)in1; (void)in1_sz;
    (void)in_name2; (void)in2; (void)in2_sz;
    (void)in_name3; (void)in3; (void)in3_sz;
    return (jint)CRYPTOCB_UNAVAILABLE;
#else
    // TODO: implement ProvisionAction()

    ccbVaultIc_Provision prov;
    jlong jSize;
    int rc = 0;
    ccbVaultIc_Load l;
    ccbVaultIc_File files[3];

    XMEMSET(&prov, 0, sizeof(prov));
    prov.self_test = (int)selftest;
    prov.create.id = (int)id;
    prov.create.role = (int)role;

    switch((int)kind) {
        case CCBVAULTIC_AUTH_KIND_KDF: {
            /*
            prov.create.auth.kdf.key
            prov.create.auth.kdf.label
             */
        };break;

        case CCBVAULTIC_AUTH_KIND_SCP03: {
            /*
            prov.create.auth.scp03.mac
            prov.create.auth.scp03.enc
             */
        };break;

        case CCBVAULTIC_AUTH_KIND_PIN:
        case CCBVAULTIC_AUTH_KIND_NONE:
        default:
            rc = BAD_FUNC_ARG;
    }
    if(rc == 0) {
        //Set the file data

        XMEMSET(&l, 0, sizeof(l));
        XMEMSET(files, 0, sizeof(files));
        l.file_count = 3;
        l.file = files;

        /* Setup file 1 */
        if (in_name1 != NULL) {
            l.file[0].name = (*jenv)->GetStringUTFChars(jenv, in_name1, NULL);
            if (l.file[0].name != NULL) {
                l.file[0].name_len = XSTRLEN(l.file[0].name);
                l.file[0].data = (*jenv)->GetDirectBufferAddress(jenv, in1);
                (*jenv)->GetLongArrayRegion(jenv, in1_sz, 0, 1, &jSize);
                l.file[0].data_len = jSize;
            }
        }

        /* Setup file 2 */
        if (in_name2 != NULL) {
            l.file[1].name = (*jenv)->GetStringUTFChars(jenv, in_name2, NULL);
            if (l.file[1].name != NULL) {
                l.file[1].name_len = XSTRLEN(l.file[1].name);
                l.file[1].data = (*jenv)->GetDirectBufferAddress(jenv, in2);
                (*jenv)->GetLongArrayRegion(jenv, in2_sz, 0, 1, &jSize);
                l.file[1].data_len = jSize;
            }
        }

        /* Setup file 3 */
        if (in_name3 != NULL) {
            l.file[2].name = (*jenv)->GetStringUTFChars(jenv, in_name3, NULL);
            if (l.file[2].name != NULL) {
                l.file[2].name_len = XSTRLEN(l.file[2].name);
                l.file[2].data = (*jenv)->GetDirectBufferAddress(jenv, in3);
                (*jenv)->GetLongArrayRegion(jenv, in3_sz, 0, 1, &jSize);
                l.file[2].data_len = jSize;
            }
        }
        /* Invoke the callback */
        rc = wc_CryptoCb_Command((int)devId, CCBVAULTIC_CMD_PROVISIONACTION, (void*) &prov, NULL);
    }
    //Cleanup strings
    /*
     *
     */
    return (jint)rc;
#endif
}

JNIEXPORT jint JNICALL Java_com_wolfssl_ccbvaultic_ccbVaultIc_LoadAction(
        JNIEnv *jenv, jobject jcl,
        jint devId,
        jstring in_name1, jobject in1, jlongArray in1_sz,
        jstring in_name2, jobject in2, jlongArray in2_sz,
        jstring in_name3, jobject in3, jlongArray in3_sz)
{
#ifndef HAVE_CCBVAULTIC
    (void)jenv; (void)jcl; (void)devId;
    (void)in_name1; (void)in1; (void)in1_sz;
    (void)in_name2; (void)in2; (void)in2_sz;
    (void)in_name3; (void)in3; (void)in3_sz;
    return (jint)CRYPTOCB_UNAVAILABLE;
#else
    jlong jSize;
    int rc = 0;
    ccbVaultIc_Load l;
    ccbVaultIc_File files[3];

    XMEMSET(&l, 0, sizeof(l));
    XMEMSET(files, 0, sizeof(files));
    l.file_count = 3;
    l.file = files;

    /* Setup file 1 */
    if(in_name1 != NULL) {
        l.file[0].name = (*jenv)->GetStringUTFChars(jenv, in_name1, NULL);
        if (l.file[0].name != NULL) {
            l.file[0].name_len = XSTRLEN(l.file[0].name);
            l.file[0].data = (*jenv)->GetDirectBufferAddress(jenv, in1);
            (*jenv)->GetLongArrayRegion(jenv, in1_sz, 0, 1, &jSize);
            l.file[0].data_len = jSize;
        }
    }

    /* Setup file 2 */
    if(in_name2 != NULL) {
        l.file[1].name = (*jenv)->GetStringUTFChars(jenv, in_name2, NULL);
        if (l.file[1].name != NULL) {
            l.file[1].name_len = XSTRLEN(l.file[1].name);
            l.file[1].data = (*jenv)->GetDirectBufferAddress(jenv, in2);
            (*jenv)->GetLongArrayRegion(jenv, in2_sz, 0, 1, &jSize);
            l.file[1].data_len = jSize;
        }
    }

    /* Setup file 3 */
    if(in_name3 != NULL) {
        l.file[2].name = (*jenv)->GetStringUTFChars(jenv, in_name3, NULL);
        if (l.file[2].name != NULL) {
            l.file[2].name_len = XSTRLEN(l.file[2].name);
            l.file[2].data = (*jenv)->GetDirectBufferAddress(jenv, in3);
            (*jenv)->GetLongArrayRegion(jenv, in3_sz, 0, 1, &jSize);
            l.file[2].data_len = jSize;
        }
    }

    rc = wc_CryptoCb_Command((int)devId, CCBVAULTIC_CMD_LOADACTION, (void*) &l, NULL);
    if(rc == 0) {
        /* Update the output lengths */
        jSize = l.file[0].data_len;
        (*jenv)->SetLongArrayRegion(jenv, in1_sz, 0, 1, (jlong*)&jSize);
        jSize = l.file[1].data_len;
        (*jenv)->SetLongArrayRegion(jenv, in2_sz, 0, 1, (jlong*)&jSize);
        jSize = l.file[2].data_len;
        (*jenv)->SetLongArrayRegion(jenv, in3_sz, 0, 1, (jlong*)&jSize);
    }

    if ((in_name1 != NULL) && (l.file[0].name != NULL)) {
        (*jenv)->ReleaseStringUTFChars(jenv, in_name1, l.file[0].name);
    }
    if ((in_name2 != NULL) && (l.file[1].name != NULL)) {
        (*jenv)->ReleaseStringUTFChars(jenv, in_name2, l.file[1].name);
    }
    if ((in_name3 != NULL) && (l.file[2].name != NULL)) {
        (*jenv)->ReleaseStringUTFChars(jenv, in_name3, l.file[2].name);
    }
    return rc;
#endif
}

JNIEXPORT jint JNICALL Java_com_wolfssl_ccbvaultic_ccbVaultIc_PerformSelfTest(
        JNIEnv *jenv, jobject jcl,
        jint devId)
{
#ifndef HAVE_CCBVAULTIC
    (void)jenv; (void)jcl; (void)devId;
    return (jint)CRYPTOCB_UNAVAILABLE;
#else
    (void)jcl;
    int rc = 0;


    /* Invoke the callback */
    rc = wc_CryptoCb_Command((int)devId, CCBVAULTIC_CMD_SELFTEST, NULL, NULL);
    return (jint)rc;

#endif
}