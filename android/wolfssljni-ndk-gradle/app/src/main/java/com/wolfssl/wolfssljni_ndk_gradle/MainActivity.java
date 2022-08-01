package com.wolfssl.wolfssljni_ndk_gradle;

import android.os.AsyncTask;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.wolfssl.provider.jsse.WolfSSLProvider;

import java.security.Security;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

public class MainActivity extends AppCompatActivity {

    public native int testWolfCrypt();
    public native int benchWolfCrypt();

    private static String host = "www.wolfssl.com";
    private static int port = 443;

    private View.OnClickListener wcTestListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            TextView tv = (TextView) findViewById(R.id.displayText);
            tv.setText("Running wolfCrypt tests.\n" +
                       "See logcat output for details (adb logcat).\n" +
                       "...\n");

            AsyncTask.execute(new Runnable() {
                @Override
                public void run() {
                    int ret = testWolfCrypt();
                    if (ret == 0) {
                        tv.append("TESTS PASSED\n");
                    } else {
                        tv.append("TESTS FAILED!\nInspect logcat.");
                    }
                    tv.append("\n");
                    tv.append("To adjust wolfSSL configuration, please edit\n" +
                            "add_definitions() in CMakeLists.txt.");
                }
            });

        }
    };

    private View.OnClickListener wcBenchmarkListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            TextView tv = (TextView) findViewById(R.id.displayText);
            tv.setText("Running wolfCrypt benchmark.\n" +
                    "See logcat output for details (adb logcat).\n" +
                    "...\n");

            AsyncTask.execute(new Runnable() {
                @Override
                public void run() {
                    int ret = benchWolfCrypt();
                    if (ret == 0) {
                        tv.append("BENCHMARK PASSED\n");
                    } else {
                        tv.append("BENCHMARK FAILED!\nInspect logcat.");
                    }
                    tv.append("\n");
                    tv.append("To adjust wolfSSL configuration, please edit\n" +
                              "add_definitions() in CMakeLists.txt.");
                }
            });

        }
    };

    private View.OnClickListener sslSocketConnectListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            TextView tv = (TextView) findViewById(R.id.displayText);
            tv.setText("Making simple SSL/TLS connection to:\n" +
                       "www.wolfssl.com:443\n\n" +
                       "See logcat output for details (adb logcat).\n" +
                       "...\n");

            AsyncTask.execute(new Runnable() {
                @Override
                public void run() {

                    try {
                        /* Enable wolfJSSE debug messages */
                        System.setProperty("wolfjsse.debug", "true");

                        Security.insertProviderAt(new WolfSSLProvider(), 1);

                        /* not setting up KeyStore or TrustStore, wolfJSSE will load
                         * CA certs from the Android system KeyStore by default. */
                        SSLContext ctx = SSLContext.getInstance("TLS");
                        ctx.init(null, null, null);

                        SSLSocketFactory sf = ctx.getSocketFactory();
                        SSLSocket sock = (SSLSocket)sf.createSocket(host, port);

                        sock.startHandshake();

                        sock.close();

                        tv.append("TLS Connection Successful!");

                    } catch (Exception e) {
                        tv.append("TLS Connection Failed");
                        e.printStackTrace();
                    }
                }
            });

        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView tv = (TextView) findViewById(R.id.displayText);
        tv.setText("Please make a selection below");

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
