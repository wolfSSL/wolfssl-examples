package com.wolfssl.wolfssljni_ndk_gradle;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.wolfssl.WolfSSL;
import com.wolfssl.provider.jsse.WolfSSLProvider;

import java.io.ByteArrayOutputStream;
import java.security.Security;
import java.util.concurrent.Executors;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

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

    private final View.OnClickListener sslSocketConnectListener = v -> {
        setDisplayText("Making simple SSL/TLS connection to:\n" +
                host + ":" + port + "\n" +
                "See logcat output for details (adb logcat).\n" +
                "...\n");
        Executors.newSingleThreadExecutor().execute(() -> {
            try {
                /* Enable wolfJSSE debug messages */
                System.setProperty("wolfjsse.debug", "true");

                /* Select The devId which will be registered and used on every other invocation */
                /* Unique devId's are compiled into wolfssljni */
                //int devId = 0x56490420;  /* VaultIC 420 */
                int devId = WolfSSL.INVALID_DEVID;  /* No hardware offload */

                /* Select if the cipher suites and protocols will be limited */
                boolean limitCipherProtocol = false;

                long[] ts = new long[10];

                ts[0] = System.currentTimeMillis();
                WolfSSLProvider wolfProv = new WolfSSLProvider();
                appendDisplayText("A. Inserting Provider " + wolfProv.getName() + "\n");
                Security.insertProviderAt(wolfProv, 1);

                ts[1] = System.currentTimeMillis();
                connectCount++;

                if (((connectCount % 2) == 0) && (devId != WolfSSL.INVALID_DEVID)) {
                    appendDisplayText("B. Using hardware offload\n");
                    wolfProv.registerDevId(devId);
                    wolfProv.setDevId(devId);
                } else {
                    appendDisplayText("B. Using software crypto.\n");
                    wolfProv.setDevId(WolfSSL.INVALID_DEVID);
                }

                ts[2] = System.currentTimeMillis();
                appendDisplayText("C. Setting up context and socket\n");

                /* not setting up KeyStore or TrustStore, wolfJSSE will load
                 * CA certs from the Android system KeyStore by default. */
                SSLContext ctx = SSLContext.getInstance("TLS", wolfProv.getName());
                ctx.init(null, null, null);

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
                String getCmd = "GET / HTTP/1.1\r\n" +
                                "Host: www.wolfssl.com\r\n" +
                                "Accept: */*\r\n\r\n";
                sock.getOutputStream().write(getCmd.getBytes());

                ts[5] = System.currentTimeMillis();
                long sizeLimit = 10 * 1024;
                appendDisplayText("F. Receiving at least " + sizeLimit + " bytes\n");
                byte[] data = new byte[4 * 1024];
                ByteArrayOutputStream buffer = new ByteArrayOutputStream();
                long totalRead = 0;
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
                if (((connectCount % 2) == 0)  && (devId != WolfSSL.INVALID_DEVID)) {
                    appendDisplayText("H. Disabling hardware offload\n");
                    wolfProv.setDevId(WolfSSL.INVALID_DEVID);
                    wolfProv.unRegisterDevId(devId);
                } else {
                    appendDisplayText("H. Software crypto still enabled\n");
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
