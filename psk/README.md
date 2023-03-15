This folder contains examples related to PSK, including:
* Quick-Start section to simply build and run some of the examples
* Tutorial section detailing the process of adding PSK support to a
  client/server system.

# Quick Start
To build and run the basic PSK example:

In the wolfSSL directory:
```
$ ./configure --enable-psk --enable-opensslextra CFLAGS="-DWOLFSSL_STATIC_PSK"
$ make && make install
```
In the psk directory:
```
$ make
$ ./server-psk
$ ./client-psk
```

# TCP/PSK Tutorial

## **Tutorial for adding wolfSSL Security to a Simple Client.**

1. Include the wolfSSL compatibility header:
``#include <wolfssl/ssl.h>``
2. Change all calls from read() or recv() to wolfSSL_read(), in the simple client

    ``read(sockfd, recvline, MAXLINE)`` becomes ``wolfSSL_read(ssl, recvline, MAXLINE)``

3. Change all calls from write() or send() to wolfSSL_write(), in the simple client

    ``write(socked, sendline, strlen(sendline))`` becomes ``wolfSSL_write(ssl, sendline, strlen(sendline))``

4. In the main method initialize wolfSSL and WOLFSSL_CTX. You must initialize wolfSSL before making any other wolfSSL calls.
   wolfSSL_CTX_new() takes an argument that defines what SSL/TLS protocol to use. In this case ``wolfTLSv1_2_client_method()`` is used to specify TLS 1.2.

        wolfSSL_Init();

        if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL)
            fprintf(stderr, "SSL_CTX_new error.\n");
            return 1;
        }

5. Create the wolfSSL object after each TCP connect and associate the file descriptor with the session:

        if ((ssl = wolfSSL_new(ctx)) == NULL) {
            fprintf(stderr, "wolfSSL_new error.\n");
            return 1;
        }

        ret = wolfSSL_set_fd(ssl, sockfd);

        if (ret != SSL_SUCCESS){
            return 1;
        }

6. Cleanup. After each wolfSSL object is done being used you can free it up by calling ``wolfSSL_free(ssl);``.

7. When completely done using SSL/TLS, free the WOLFSSL_CTX object by

    ``wolfSSL_CTX_free(ctx);``

    ``wolfSSL_Cleanup();``

## **Adding Pre-Shared Keys (PSK) to the wolfSSL Simple Client.**

1. When configuring wolfSSL

    ``./configure --enable-psk CFLAGS="-DWOLFSSL_STATIC_PSK"``

    ``make``

    ``sudo make install``

2. In the main method add

    ``wolfSSL_CTX_set_psk_client_callback(ctx, 	My_Psk_Client_cb);``

3. Add the function

		static inline unsigned int My_Psk_Client_Cb(WOLFSSL* ssl, const char* hint,
            char* identity, unsigned int id_max_len, unsigned char* key,
            unsigned int key_max_len)
    		{
        		(void)ssl;
        		(void)hint;
        		(void)key_max_len;

        		strncpy(identity, "Client_identity", id_max_len);

        		key[0] = 26;
        		key[1] = 43;
       			key[2] = 60;
        		key[3] = 77;

        		return 4;
        	}

## **Adding Non-Blocking to the wolfSSL Simple Client.**

1. Include the fcntl.h header file. This is needed for some of the constants that will be used when dealing with non-blocking on the socket. `` #include <fcntl.h>``

2. NOTE: TLS is nonblocking by default, just make sure the sockets are configured to be non-blocking sockets

3. Now we must invoke the fcntl callable serve to use non-blocking.

        int flags = fcntl(sockfd, F_GETFL, 0);
        if (flags < 0) {
            printf("fcntl get failed\n");
            return 1;
        }

        flags = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        if (flags < 0) {
            printf("fcntl set failed\n");
            return 1;
        }
4. Now we are ready to call the function to start non-blocking on our socket. This is done by adding:

        ret = NonBlockingSSL_Connect(ssl);

        if (ret != 0) {
    	    return 1;
        }

