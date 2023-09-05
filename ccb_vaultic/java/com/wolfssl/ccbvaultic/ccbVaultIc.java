package com.wolfssl.ccbvaultic;

import android.util.Base64;

import java.io.ByteArrayInputStream;
import java.nio.ByteBuffer;
import java.security.BasicPermission;
import java.security.KeyFactory;
import java.security.KeyStore;
import java.security.PrivateKey;
import java.security.cert.Certificate;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.security.spec.EncodedKeySpec;
import java.security.spec.PKCS8EncodedKeySpec;
import java.util.Arrays;
import java.util.Collection;

import javax.net.ssl.KeyManager;
import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.TrustManager;

public class ccbVaultIc {
    static {
        System.loadLibrary("ccbvaulticjni");
    }

    /* From ccb_vaultic.h */
    public static final int CCBVAULTIC_SERIAL_LEN               = 8;
    public static final int CCBVAULTIC_VERSION_LEN              = 32;

    public static final int CCBVAULTIC_CMD_INFO                 = 0x8000;
    public static final int CCBVAULTIC_CMD_LOADACTION           = 0x8001;
    public static final int CCBVAULTIC_CMD_PROVISIONACTION      = 0x8002;

    public static final int CCBVAULTIC_CMD_NVMREAD              = 0x8100;

    public static final int CCBVAULTIC_INFO_LEN                 = 128;
    public static final int CCBVAULTIC_FAST_START_MS   = 700;
    public static final int CCBVAULTIC_SLOW_START_MS   = 5000;
    public static final int CCBVAULTIC_APDU_TIMEOUT_MS = 5000;
    public static final int CCBVAULTIC_SPI_RATE_KHZ    = 5000;
    public static final int CCBVAULTIC_FILE_NAME_LEN_MIN = 0x1;     /* At  least a NULL */
    public static final int CCBVAULTIC_FILE_NAME_LEN_MAX = 0x10;    /* 16 byte limit for names */
    public static final int CCBVAULTIC_FILE_DATA_LEN_MAX = 0x10000; /* 16kB limit for files */
    public static final int CCBVAULTIC_AUTH_ID_MIN              = 0;
    public static final int CCBVAULTIC_AUTH_ID_MAX              = 7;

    public static final int CCBVAULTIC_AUTH_ROLE_NONE           = 0;
    public static final int CCBVAULTIC_AUTH_ROLE_MANUFACTURER   = 1;
    public static final int CCBVAULTIC_AUTH_ROLE_UNAPPROVED     = 2;
    public static final int CCBVAULTIC_AUTH_ROLE_APPROVED       = 3;

    public static final int CCBVAULTIC_AUTH_KIND_NONE           = 0;
    public static final int CCBVAULTIC_AUTH_KIND_PIN            = 1;
    public static final int CCBVAULTIC_AUTH_KIND_SCP03          = 2;
    public static final int CCBVAULTIC_AUTH_KIND_KDF            = 3;

    public static final int CCBVAULTIC_AUTH_PIN_LEN_MIN = 0x4;
    public static final int CCBVAULTIC_AUTH_PIN_LEN_MAX = 0x20;

    public static final int CCBVAULTIC_AUTH_MAC_LEN = 0x10;          /* AES128 keys only */
    public static final int CCBVAULTIC_AUTH_ENC_LEN = 0x10;          /* AES128 keys only */

    public static final int CCBVAULTIC_AUTH_KDF_KEY_LEN_MIN = 0x10;
    public static final int CCBVAULTIC_AUTH_KDF_KEY_LEN_MAX = 0x40;
    public static final int CCBVAULTIC_AUTH_KDF_LABEL_LEN_MIN = 0x0;
    public static final int CCBVAULTIC_AUTH_KDF_LABEL_LEN_MAX = 0x40;

    public static final int CCBVAULTIC_AUTH_ROLE_MANU = CCBVAULTIC_AUTH_ROLE_MANUFACTURER;
    public static final int CCBVAULTIC_AUTH_ROLE_APP = CCBVAULTIC_AUTH_ROLE_APPROVED;
    public static final int CCBVAULTIC_AUTH_ROLE_DEBUG = CCBVAULTIC_AUTH_ROLE_UNAPPROVED;

