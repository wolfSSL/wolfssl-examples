package com.wolfssl.ccbvaultic;

import java.nio.ByteBuffer;

public class ccbVaultIc {
    static {
        System.loadLibrary("ccbvaulticjni");
    }
    public native int GetInfoText(int devId, ByteBuffer out, long[] outSz);
}