5. Now we need to add the non-blocking function.

     **Add the tcp_select function**

            enum {
            TEST_SELECT_FAIL,
            TEST_TIMEOUT,
            TEST_RECV_READY,
            TEST_ERROR_READY
            };


            static inline int tcp_select(int socketfd, int to_sec)
            {
                fd_set recvfds, errfds;
                int nfds = socketfd + 1;
                struct timeval timeout = { (to_sec > 0) ? to_sec : 0, 0};

                FD_ZERO(&recvfds);
                FD_SET(socketfd, &recvfds);
                FD_ZERO(&errfds);
                FD_SET(socketfd, &errfds);

                int result = select(nfds, &recvfds, NULL, &errfds, &timeout);

                if (result == 0)
                    return TEST_TIMEOUT;
                else if (result > 0) {
                    if (FD_ISSET(socketfd, &recvfds))
                        return TEST_RECV_READY;
                    else if(FD_ISSET(socketfd, &errfds))
                        return TEST_ERROR_READY;
                }

                return TEST_SELECT_FAIL;
            }

    **Add the non-blocking function**

        static int NonBlockingSSL_Connect(WOLFSSL* ssl)
        {
            int ret, error, sockfd, select_ret, currTimeout;

            ret    = wolfSSL_connect(ssl);
            error  = wolfSSL_get_error(ssl, 0);
            sockfd = (int)wolfSSL_get_fd(ssl);

            while (ret != SSL_SUCCESS && (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)) {
                currTimeout = 1;

                if (error == SSL_ERROR_WANT_READ)
                    printf("... client would read block\n");
                else
                    printf("... client would write block\n");

                select_ret = tcp_select(sockfd, currTimeout);

                if ((select_ret == TEST_RECV_READY) ||
                    (select_ret == TEST_ERROR_READY)) {
                    ret   = wolfSSL_connect(ssl);
                    error = wolfSSL_get_error(ssl, 0);
                }
                else if (select_ret == TEST_TIMEOUT) {
                    error = SSL_ERROR_WANT_READ;
                }
                else {
                    error = SSL_FATAL_ERROR;
                }
            }
            if (ret != SSL_SUCCESS){
                printf("SSL_connect failed");
                return 1;
            }

	        return 0;
        }

## **Adding Session Resumption to Simple Client**

Session resumption allows a client/server pair to re-use previously generated crypto material, so that they don't have to compute new crypto keys every time a connection gets established.

1. After sending a string to the server we need to save the session ID so it can be used again for the next connection.

        /* Save the session ID to reuse */
        WOLFSSL_SESSION* session   = wolfSSL_get_session(ssl);
        WOLFSSL*         sslResume = wolfSSL_new(ctx);

2. Now we must close wolfSSL SSL and close connections i.e. free the socket and ctx.

        /* shut down wolfSSL */
        wolfSSL_shutdown(ssl);

        /* close connection */
        close(sockfd);

        /* cleanup without wolfSSL_Cleanup() for now */
        wolfSSL_free(ssl);
        wolfSSL_CTX_free(ctx);

3. Now we are ready to reconnect and start a new socket but we are going to reuse the session id to make things go a little faster.


        sock = socket(AF_INET, SOCK_STREAM, 0);

        /* connect to the socket */
        ret = connect(sock, (struct sockaddr *) &servaddr, sizeof(servaddr));

        if (ret != 0){
            return 1;
        }

        /* set the session ID to connect to the server */
        wolfSSL_set_fd(sslResume, sock);
        wolfSSL_set_session(sslResume, session);

4. Check if the connect was successful.

        /* check has connect successfully */
        if (wolfSSL_connect(sslResume) != SSL_SUCCESS) {
            printf("SSL resume failed\n");
            return 1;
        }

5. If successful we can write to the server again.

6. Check to see if the session id was actually reused or if it was just a new session.

        /* check to see if the session id is being reused */
        if (wolfSSL_session_reused(sslResume))
            printf("reused session id\n");
        else
            printf("didn't reuse session id!!!\n");

7. Now close the ssl and socket.

        /* shut down wolfSSL */
        wolfSSL_shutdown(sslResume);

        /* shut down socket */
        close(sock);

        /* clean up now with wolfSSL_Cleanup() */
        wolfSSL_free(sslResume);
        wolfSSL_CTX_free(ctx);
        wolfSSL_Cleanup();


