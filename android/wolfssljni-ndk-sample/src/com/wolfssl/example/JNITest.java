/* JNITest.java
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */
package com.wolfssl.example;

import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;

import com.wolfssl.WolfSSL;

public class JNITest extends Activity
{
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        /* create TextView and set contents */
        TextView tv = new TextView(this);
        String jniString = getJniString();
        tv.setText(jniString);
        setContentView(tv);

        try {

            /* initialize wolfSSL library, using JNI wrapper */
            WolfSSL sslLib = new WolfSSL();

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public native String getJniString();

    static {
        /* these must be loaded in the correct order of dependency */
        System.loadLibrary("wolfssl");
        System.loadLibrary("wolfssljni");
        System.loadLibrary("jnitest");
    }
}