    public static final int CCBVAULTIC_AUTH_ID_MANU = 7;
    public static final int CCBVAULTIC_AUTH_ID_APP = 0;
    public static final int CCBVAULTIC_AUTH_ID_DEBUG = 1;


    public void SetByteBufferWithLen(ByteBuffer byteBuffer, long[] len, byte[] inData)
    {
        if ((byteBuffer == null) || (len == null)) {
            return;
        }
        if((inData == null) || (inData.length == 0)) {
            len[0] = 0;
            return;
        }
        int minLen = Math.min(byteBuffer.limit(), inData.length);
        byteBuffer.put(inData, 0 , minLen);
        len[0] = minLen;
    }

    public byte[] GetByteBufferWithLen(ByteBuffer byteBuffer, long[] len)
    {
        if ((byteBuffer == null) || (len == null) || (len[0] <= 0)) {
            return null;
        }
        byte[] out = new byte[(int)len[0]];
        byteBuffer.get(out, 0, (int)len[0]);
        return out;
    }

    /* Buffer should be at least CCBVAULTIC_INFO_LEN bytes */
    public native int GetInfoText(int devId, ByteBuffer out, long[] outSz);

    public int GetInfoText(int devId, byte[] out)
    {
        ByteBuffer text = ByteBuffer.allocateDirect(CCBVAULTIC_INFO_LEN);
        long[] text_len = {CCBVAULTIC_INFO_LEN};
        int rc = GetInfoText(devId, text, text_len);

        if (rc == 0) {
            int minLen = Math.min((int)text_len[0], out.length);
            text.get(out, 0, minLen);
        }
        return rc;
    }

    public native int UpdateDefaultAuth(int id, int role, int kind,
                                        ByteBuffer in1, long[] in1Sz,
                                        ByteBuffer in2, long[] in2Sz);

    public int UpdateDefaultAuth_Pin(int id,  int role, byte[] pin)
    {
        ByteBuffer pinBytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_PIN_LEN_MAX);
        long[] pinLen = {0};
        SetByteBufferWithLen(pinBytes, pinLen, pin);