## **Tutorial for adding wolfSSL Security and PSK (Pre shared Keys) to a Simple Server.**

1. Include the wolfSSL compatibility header:
    ``#include <wolfssl/ssl.h>``

2. Change all calls from read() or recv() to wolfSSL_read(), in the simple server
    ``read(sockfd, recvline, MAXLINE)`` becomes ``wolfSSL_read(ssl, recvline, MAXLINE)``

	>(wolfSSL_read on first use also calls wolfSSL_accept if not explicitly called earlier in code.)

3. Change all calls from write() or send() to wolfSSL_write(), in the simple server
    ``write(sockfd, sendline, strlen(sendline))`` becomes ``wolfSSL_write(ssl, sendline, strlen(sendline))``

4. Run the wolfSSL method to initialize wolfSSL
    ``wolfSSL_Init()``

5. Create a ctx pointer that contains a server method using the following process. The server method wolfSSLv23_server_method()
   allows clients with TLS 1+ to connect.
    ```
    WOLFSSL_CTX* ctx;

    if ((ctx = wolfSSL_CTX_new(wolfSSLv23_server_method())) == NULL)
	    err_sys(“wolfSSL_CTX_new error”);
    ```

6. In the servers main loop for accepting clients create a WOLFSSL pointer. Once a new client is accepted create a wolfSSL object and associate that object with the socket that the client is on. After using the wolfSSL object it should be freed and also before closing the program the ctx pointer should be freed and a wolfSSL cleanup method called.

	```
    WOLFSSL* ssl;

	wolfSSL_set_fd(ssl, “integer (file descriptor) returned from accept”);

	wolfSSL_free(ssl);

	wolfSSL_CTX_free(ctx);
	wolfSSL_Cleanup();
    ```


## Now adding Pre-Shared Keys (PSK) to the wolfSSL Simple Server:

The following steps are on how to use PSK in a wolfSSL server

1. Build wolfSSL with pre shared keys enabled executing the following commands in wolfSSL’s root directory. Depending on file locations sudo may be needed when running the commands.
    ```
    ./configure --enable-psk
    make
    make install
    ```

2. Set up the psk suit with using the wolfSSL callback, identity hint, and cipher list methods. These methods get called immediately after the process of setting up ctx.
    ```
    wolfSSL_CTX_set_psk_server_callback(ctx, my_psk_server_cb);
    wolfSSL_CTX_use_psk_identity_hint(ctx, “wolfssl server”);
    wolfSSL_CTX_set_cipher_list(ctx, “PSK-AES128-CBC-SHA256”);
    ```

	>PSK-AES128-CBC-SHA256 creates the cipher list of having pre shared keys with advanced encryption security using 128 bit key
	>with cipher block chaining using secure hash algorithm.

3. Add the my_psk_server_cb function as follows. This is a necessary function that is passed in as an argument to the wolfSSL callback.

    ```
    static inline unsigned int my_psk_client_cb(WOLFSSL* ssl, char* identity, unsigned
                                                char* key, unsigned int key_max_len) {
    		(void)ssl;
    		(void)key_max_len;

    		if (strncmp(identity, "Client_identity", 15) != 0)
			return 0;

    		/* test key n hex is 0x1a2b3c4d , in decimal 439,041,101, we're using
     		* unsigned binary */
    		key[0] = 26;
    		key[1] = 43;
    		key[2] = 60;
    		key[3] = 77;

    		return 4;
    }
    ```


Example Makefile for Simple wolfSSL PSK Client:

    ```
	CC=gcc
	OBJ = client-psk.o
	CFLAG=-Wall

    %.o: %.c $(DEPS)
        $(CC) -c -o $@ $< $(CFLAGS)

	client-psk: client-psk.c
        $(CC) -Wall -o client-psk client-psk.c -lwolfssl

    .PHONY: clean

	clean:
		rm -f *.o client-psk
    ```

The -lwolfssl will link the wolfSSL Libraries to your program


