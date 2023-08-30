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

/* Prototypes are in ccbVaultIc.java */

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


JNIEXPORT jint JNICALL
Java_com_wolfssl_ccbvaultic_ccbVaultIc_UpdateDefaultAuth(JNIEnv *env, jobject thiz, jint id,
                                                         jint role, jint kind, jobject in1,
                                                         jlongArray in1_sz, jobject in2,
                                                         jlongArray in2_sz) {
    // TODO: implement UpdateDefaultAuth()
    return (jint)CRYPTOCB_UNAVAILABLE;
}

JNIEXPORT jint JNICALL
Java_com_wolfssl_ccbvaultic_ccbVaultIc_ProvisionAction(JNIEnv *env, jobject thiz, jint devId,
                                                       jboolean selftest, jint id, jint role,
                                                       jint kind, jstring in_name1, jobject in1,
                                                       jlongArray in1_sz, jstring in_name2,
                                                       jobject in2, jlongArray in2_sz,
                                                       jstring in_name3, jobject in3,
                                                       jlongArray in3_sz) {
    // TODO: implement ProvisionAction()

    //ccbVaultIc_Provision prov;
    //prov.self_test= selftest;
    //switch(kind)
    //{
        //setup auth.
    //}
    //Set the file data


    /* Invoke the callback */
    //int rc = wc_CryptoCb_Command((int)devId, CCBVAULTIC_CMD_PROVISIONACTION, (void*) &info, NULL);

    //return (jint)rc;
    return (jint)CRYPTOCB_UNAVAILABLE;
}

JNIEXPORT jint JNICALL
Java_com_wolfssl_ccbvaultic_ccbVaultIc_LoadAction(JNIEnv *env, jobject thiz, jint devId,
                                                  jstring in_name1, jobject in1, jlongArray in1_sz,
                                                  jstring in_name2, jobject in2, jlongArray in2_sz,
                                                  jstring in_name3, jobject in3,
                                                  jlongArray in3_sz) {
    // TODO: implement LoadAction()
    //Handle null or emtpy strings
    return (jint)CRYPTOCB_UNAVAILABLE;
}

JNIEXPORT jint JNICALL
Java_com_wolfssl_ccbvaultic_ccbVaultIc_PerformSelfTest(JNIEnv *env, jobject thiz, jint devId) {
    // TODO: implement PerformSelfTest()
    return (jint)CRYPTOCB_UNAVAILABLE;
}