        return UpdateDefaultAuth(id, role, CCBVAULTIC_AUTH_KIND_PIN,
                pinBytes, pinLen,
                null, null);
    }

    /* Role should be CCBVAULTIC_AUTH_ROLE_APPROVED or _UNAPPROVED */
    public int UpdateDefaultAuth_SCP03(int id,  int role, byte[] mac, byte[] enc)
    {
        ByteBuffer macBytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_MAC_LEN);
        long[] macLen = {0};

        ByteBuffer encBytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_ENC_LEN);
        long[] encLen = {0};

        SetByteBufferWithLen(macBytes, macLen, mac);
        SetByteBufferWithLen(encBytes, encLen, enc);

        return UpdateDefaultAuth(id, role, CCBVAULTIC_AUTH_KIND_SCP03,
                macBytes, macLen,
                encBytes, encLen);
    }

    /* Role should be CCBVAULTIC_AUTH_ROLE_APPROVED or _UNAPPROVED */
    public int UpdateDefaultAuth_KDF(int id,  int role, byte[] key, byte[] label)
    {
        ByteBuffer keyBytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_KDF_KEY_LEN_MAX);
        long[] keyLen = {0};

        ByteBuffer labelBytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_KDF_LABEL_LEN_MAX);
        long[] labelLen = {0};

        SetByteBufferWithLen(keyBytes, keyLen, key);
        SetByteBufferWithLen(labelBytes, labelLen, label);

        return UpdateDefaultAuth(id, role, CCBVAULTIC_AUTH_KIND_KDF,
                keyBytes, keyLen,
                labelBytes, labelLen);
    }

    /* Manufacturer login */
    public int UpdateDefaultAuth_Manu(byte[] pin)
    {
        return UpdateDefaultAuth_Pin(
                CCBVAULTIC_AUTH_ID_MANU,
                CCBVAULTIC_AUTH_ROLE_MANU,
                pin);
    }

    /* App Login */
    public int UpdateDefaultAuth_App(byte[] key, byte[] label)
    {
        return UpdateDefaultAuth_KDF(
                CCBVAULTIC_AUTH_ID_APP,
                CCBVAULTIC_AUTH_ROLE_APP,
                key, label);
    }

    /* Debug login*/
    public int UpdateDefaultAuth_Debug(byte[] mac, byte[] enc)
    {
        return UpdateDefaultAuth_SCP03(
                CCBVAULTIC_AUTH_ID_DEBUG,
                CCBVAULTIC_AUTH_ROLE_DEBUG,
                mac, enc);
    }


    public native int ProvisionAction(int devId,
                                      boolean poweron_selftest,
                                      int id, int role, int kind,  //create user data
                                      ByteBuffer userin1, long[] userin1Sz,
                                      ByteBuffer userin2, long[] userin2Sz,
                                      String inName1, ByteBuffer in1, long[] in1Sz,
                                      String inName2, ByteBuffer in2, long[] in2Sz,
                                      String inName3, ByteBuffer in3, long[] in3Sz);
    public int ProvisionAction( int devId,
                                boolean poweron_selftest,
                                int id, int role, int kind,
                                byte[] userin1, byte[] userin2,
                                String inName1, byte[] in1,
                                String inName2, byte[] in2,
                                String inName3, byte[] in3
                                )
    {
        ByteBuffer userin1Bytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_KDF_KEY_LEN_MAX);
        long[] userin1Len = {0};
        ByteBuffer userin2Bytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_KDF_LABEL_LEN_MAX);
        long[] userin2Len = {0};
        ByteBuffer in1Bytes = ByteBuffer.allocateDirect(CCBVAULTIC_FILE_DATA_LEN_MAX);
        long[] in1Len = {0};
        ByteBuffer in2Bytes = ByteBuffer.allocateDirect(CCBVAULTIC_FILE_DATA_LEN_MAX);
        long[] in2Len = {0};
        ByteBuffer in3Bytes = ByteBuffer.allocateDirect(CCBVAULTIC_FILE_DATA_LEN_MAX);
        long[] in3Len = {0};

        SetByteBufferWithLen(userin1Bytes, userin1Len, userin1);
        SetByteBufferWithLen(userin2Bytes, userin2Len, userin2);
        SetByteBufferWithLen(in1Bytes, in1Len, in1);
        SetByteBufferWithLen(in2Bytes, in2Len, in2);
        SetByteBufferWithLen(in3Bytes, in3Len, in3);

        return ProvisionAction(devId, poweron_selftest,
                id, role,
                kind, userin1Bytes, userin1Len, userin2Bytes, userin2Len,
                inName1, in1Bytes, in1Len,
                inName2, in2Bytes, in2Len,
                inName3, in3Bytes, in3Len);
    }
    public int ProvisionAction_KDF(int devId,
                                   boolean poweron_selftest,
                                   int id, int role,        //create user data
                                   byte[] key, byte[] label,
                                   String inName1, byte[] in1,
                                   String inName2, byte[] in2,
                                   String inName3, byte[] in3)
    {
        if((key ==  null) || (key.length < CCBVAULTIC_AUTH_KDF_KEY_LEN_MIN) ||
                (key.length > CCBVAULTIC_AUTH_KDF_KEY_LEN_MAX)) {
            return -1;
        }
        if((label ==  null) || (label.length < CCBVAULTIC_AUTH_KDF_LABEL_LEN_MIN) ||
                (label.length > CCBVAULTIC_AUTH_KDF_LABEL_LEN_MAX)) {
            return -1;
        }
        return ProvisionAction(devId, poweron_selftest,
                id, role,
                CCBVAULTIC_AUTH_KIND_KDF, key, label,
                inName1, in1,
                inName2, in2,
                inName3, in3);
    }
    public int ProvisionAction_App(int devId, boolean poweron_selftest,
                                   byte[] key, byte[] label,
                                   String inName1, byte[] in1,
                                   String inName2, byte[] in2,
                                   String inName3, byte[] in3)
    {
        return ProvisionAction_KDF(devId, poweron_selftest,
                CCBVAULTIC_AUTH_ID_APP, CCBVAULTIC_AUTH_ROLE_APP,
                key, label,inName1, in1, inName2, in2, inName3, in3);
    }

    /* Invoke the load action using the provided devId */
    public native int LoadAction(int devId,
                                      String inName1, ByteBuffer out1Bytes, long[] out1Len,
                                      String inName2, ByteBuffer out2Bytes, long[] out2Len,
                                      String inName3, ByteBuffer out3Bytes, long[] out3Len);

    public class LoadFiles {
        public int rc;
        public byte[] file1;
        public byte[] file2;
        public byte[] file3;
    }
    public LoadFiles LoadAction(int devId,
                            String inName1,
                            String inName2,
                            String inName3)
    {
        ByteBuffer out1Bytes = ByteBuffer.allocateDirect(CCBVAULTIC_FILE_DATA_LEN_MAX);
        long[] out1Len = {CCBVAULTIC_FILE_DATA_LEN_MAX};
        ByteBuffer out2Bytes = ByteBuffer.allocateDirect(CCBVAULTIC_FILE_DATA_LEN_MAX);
        long[] out2Len = {CCBVAULTIC_FILE_DATA_LEN_MAX};
        ByteBuffer out3Bytes = ByteBuffer.allocateDirect(CCBVAULTIC_FILE_DATA_LEN_MAX);
        long[] out3Len = {CCBVAULTIC_FILE_DATA_LEN_MAX};

        LoadFiles out = new LoadFiles();
        out.rc= LoadAction(devId,
                inName1, out1Bytes, out1Len,
                inName2, out2Bytes, out2Len,
                inName3, out3Bytes, out3Len);
        if (out.rc == 0) {
            out.file1 = GetByteBufferWithLen(out1Bytes,out1Len);
            out.file2 = GetByteBufferWithLen(out2Bytes,out2Len);
            out.file3 = GetByteBufferWithLen(out3Bytes,out3Len);
        }
        return out;
    }


    /* Copy data that is not a PEM header.  Note this does not handle non-base64 text prior to the
     * first  header. */
    private byte[] StripPemHeaders(byte[] data)
    {
        if (data == null) return null;
        /* Don't copy sections "-----xxxx-----"
         *                      ^head
         *                           ^desc
         *                               ^foot
         */
        int outSize = 0;
        byte[] outData = new byte[data.length];
        boolean inHead = false;
        boolean inDesc = false;
        boolean inFoot = false;
        for (byte datum : data) {
            if (datum == '-') {
                if (!inHead) {
                    inHead = true;
                } else {
                    if (inDesc) {
                        inFoot = true;
                    }
                }
                // Always remove -'s
            } else {
                // Not a -
                if (inFoot) {
                    //End of footer
                    inFoot = false;
                    inDesc = false;
                    inHead = false;
                } else {
                    if (inHead) {
                        inDesc = true;
                    }
                }
                if (!inDesc) {
                    //Keep this byte
                    outData[outSize++] = datum;
                }
            }
        }
        //Truncate the output
        return Arrays.copyOfRange(outData, 0, outSize);
    }
    public KeyManager[] GenerateKM(byte[] keyPem, byte[] crtPem)
    {
        /* Check for empty or missing files */
        if( (keyPem == null) || (keyPem.length == 0) ||
                (crtPem == null) || (crtPem.length == 0)) {
            return null;
        }
        try {
            byte[] keyFile = Base64.decode(StripPemHeaders(keyPem),Base64.DEFAULT);
            byte[] crtFile = Base64.decode(StripPemHeaders(crtPem),Base64.DEFAULT);

            /* Parse the provided keyFile into a PrivateKey */
            KeyFactory kF = KeyFactory.getInstance("RSA");
            PrivateKey pK = kF.generatePrivate(new PKCS8EncodedKeySpec(keyFile));

            /* Parse the provided crtFile into a X509Certificate[] CertChain */
            CertificateFactory cF = CertificateFactory.getInstance("X.509");
            Collection<? extends Certificate> certs =
                    cF.generateCertificates(new ByteArrayInputStream(crtFile));
            X509Certificate[] certChain = new X509Certificate[certs.size()];
            int i=0;
            for (Certificate cert:certs) {
                certChain[i++] = (X509Certificate) cert;
            }

            /* Create a PKCS12 KeyStore and put the PrivateKey and CertChain in */
            KeyStore kS = KeyStore.getInstance("PKCS12");
            kS.load(null, null);
            kS.setKeyEntry("alias", pK, null, certChain);

            /* Create a KeyManagerFactory and Init it using the KeyStore */
            KeyManagerFactory kmf = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());
            kmf.init(kS, null);
            return kmf.getKeyManagers();
        }
        catch (Exception e){
            /* Do something?*/
        }
        return null;
    }

    public TrustManager[] GenerateTM(byte[] cafile)
    {

        return null;
    }

    public native int PerformSelfTest(int devId);

}
