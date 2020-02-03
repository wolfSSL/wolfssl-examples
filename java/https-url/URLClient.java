/* URLClient.java
 *
 * Copyright (C) 2006-2020 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
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

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.InetSocketAddress;
import java.net.MalformedURLException;
import java.net.ServerSocket;
import java.net.Socket;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLEngine;
import javax.net.ssl.SSLPeerUnverifiedException;
import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;

import java.security.Provider;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.net.ServerSocketFactory;
import javax.net.SocketFactory;
import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.TrustManagerFactory;

import com.wolfssl.provider.jsse.WolfSSLDebug;
import com.wolfssl.provider.jsse.WolfSSLProvider;
import com.wolfssl.WolfSSL;

import java.security.PrivateKey;
import java.security.Security;
import javax.net.ssl.KeyManager;
import javax.net.ssl.X509KeyManager;
import java.security.cert.X509Certificate;
import java.security.cert.Certificate;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.HostnameVerifier;
import java.net.URL;

public class URLClient {
    public URLClient() {
    }

    public void run(String[] args) throws Exception {
        int ret = 0, input;
        byte[] back = new byte[80];
        String msg  = "Too legit to quit";
        String provider = "wolfJSSE";

        KeyStore pKey, cert;
        TrustManagerFactory tm = null;
        KeyManagerFactory km = null;
        ServerSocketFactory srv;
        SSLContext ctx;
        SSLSocketFactory sf;

        /* config info */
        String version;
        String cipherList = null;             /* default ciphersuite list */
        int sslVersion = 3;                   /* default to TLS 1.2 */
        boolean useEnvVar  = false;           /* load cert/key from enviornment variable */
        boolean listSuites = false;           /* list all supported cipher suites */
        boolean listEnabledProtocols = false;  /* show enabled protocols */
        boolean putEnabledProtocols  = false;  /* set enabled protocols */

        /* cert info */
        String clientJKS  = "../example-keystores/client.jks";
        String caJKS      = "../example-keystores/client.jks";
        String clientPswd = "wolfSSL test";
        String caPswd = "wolfSSL test";

        /* server (peer) info */
        String host = "localhost";
        int port    =  11111;

        /* set/get enabled protocols */
        String[] protocols = null;

        /* pull in command line options from user */
        for (int i = 0; i < args.length; i++)
        {
            String arg = args[i];

            if (arg.equals("-?")) {
                printUsage();

            } else if (arg.equals("-h")) {
                if (args.length < i+2)
                    printUsage();
                host = args[++i];

            } else if (arg.equals("-p")) {
                if (args.length < i+2)
                    printUsage();
                port = Integer.parseInt(args[++i]);

            } else if (arg.equals("-v")) {
                if (args.length < i+2)
                    printUsage();
                if (args[i+1].equals("d")) {
                    i++;
                    sslVersion = -1;
                }
                else {
                    sslVersion = Integer.parseInt(args[++i]);
                    if (sslVersion < 0 || sslVersion > 4) {
                        printUsage();
                    }
                }

            } else if (arg.equals("-l")) {
                if (args.length < i+2)
                    printUsage();
                cipherList = args[++i];

            } else if (arg.equals("-c")) {
                if (args.length < i+2) {
                    printUsage();
                }
                String[] tmp = args[++i].split(":");
                if (tmp.length != 2) {
                    printUsage();
                }
                clientJKS = tmp[0];
                clientPswd = tmp[1];

            } else if (arg.equals("-A")) {
                if (args.length < i+2)
                    printUsage();
                String[] tmp = args[++i].split(":");
                if (tmp.length != 2) {
                    printUsage();
                }
                caJKS = tmp[0];
                caPswd = tmp[1];

            } else if (arg.equals("-e")) {
                listSuites = true;

            } else if (arg.equals("-env")) {
                useEnvVar = true;

            } else if (arg.equals("-getp")) {
                listEnabledProtocols = true;

            } else if (arg.equals("-setp")) {
                putEnabledProtocols = true;
                protocols = args[++i].split(" ");
                sslVersion = -1;

            } else {
                printUsage();
            }
        }

        switch (sslVersion) {
            case -1: version = "TLS"; break;
            case 0:  version = "SSLv3"; break;
            case 1:  version = "TLSv1"; break;
            case 2:  version = "TLSv1.1"; break;
            case 3:  version = "TLSv1.2"; break;
            case 4:  version = "TLSv1.3"; break;
            default:
                printUsage();
                return;
        }

        /* trust manager (certificates) */
        cert = KeyStore.getInstance("JKS");
        cert.load(new FileInputStream(caJKS), caPswd.toCharArray());
        tm = TrustManagerFactory.getInstance("SunX509", provider);
        tm.init(cert);

        /* load private key */
        pKey = KeyStore.getInstance("JKS");
        pKey.load(new FileInputStream(clientJKS), clientPswd.toCharArray());
        km = KeyManagerFactory.getInstance("SunX509", provider);
        km.init(pKey, clientPswd.toCharArray());

        /* setup context with certificate and private key */
        ctx = SSLContext.getInstance(version, provider);
        ctx.init(km.getKeyManagers(), tm.getTrustManagers(), null);

        if (listSuites) {
            String[] suites = ctx.getDefaultSSLParameters().getCipherSuites();
            for (String x : suites) {
                System.out.println("\t" + x);
            }
            return;
        }

        /* set system properties for protocol and cipher suite list */
        if (cipherList != null) {
        		System.setProperty("https.cipherSuites", cipherList);
        }
        System.setProperty("https.protocols", version);
        
        System.out.println("Creating socket factory using provider " + ctx.getProvider());
        sf = ctx.getSocketFactory();
        /* print enabled protocols if requested */
        if (listEnabledProtocols) {
            SSLSocket sk = (SSLSocket)sf.createSocket();
            String[] prtolists = sk.getEnabledProtocols();
            for (String str : prtolists) {
                System.out.println("\t" + str);
            }
            return;
        }
        
        URL url;
        HttpsURLConnection con;
        
        /* HTTPS connection */
		try {
			url = new URL(host);
			con = (HttpsURLConnection)url.openConnection();
			con.setSSLSocketFactory(sf);
			con.setRequestMethod("GET");
			ret = con.getResponseCode();
			if (ret == HttpURLConnection.HTTP_OK) {
				showPeer(con);
				
				System.out.println("\nReturn from GET =\n" + con.getResponseMessage());
			}
			else {
				System.out.println("Unable to connect to " + host + " GET response was " + ret);
			}
		} catch (MalformedURLException e) {
	        System.out.println("Error creating URL");
			e.printStackTrace();
		} catch (IOException e) {
	        System.out.println("Error opening connection to " + host);
			e.printStackTrace();
		}

    }


    private void showPeer(HttpsURLConnection con) throws SSLPeerUnverifiedException {
    		System.out.println("Connected to URL " + con.getURL());
    		System.out.println("Using cipher suite " + con.getCipherSuite());
    		System.out.println("Last modified " + con.getIfModifiedSince());
    		Certificate[] certs = con.getServerCertificates();
    		System.out.println("Peer certificate : " + certs[0].toString());
    		
    		System.out.println("Peer principal name : " + con.getPeerPrincipal().getName());
    }

    public static void main(String[] args) {
        WolfSSL.loadLibrary();
        Security.addProvider(new WolfSSLProvider());

        URLClient client = new URLClient();
        try {
            client.run(args);
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    private void printUsage() {
        System.out.println("Java wolfJSSE example client usage:");
        System.out.println("-?\t\tHelp, print this usage");
        System.out.println("-h <host>\tHost to connect to, default 127.0.0.1");
        System.out.println("-p <num>\tPort to connect to, default 11111");
        System.out.println("-v <num>\tSSL version [0-4], SSLv3(0) - " +
                           "TLS1.3(4)), default 3 : use 'd' for downgrade");
        System.out.println("-l <str>\tComma seperated cipher list");
        System.out.println("-e\t\tGet all supported cipher suites");
        System.out.println("-getp\t\tGet enabled protocols");
        System.out.println("-setp <protocols> \tSet enabled protocols " +
                           "e.g \"TLSv1.1 TLSv1.2\"");
        System.out.println("-c <file>:<password>\tCertificate/key JKS,\t\tdefault " +
                "../provider/rsa.jks:wolfSSL test");
        System.out.println("-A <file>:<password>\tCertificate/key CA JKS file,\tdefault " +
                "../provider/cacerts.jks:wolfSSL test");
        System.exit(1);
    }
}

