package com.wolfssl.wolfssljni_ndk_gradle;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

import com.wolfssl.WolfSSL;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText("wolfSSL JNI Android Studio Example App");

        /* calling an API here to make sure library is linked */
        try {
            WolfSSL wolflib = new WolfSSL();
            tv.append("\r\n... initialized WolfSSL object");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    static {
        System.loadLibrary("wolfssl");
        System.loadLibrary("wolfssljni");
    }
}
