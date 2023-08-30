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

#include "com_wolfssl_ccbvaultic_ccbVaultIc.h"
/*
 * Class:     com_wolfssl_ccbvaultic_ccbVaultIc
 * Method:    GetInfoText
 * Signature: (I[B)I
 */
JNIEXPORT jint JNICALL Java_com_wolfssl_ccbvaultic_ccbVaultIc_GetInfoText
  (JNIEnv* jenv, jobject jcl, jint devId, jobject out, jlongArray outSz)
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

