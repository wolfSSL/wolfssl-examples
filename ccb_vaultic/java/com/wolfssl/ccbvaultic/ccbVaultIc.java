package com.wolfssl.ccbvaultic;

import java.nio.ByteBuffer;

import javax.net.ssl.KeyManager;
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


    /* Buffer should be at least CCBVAULTIC_INFO_LEN bytes */
    public native int GetInfoText(int devId, ByteBuffer out, long[] outSz);

    public int GetInfoText(int devId, byte[] out)
    {
        ByteBuffer text = ByteBuffer.allocateDirect(CCBVAULTIC_INFO_LEN);
        long[] text_len = {CCBVAULTIC_INFO_LEN};
        int rc = GetInfoText(devId, text, text_len);

        if (rc == 0) {
            text.get(out, 0, (int)text_len[0]);
        }
        return rc;
    }

    public native int UpdateDefaultAuth(int id, int role, int kind,
                                        ByteBuffer in1, long[] in1Sz,
                                        ByteBuffer in2, long[] in2Sz);

    public int UpdateDefaultAuth_Pin(int id,  int role, byte[] pin)
    {
        ByteBuffer pinBytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_PIN_LEN_MAX);
        pinBytes.put(pin, 0, pin.length);
        long[] pinLen = {(long)pin.length};

        return UpdateDefaultAuth(id, role, CCBVAULTIC_AUTH_KIND_PIN,
                pinBytes, pinLen,
                null, null);
    }

    /* Role should be CCBVAULTIC_AUTH_ROLE_APPROVED or _UNAPPROVED */
    public int UpdateDefaultAuth_SCP03(int id,  int role, byte[] mac, byte[] enc)
    {
        ByteBuffer macBytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_MAC_LEN);
        macBytes.put(mac, 0, mac.length);
        long[] macLen = {(long)mac.length};

        ByteBuffer encBytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_ENC_LEN);
        encBytes.put(enc, 0, enc.length);
        long[] encLen = {(long)enc.length};

        return UpdateDefaultAuth(id, role, CCBVAULTIC_AUTH_KIND_SCP03,
                macBytes, macLen,
                encBytes, encLen);
    }

    /* Role should be CCBVAULTIC_AUTH_ROLE_APPROVED or _UNAPPROVED */
    public int UpdateDefaultAuth_KDF(int id,  int role, byte[] key, byte[] label)
    {
        ByteBuffer keyBytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_KDF_KEY_LEN_MAX);
        keyBytes.put(key, 0, key.length);
        long[] keyLen = {(long)key.length};

        ByteBuffer labelBytes = ByteBuffer.allocateDirect(CCBVAULTIC_AUTH_KDF_LABEL_LEN_MAX);
        labelBytes.put(label, 0, label.length);
        long[] labelLen = {(long)label.length};

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

    /* Debug */
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
    public int ProvisionAction_KDF(int devId,
                                   boolean poweron_selftest,
                                   int id, int role,        //create user data
                                   byte[] key, byte[] label,
                                   String inName1, byte[] in1,
                                   String inName2, byte[] in2,
                                   String inName3, byte[] in3)
    {
        //return ProvisionAction(devId, poweron_selftest, id, role, CCBVAULTIC_AUTH_KIND_KDF,
        // keyBytes, keyLen, labelBytes, labelLen,
        // inName1, in1Bytes, in1Len,
        // inName1, in2Bytes, in2Len,
        // inName1, in3Bytes, in3Len,)
        return -1;
    }
    public int ProvisionAction_App(int devId, boolean poweron_selftest,
                                   byte[] key, byte[] label,
                                   String inName1, byte[] in1,
                                   String inName2, byte[] in2,
                                   String inName3, byte[] in3)
    {
        return ProvisionAction_KDF(devId, poweron_selftest,
                CCBVAULTIC_AUTH_ID_APP,CCBVAULTIC_AUTH_ROLE_APP,
                key, label,inName1, in1, inName2, in2, inName3, in3);
    }
    public native int LoadAction(int devId,
                                      String inName1, ByteBuffer in1, long[] in1Sz,
                                      String inName2, ByteBuffer in2, long[] in2Sz,
                                      String inName3, ByteBuffer in3, long[] in3Sz);

    public int LoadAction(int devId,
                            String inName1, byte[] in1,
                            String inName2, byte[] in2,
                            String inName3, byte[] in3)
    {
        //return LoadAction(devId, inName1, in1Bytes, in1Len,
        //        inName2, in2Bytes, in2Len,
        //        inName3, in3Bytes, in3Len);
        return -1;
    }
    public KeyManager GenerateKM(byte[] keyFile, byte[] crtFile)
    {
        return null;
    }

    public TrustManager GenerateTM(byte[] cafile)
    {
        return null;
    }

    public native int PerformSelfTest(int devId);

}
