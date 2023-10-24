package com.wolfssl.wolfssljni_ndk_gradle;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.wolfssl.WolfSSL;
import com.wolfssl.provider.jsse.WolfSSLKeyX509;
import com.wolfssl.provider.jsse.WolfSSLProvider;

import com.wolfssl.ccbvaultic.ccbVaultIc;

import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.security.KeyStore;
import java.security.Security;
import java.util.concurrent.Executors;

import javax.net.ssl.KeyManager;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;

public class MainActivity extends AppCompatActivity {

    public native int testWolfCrypt();
    public native int benchWolfCrypt();

    private static final String host = "www.wolfssl.com";
    private static final int port = 443;

    private int connectCount = 0;

    private void setDisplayText(String s)
    {
        runOnUiThread(() -> {
            TextView tv = (TextView) findViewById(R.id.displayText);
            tv.setText(s);
        });
    }
    private void appendDisplayText(String s)
    {
        runOnUiThread(() -> {
            TextView tv = (TextView) findViewById(R.id.displayText);
            tv.append(s);
        });
    }
    private final View.OnClickListener wcTestListener = v -> {
        setDisplayText("Running wolfCrypt tests.\n" +
                   "See logcat output for details (adb logcat).\n" +
                   "...\n");

        Executors.newSingleThreadExecutor().execute(() -> {
            int ret = testWolfCrypt();
            if (ret == 0) {
                appendDisplayText("TESTS PASSED\n");
            } else {
                appendDisplayText("TESTS FAILED!\nInspect logcat.");
            }
            appendDisplayText("\nTo adjust wolfSSL configuration, please edit\n" +
                    "add_definitions() in CMakeLists.txt.");
        });

    };

    private final View.OnClickListener wcBenchmarkListener = v -> {
        setDisplayText("Running wolfCrypt benchmark.\n" +
                "See logcat output for details (adb logcat).\n" +
                "...\n");

        Executors.newSingleThreadExecutor().execute(() -> {
            int ret = benchWolfCrypt();
            if (ret == 0) {
                appendDisplayText("BENCHMARK PASSED\n");
            } else {
                appendDisplayText("BENCHMARK FAILED!\nInspect logcat.");
            }
            appendDisplayText("\nTo adjust wolfSSL configuration, please edit\n" +
                      "add_definitions() in CMakeLists.txt.");
        });
    };

    String[] filenames = {"/key.pem","/crt.pem",null}; //"/ca.pem"

