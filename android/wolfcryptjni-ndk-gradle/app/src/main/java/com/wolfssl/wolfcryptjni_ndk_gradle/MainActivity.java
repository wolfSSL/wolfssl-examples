package com.wolfssl.wolfcryptjni_ndk_gradle;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

import java.util.*;

import com.wolfssl.wolfcrypt.*;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText("wolfCrypt JNI Android Studio Example App");

        /* calling an API here to make sure library is linked */
        try {

            tv.append("\r\nStarting SHA Example\r\n");

            String[] dataVector = new String[] {
                    "",
                    "226833eca43edeab",
                    "01ae37df5128cb6059b57a904e834ca9",
                    "421b944a38f03450b21d1c8c6514461fb82ef846cc9eebe7",
                    "9f1daf4748d7aa20a359a7d8a220446de1a918e6dad68bda5894eb312ebbbc2e" };

            String[] hashVector = new String[] {
                    "da39a3ee5e6b4b0d3255bfef95601890afd80709",
                    "69e8fb462869452f0387733b03045dc0835531e5",
                    "6849e5d39ac08f5daec25b91c4f4160cd921f8b7",
                    "7e328a4e252a2d901a7f79365953a5e0682a8a9d",
                    "22bbfc22a78aef3e356a32066eee78fc2ce28d8c" };

            for (int i = 0; i < dataVector.length; i++) {
                Sha sha = new Sha();

                byte[] data = Util.h2b(dataVector[i]);
                byte[] expected = Util.h2b(hashVector[i]);

                sha.update(data);
                byte[] result = sha.digest();

                if (Arrays.equals(expected, result) == true) {
                    tv.append("... test #" + i + ": PASSED\r\n");
                } else {
                    tv.append("... test #" + i + ": FAILED\r\n");
                }
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    static {
        System.loadLibrary("wolfssl");
        System.loadLibrary("wolfcryptjni");
    }
}

class Util {
    public static byte[] h2b(String s) {
        int len = s.length();
        byte[] data = new byte[len / 2];

        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4) + Character
                    .digit(s.charAt(i + 1), 16));
        }

        return data;
    }

    final protected static char[] hexArray = "0123456789ABCDEF".toCharArray();

    public static String b2h(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];

        for (int j = 0; j < bytes.length; j++) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = hexArray[v >>> 4];
            hexChars[j * 2 + 1] = hexArray[v & 0x0F];
        }

        return new String(hexChars);
    }
}