The makefile for the server is going to be similar to that of the client. If the user wants separate makefiles just make and use the same set up of the client makefile and replace every instance of client-psk with server-psk. To combine make files just add a server-psk with similar ending to each time client-psk is referenced and change the target. There will also need to be a target for when compiling all targets.

    ```
	all: server-psk client-psk

	server-psk: server-psk.c
		$(CC) -Wall -o server-psk server-psk.c -lwolfssl
    ```

## Nonblocking psk
###### What is nonblocking?
When a socket is setup as non-blocking, reads and writes to the socket do not cause the application to block and wait. Instead the read or write function will read/write only the data currently available (if any). If the entire read/write is not completed, a status indicator is returned. The application may retry the read/write later.

##Tutorial for adding nonblocking to a Client.
1. Include the fcntl.h header file. This is needed for some of the constants that will be used when dealing with non blocking on the socket.

    ``#include <fcntl.h>``

2. NOTE: TLS is nonblocking by default, just make sure the sockets are configured to be non-blocking sockets

3. Now we much invoke the fcntl callable serve to use nonblocking. This is done by adding:

    ```
    /* invokes the fcntl callable service to get the file status
     * flags for a file. checks if it returns an error, if it does
     * stop program */
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0){
    printf("fcntl get failed\n");
        exit(0);
    }

    /* invokes the fcntl callable service set file status flags.
     * Do not block an open, a read, or a write on the file
     * (do not wait for terminal input. If an error occurs,
    * stop program*/
    flags = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    if (flags < 0){
        printf("fcntl set failed\n");
        exit(0);
    }
    ```

4. Now we are ready to call the function to start nonblocking on our socket. This is done by adding:

    ```
    /* setting up and running nonblocking socket */
    NonBlockingSSL_Connect(ssl);
    ```





5. Before adding the NonblockingSSL_Connect function into our code we must add a tcp_select function that will be used by the NonblockingSSL_Connect. This is done by adding:

    ```
    /*
     * enum used for tcp_select function
     */
    enum {
        TEST_SELECT_FAIL,
        TEST_TIMEOUT,
        TEST_RECV_READY,
        TEST_ERROR_READY
    };

    static inline int tcp_select(int socketfd, int to_sec)
    {
        fd_set recvfds, errfds;
        int nfds = socketfd + 1;
        struct timeval timeout = { (to_sec > 0) ? to_sec : 0, 0};
        int result;
    
        FD_ZERO(&recvfds);
        FD_SET(socketfd, &recvfds);
        FD_ZERO(&errfds);
        FD_SET(socketfd, &errfds);
    
        result = select(nfds, &recvfds, NULL, &errfds, &timeout);
    
        if (result == 0)
            return TEST_TIMEOUT;
        else if (result > 0) {
            if (FD_ISSET(socketfd, &recvfds))
                return TEST_RECV_READY;
            else if(FD_ISSET(socketfd, &errfds))
                return TEST_ERROR_READY;
        }
        return TEST_SELECT_FAIL;
        }
    ```




6. Now we can add the NonBlockingSSL_Connect function. This can be done by adding:

    ```
    static void NonBlockingSSL_Connect(WOLFSSL* ssl){

    int ret = wolfSSL_connect(ssl);

    int error = wolfSSL_get_error(ssl, 0);
    int sockfd = (int)wolfSSL_get_fd(ssl);
	int select_ret;

	while (ret != SSL_SUCCESS && (error == SSL_ERROR_WANT_READ ||
	                           error == SSL_ERROR_WANT_WRITE)) {
		int currTimeout = 1;

		if (error == SSL_ERROR_WANT_READ)
			printf("... client would read block\n");
		else {
			printf("... client would write block\n");

	    	select_ret = tcp_select(sockfd, currTimeout);

		    	if ((select_ret == TEST_RECV_READY) ||
				(select_ret == TEST_ERROR_READY)) {
				ret = wolfSSL_connect(ssl);
				error = wolfSSL_get_error(ssl, 0);
			}
			else if (select_ret == TEST_TIMEOUT && !wolfSSL_dtls(ssl)) {
				error = SSL_ERROR_WANT_READ;
			}
			else {
				error = SSL_FATAL_ERROR;
			}
		}
		if (ret != SSL_SUCCESS){
			printf("SSL_connect failed");
			exit(0);
		}
 	}
    ```