    byte[] debugPin = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};

    byte[] debugMac = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                        0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
    byte[] debugEnc = { 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
                        0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F};


    byte[] debugKey = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                        0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
                        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
                        0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F};
    byte[] debugLabel = "Debug Label".getBytes();

    byte[] debugFile1 = "FILE1".getBytes();
    byte[] debugFile2 = "FILE2".getBytes();


    public String getCmd = "GET / HTTP/1.1\r\n" +
            "Host: www.wolfssl.com\r\n" +
            "Accept: */*\r\n\r\n";

    private final View.OnClickListener sslSocketConnectListener = v -> {
        setDisplayText("Making simple SSL/TLS connection to:" +
                host + ":" + port + "\n" +
                "See logcat output for details (adb logcat).\n");
        Executors.newSingleThreadExecutor().execute(() -> {
            try {
                /* Enable wolfJSSE debug messages */
                System.setProperty("wolfjsse.debug", "true");

                /* Select The devId which will be registered and used on every other invocation */
                /* Unique devId's are compiled into wolfssljni */
                int devId = 0x56490420;  /* VaultIC 420 */
                //int devId = WolfSSL.INVALID_DEVID;  /* No hardware offload */

                /* Select if the cipher suites and protocols will be limited */
                boolean limitCipherProtocol = true;
                //boolean limitCipherProtocol = false;

                /* Select if using Debug or KDF user for HW provisioning and connection */
                boolean useKdfUser = true;
                //boolean useKdfUser = false;

                long[] ts = new long[10];

                ts[0] = System.currentTimeMillis();
                WolfSSLProvider wolfProv = new WolfSSLProvider();
                appendDisplayText("A. Inserting Provider " + wolfProv.getName() + "\n");
                Security.insertProviderAt(wolfProv, 1);

                ts[1] = System.currentTimeMillis();
                connectCount++;

                KeyManager[] km = null;
                TrustManager[] tm = null;
                long totalRead = 0;
                ccbVaultIc cv = new ccbVaultIc();

                if(devId != WolfSSL.INVALID_DEVID) {
                    switch(connectCount % 3) {
                        case 1:  //SW
                        {
                            appendDisplayText("B. Using software crypto.\n");
                            wolfProv.setDevId(WolfSSL.INVALID_DEVID);
                        };break;
                        case 2:  //HW Manu
                        {
                            appendDisplayText("B. Using hardware offload - MANU\n");

                            // MANU
                            cv.UpdateDefaultAuth_Manu(debugPin);

                            wolfProv.registerDevId(devId);
                            wolfProv.setDevId(devId);

                            int rc = 0;
                            if(useKdfUser) {
                                rc = cv.ProvisionAction_App(
                                        devId, Boolean.TRUE,
                                        debugKey, debugLabel,
                                        filenames[0], debugFile1,
                                        filenames[1], debugFile2,
                                        null, null);
                            } else {
                                rc = cv.ProvisionAction_Debug(
                                        devId, Boolean.TRUE,
                                        debugMac, debugEnc,
                                        filenames[0], debugFile1,
                                        filenames[1], debugFile2,
                                        null, null);
                            }
                            appendDisplayText("Provision rc=" + rc + "\n" );
                            byte[] data = new byte[ccbVaultIc.CCBVAULTIC_INFO_LEN];
                            rc = cv.GetInfoText(devId, data);
                            appendDisplayText("GetInfoText rc=" + rc + "\n" + new String(data));

                            rc = cv.PerformSelfTest(devId);
                            appendDisplayText("SelfTest rc=" + rc + "\n");
                            // Skip the connection tests
                        };break;
                        case 0:  //HW App
                        {
                            appendDisplayText("B. Using hardware offload - APP\n");

                            if(useKdfUser) {
                                cv.UpdateDefaultAuth_App(debugKey, debugLabel);
                            } else {
                                cv.UpdateDefaultAuth_Debug(debugMac, debugEnc);
                            }

                            wolfProv.registerDevId(devId);
                            wolfProv.setDevId(devId);

                            //Load static data
                            //km = cv.GenerateKM(medKeyFile.getBytes(), medCrtFile.getBytes());

                            //Load data from VaultIC
                            ccbVaultIc.LoadFiles loadFiles = cv.LoadAction(devId,
                                    filenames[0], filenames[1],filenames[2]);
                            if (loadFiles.rc == 0) {
                                int[] sizes = new int[3];
                                if (loadFiles.file1 != null) sizes[0] = loadFiles.file1.length;
                                if (loadFiles.file2 != null) sizes[1] = loadFiles.file2.length;
                                if (loadFiles.file3 != null) sizes[2] = loadFiles.file3.length;
                                appendDisplayText("Loaded File sizes: " +
                                        " 1:" + sizes[0] +
                                        " 2:" + sizes[1] +
                                        " 3:" + sizes[2] +
                                        "\n");
                                km = cv.GenerateKM(loadFiles.file1, loadFiles.file2);
                                tm = cv.GenerateTM(loadFiles.file3);
                            }

                            byte[] data = new byte[ccbVaultIc.CCBVAULTIC_INFO_LEN];
                            int rc = cv.GetInfoText(devId, data);
                            appendDisplayText("GetInfoText rc=" + rc + "\n" + new String(data));
                        };break;
                    }
                }
                else {
                    appendDisplayText("B. Using internal crypto.\n");
                    wolfProv.setDevId(WolfSSL.INVALID_DEVID);
                }

                ts[2] = System.currentTimeMillis();

                // Skip the connection if we are doing HW MANU
                if((devId == WolfSSL.INVALID_DEVID)||((connectCount % 3) != 2)) {
                    appendDisplayText("C. Setting up context and socket\n");

                    SSLContext ctx = SSLContext.getInstance("TLS", wolfProv.getName());
                    /* not setting up KeyStore or TrustStore, wolfJSSE will load
                     * CA certs from the Android system KeyStore by default. */
                    ctx.init(km, tm, null);

                    SSLSocketFactory sf = ctx.getSocketFactory();
                    SSLSocket sock = (SSLSocket) sf.createSocket(host, port);

                    if (limitCipherProtocol) {
                        /* Limit cipherSuites and protocol */
                        String[] cipherSuites = new String[]{"TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA"};
                        sock.setEnabledCipherSuites(cipherSuites);
                        String[] protocols = new String[]{"TLSv1.2"};
                        sock.setEnabledProtocols(protocols);
                    }

                    ts[3] = System.currentTimeMillis();
                    appendDisplayText("D. Starting Handshake\n");
                    sock.startHandshake();

                    ts[4] = System.currentTimeMillis();
                    appendDisplayText("E. Sending GET Request\n");
                    sock.getOutputStream().write(getCmd.getBytes());

                    ts[5] = System.currentTimeMillis();
                    long sizeLimit = 10 * 1024;
                    appendDisplayText("F. Receiving at least " + sizeLimit + " bytes\n");
                    byte[] data = new byte[4 * 1024];
                    ByteArrayOutputStream buffer = new ByteArrayOutputStream();

                    /* Read the first 10kB */
                    while (totalRead < sizeLimit) {
                        int nRead = sock.getInputStream().read(data, 0, data.length);
                        if (nRead < 0) break;
                        buffer.write(data, 0, nRead);
                        totalRead = totalRead + nRead;
                    }

                    ts[6] = System.currentTimeMillis();
                    appendDisplayText("G. Closing socket\n");
                    sock.close();

                    ts[7] = System.currentTimeMillis();
                }
                if(devId != WolfSSL.INVALID_DEVID) {
                    switch(connectCount % 3) {
                        case 1:  //SW
                        {
                            appendDisplayText("H. Software crypto still enabled\n");
                        };break;
                        case 0:  //HW -App
                        case 2:  //HW - Manu
                        {
                            appendDisplayText("H. Disabling hardware offload\n");
                            wolfProv.setDevId(WolfSSL.INVALID_DEVID);
                            wolfProv.unRegisterDevId(devId);
                        };break;
                    }
                } else {
                    appendDisplayText("H. Internal crypto still enabled\n");
                }

                ts[8] = System.currentTimeMillis();
                appendDisplayText("I. Removing Provider\n");
                Security.removeProvider(wolfProv.getName());

                ts[9] = System.currentTimeMillis();

                appendDisplayText("TLS Connection Successful! Got" +
                        totalRead + " bytes.\n");
                appendDisplayText("Times(ms):" +
                        " A:" + (ts[1] - ts[0]) +
                        " B:" + (ts[2] - ts[1]) +
                        " C:" + (ts[3] - ts[2]) +
                        " D:" + (ts[4] - ts[3]) +
                        " E:" + (ts[5] - ts[4]) +
                        ", " + getCmd.length() * 1000 / (ts[5] - ts[4] + 1) + "Bps" +
                        " F:" + (ts[6] - ts[5]) +
                        ", " + totalRead * 1000 / (ts[6] - ts[5] + 1) + "Bps" +
                        " G:" + (ts[7] - ts[6]) +
                        " H:" + (ts[8] - ts[7]) +
                        " I:" + (ts[9] - ts[8]) +
                        "\n");

            } catch (Exception e) {
                appendDisplayText("TLS Connection Failed");
                e.printStackTrace();
            }
        });
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        setDisplayText("Please make a selection below");

        Button wcTestButton = (Button) findViewById(R.id.btn_wc_test);
        wcTestButton.setOnClickListener(wcTestListener);

        Button wcBenchButton = (Button) findViewById(R.id.btn_wc_bench);
        wcBenchButton.setOnClickListener(wcBenchmarkListener);

        Button sslSocketConnectButton = (Button) findViewById(R.id.btn_sslsocket_connect);
        sslSocketConnectButton.setOnClickListener(sslSocketConnectListener);
    }

    static {
        System.loadLibrary("wolfssl");
        System.loadLibrary("wolfssljni");
        System.loadLibrary("jnisample");
    }
}