## Tutorial for adding nonblocking to a Server.

Nonblocking on the server side allows for switching between multiple client connections when reading and writing without closing them.

1. Include the fcntl.h header file. This is needed for some of the constants that will be used when dealing with non blocking on the socket.
    ``#include <fcntl.h>``

2. NOTE: TLS is nonblocking by default, just make sure the sockets are configured to be non-blocking sockets

3. Immediately after setting wolfSSL to use non blocking, the socket that the client is connected on needs to also be set up to be non blocking. This is done using the included fcntl.h and making the following function call.

    ``fcntl(*sockfd, F_SETFL, O_NONBLOCK);``

	>Both F_SETFL and O_NONBLOCK are constants from the fcntl.h file.

4. Include a function to select tcp. What this function does is it checks file descriptors for readiness of reading, writing, for pending exceptions, and for timeout. The timeout variable needs to point to struct timeval type. If the timeval members are 0 then the function does not block. The function and its input parameters are listed below.

    ``select(int nfds, fd_set* read, fd_set* write, fd_set* exception, struct timeval* time)``

	>For the example server we do not consider write when selecting the tcp so it is set to NULL. For ease the example code uses enumerated values for which state the function select returns. This then makes the next loop discussed easier.

5. Next is to add a loop for handling when to read and write. This loop uses the select tcp function to continually check on the status of the tcp connection and when it is ready or has an exception the wolfSSL_accept function is called.

6. The final thing added is a loop around wolfSSL_read. This is done so that when encountering the error SSL_ERROR_WANT_READ the server gives the client some time to send the message.

    ```
	/* timed loop to continue checking for a client message */
	do {
     	if (n < 0) {
         	err = wolfSSL_get_error(ssl, 0);
         	if (err != SSL_ERROR_WANT_READ)
             	err_sys("respond: read error");
         	n = wolfSSL_read(ssl, buf, MAXLINE);
         	time(&current_time);
     	}
 	} while (err == SSL_ERROR_WANT_READ && n < 0 &&
          	difftime(current_time, start_time) < seconds);
    ```




## Concurrent Server


The main thread accepts clients and for each client accepted a new thread is spawned that then handles the typical server processes.


1. To use multiple threads include the pthread header file.
    ``#include <pthread.h>``


2. When creating multiple threads the state of variables can become an issue. Since in the example, WOLFSSL_CTX* is not changed after being initially set we can make it a global variable and allow all threads read access while they are processing without having to lock the memory.


3. After the main thread accepts a client, call the pthread_create function.
    ``pthread_create(pthread_t* thread, int attribute, void* function, void* arg)``

4. In the example the function passed to pthread_create accepts one void * argument which is the socket the client is on. The function then performs the process of creating a new SSL object, reading and writing to the client, freeing the SSL object, and then terminating the thread.
    ```
    /*
    *Process handled by a thread.
    */
    void* wolfssl_thread(void* fd)
    {
          WOLFSSL* ssl;
          int connfd = (int)fd;
          int  n;              /* length of string read */
          char buf[MAXLINE];   /* string read from client */
          char response[22] = "I hear ya for shizzle";

          /* create WOLFSSL object and respond */
          if ((ssl = wolfSSL_new(ctx)) == NULL)
              err_sys("wolfSSL_new error");
          wolfSSL_set_fd(ssl, connfd);

          /* respond to client */
          n = wolfSSL_read(ssl, buf, MAXLINE);
          if (n > 0) {
              printf("%s\n", buf);
              if (wolfSSL_write(ssl, response, 22) > 22) {
                  err_sys("respond: write error");
              }
          }
          if (n < 0) {
              err_sys("respond: read error");
          }

          /* closes the connections after responding */
          wolfSSL_shutdown(ssl);
         wolfSSL_free(ssl);
         if (close(connfd) == -1)
             err_sys("close error");
         pthread_exit( NULL);
    }
    ```

5. Void* arg is the argument that gets passed into wolfssl_thread when pthread_create is called. In this example that argument is used to pass the socket value that the client for the current thread is on.
