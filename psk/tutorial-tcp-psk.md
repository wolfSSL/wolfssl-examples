TCP/PSK Tutorial
================
TCP/PSK Tutorial
================

# **Tutorial for adding Cyassl Security to a Simple Client.**

1. Include the CyaSSL compatibility header:
``#include <cyassl/ssl.h>``
* Change all calls from read() or recv() to CyaSSL_read(), in the simple client 

    ``read(sockfd, recvline, MAXLINE)`` becomes ``CyaSSL_read(ssl, recvline, MAXLINE)``

3. Change all calls from write() or send() to CySSL_write(), in the simple client 

    ``write(socked, send line,strlen(send line))`` becomes ``CyaSSL_write(ssl, send line, strlen(sendline))``

4. In the main method initialize CyaSSL and CYASSL_CTX. 
     
        CyaSSL_Init();    
        
        if ((ctx = CyaSSL_CTX_new(CyaTLSv1_2_client_method())) == NULL)     
            fprintf(stderr, "SSL_CTX_new error.\n");
            return 1;
        } 

5. Create the CyaSSL object after each TCP connect and associate the file descriptor with the session:

        if ((ssl = CyaSSL_new(ctx)) == NULL) {
            fprintf(stderr, "CyaSSL_new error.\n");
            return 1;
        }
        
        ret = CyaSSL_set_fd(ssl, sockfd);

        if (ret != SSL_SUCCESS){
            return 1;
        }
        
6. Cleanup. After each CyaSSL object is done being used you can free it up by calling ``CyaSSL_free(ssl);``
7. When completely done using SSL/TLS, free the CYASSL_CTX object by 

    ``CyaSSL_CTX_free(CTX);``

    ``CyaSSL_Cleanup();``

#**Adding Pre-Shared Keys (PSK) to the CyaSSL Simple Client.**

1. When configuring CyaSSL

    ``sudo ./configure --enable-psk``

    ``sudo make``

    ``sudo make install``
        
2. In the main method add

    ``CyaSSL_CTX_set_psk_client_callback(ctx, 	My_Psk_Client_cb);``

3. Add the function 

		static inline unsigned int My_Psk_Client_Cb(CYASSL* ssl, const char* hint,
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

#**Adding Non-Blocking to the CyaSSL Simple Client.**  

1. Include the fcntl.h header file. This is needed for some of the constants that will be used when dealing with non-blocking on the socket. `` #include <fcntl.h>`` 

2. After the function ``CyaSSL_set_fd(ssl,sockfd)``, tell CyaSSL that you want non-blocking to be used. This is done by adding : `` CyaSSL_set_using_nonblock(ssl,1);``

3. Now we much invoke the fcnt callable serve to use non-blocking. 

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

        static int NonBlockingSSL_Connect(CYASSL* ssl)
        {
            int ret, error, sockfd, select_ret, currTimeout;
    
            ret    = CyaSSL_connect(ssl);
            error  = CyaSSL_get_error(ssl, 0);
            sockfd = (int)CyaSSL_get_fd(ssl);

            while (ret != SSL_SUCCESS && (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)) {
                currTimeout = 1;

                if (error == SSL_ERROR_WANT_READ)
                    printf("... client would read block\n");
                else
                    printf("... client would write block\n");

                select_ret = tcp_select(sockfd, currTimeout);

                if ((select_ret == TEST_RECV_READY) ||
                    (select_ret == TEST_ERROR_READY)) {
                    ret   = CyaSSL_connect(ssl);
                    error = CyaSSL_get_error(ssl, 0);
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
# **Tutorial for adding Cyassl Security and PSK (Pre shared Keys) to a Simple Client.**

1. Include the CyaSSL compatibility header:
``` 
#include <cyassl/ssl.h>
```

* Change all calls from read() or recv() to CyaSSL_read(), in the simple client
``` 
    read(sockfd, recvline, MAXLINE)
```
becomes
```
    CyaSSL_read(ssl, recvline, MAXLINE)
```	

3. Change all calls from write() or send() to CySSL_write(), in the simple client ``` write(socked, send line,strlen(send line)) ``` becomes ``` CyaSSL_write(ssl, send line, strlen(sendline))```

4. In the main method initialize CyaSSL and CYASSL_CTX. 
     
        CyaSSL_Init();    
        
        if ((ctx = CyaSSL_CTX_new(CyaTLSv1_2_client_method())) == NULL)     
        fprintf(stderr, "SSL_CTX_new error.\n");
        return 1;
        } 

5. Create the CyaSSL object after each TCP connect and associate the file descriptor with the session:

        if ( (ssl = CyaSSL_new(ctx)) == NULL) {
                 fprintf(stderr, "CyaSSL_new error.\n");
           return 1;
        }
        
        ret = CyaSSL_set_fd(ssl, sockfd);
        if (ret != SSL_SUCCESS){
            return 1;
        }
        
6. Cleanup. After each CyaSSL object is done being used you can free it up by calling `CyaSSL_free(ssl);`
7. When completely done using SSL/TLS, free the CYASSL_CTX object by `CyaSSL_CTX_free(CTX);`
`CyaSSL_Cleanup();`

**Now we add Pre-Shared Keys (PSK) to the CyaSSL Simple Client **

1. When configuring CyaSSL
        `sudo ./configure --enable-psk`
        `sudo make`
        `sudo make install`
        
* In the main method add
`CyaSSL_CTX_set_psk_client_callback(ctx, 	My_Psk_Client_cb);`

3. Add the function 

		static inline unsigned int My_Psk_Client_Cb(CYASSL* ssl, const char* hint,
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
    

## **Tutorial for adding Cyassl Security and PSK (Pre shared Keys) to a Simple Server.**
 
1. Include the CyaSSL compatibility header:
    ``#include <cyassl/ssl.h>``

2. Change all calls from read() or recv() to CyaSSL_read(), in the simple server
    ``read(sockfd, recvline, MAXLINE)`` 
becomes
    ``CyaSSL_read(ssl, recvline, MAXLINE)``

>(CyaSSL_read on first use also calls CyaSSL_accept if not explicitly called earlier in code.)
 
3. Change all calls from write() or send() to CySSL_write(), in the simple server
    ``write(sockfd, sendline, strlen(sendline))`` 
becomes
    ``CyaSSL_write(ssl, sendline, strlen(sendline))``

4. Run the CyaSSL method to initalize CyaSSL
    ``CyaSSL_Init()``

5. Create a ctx pointer that contains using the following process.
    ```	
    CYASSL_CTX* ctx;
	
    if ((ctx = CyaSSL_CTX_new(CyaSSLv23_server_method())) == NULL)
	    err_sys(“CyaSSL_CTX_new error”);
    ```

6. In the servers main loop for accepting clients create a CYASSL pointer. Once a new client is accepted create a CyaSSL object and associate that object with the socket that the client is on. After using the CyaSSL object it should be freed and also before closing the program the ctx pointer should be freed and a CyaSSL cleanup method called.
    ```
    CYASSL* ssl;
	
	CyaSSL_set_fd(ssl, “integer returned from accept”);

	CyaSSL_free(ssl);

	CyaSSL_CTX_free(ctx);
	CyaSSL_Cleanup();
    ```

	
## Now adding Pre-Shared Keys (PSK) to the CyaSSL Simple Server:

The following steps are on how to use PSK in a CyaSSL server

1. Build CyaSSL with pre shared keys enabled executing the following commands in CyaSSL’s root directory. Depending on file locations sudo may be needed when running the commands.
    ```
    ./configure --enable-psk
    make
    make install
    ```

2. Set up the psk suit with using the CyaSSL callback, identity hint, and cipher list methods. These methods get called immediately after the process of setting up ctx.
    ```
    CyaSSL_CTX_set_psk_server_callback(ctx, my_psk_server_cb);
    CyaSSL_CTX_use_psk_identity_hint(ctx, “cyassl server”);
    CyaSSL_CTX_set_cipher_list(ctx, “PSK-AES128-CBC-SHA256”);
    ```

>PSK-AES128-CBC-SHA256 creates the cipher list of having pre shared keys with advanced encryption security using 128 bit key with cipher block >chaining using secure hash algorithm.

3. Add the my_psk_server_cb function as follows. This is a function needed that is passed in as an argument to the CyaSSL callback.
    
    ```
    static inline unsigned int my_psk_client_cb(CYASSL* ssl, char* identity, unsigned 
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

	
Example Makefile for Simple Cyass PSK Client:
    ```	
    CC=gcc
    OBJ = client-psk.o
    CFLAG=-Wall

    %.o: %.c $(DEPS)
        $(CC) -c -o $@ $< $(CFLAGS)

    client-psk: client-psk.c
        $(CC) -Wall -o client-psk client-psk.c -lcyassl	

    .PHONY: clean

    clean:
	    rm -f *.o client-psk
    ```

The -lcyassl will link the Cyassl Libraries to your program 


The makefile for the server is going to be similar to that of the client. If the user wants separate makefiles just make a use the same set up of the client makefile and replace every instance of client-psk with server-psk. To combine make files just add a server-psk with similar ending to each time client-psk is referenced and change the target. There will also need to be a target for when compiling all targets.
    ```	
    all: server-psk client-psk

	server-psk: server-psk.c
		$(CC) -Wall -o server-psk server-psk.c -lcyassl
    ```

## Nonblocking psk
###### What is nonblocking?
When a socket is setup as non-blocking, reads and writes to the socket do not cause the application to block and wait. Instead the read or write function will read/write only the data currently available (if any). If the entire read/write is not completed, a status indicator is returned. The application may retry the read/write later.

##Tutorial for adding nonblocking to a Client.
1. Include the fcntl.h header file. This is needed for some of the constants that will be used when dealing with non blocking on the socket.

    ``#include <fcntl.h>``

2. After the function CyaSSL_set_fd(ssl, sockfd), tell cyassl that you want nonblocking to be used. This is done by adding:

    ``CyaSSL_set_using_nonblock(ssl,1);``

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





5. Before adding the NonblockingSSL_Connect function into our code we much add a tcp_select function that will be used by the NonblockingSSL_Connect. This is done by adding:

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
    static void NonBlockingSSL_Connect(CYASSL* ssl){

    int ret = CyaSSL_connect(ssl);

    int error = CyaSSL_get_error(ssl, 0);
    int sockfd = (int)CyaSSL_get_fd(ssl);
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
				ret = CyaSSL_connect(ssl);
				error = CyaSSL_get_error(ssl, 0);
			}
			else if (select_ret == TEST_TIMEOUT && !CyaSSL_dtls(ssl)) {
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
##Tutorial for adding nonblocking to a Server.

Nonblocking on the server side allows for switching between multiple client connections when reading and writing without closing them.

1. Include the fcntl.h header file. This is needed for some of the constants that will be used when dealing with non blocking on the socket.
    ``#include <fcntl.h>``
	
2. After accept has found a client and an ssl object has been made and associated with the clients socket then call the CyaSSL function to set CyaSSL in non blocking mode. This is done using the following function call.
    ``CyaSSL_set_using_nonblock(ssl, 1);``

3. Immediately after setting CyaSSL to use non blocking, the socket that the client is connected on needs to also be set up to be non blocking. This is done using the included fcntl.h and making the following function call.
    
    ``fcntl(*sockfd, F_SETFL, O_NONBLOCK);``

	>Both F_SETFL and O_NONBLOCK are constants from the fcntl.h file.

4. Include a function to select tcp. What this function does is it checks file descriptors for readiness of reading, writing, for pending exceptions, and for timeout. The timeout variable needs to point to struct timeval type. If the timeval members are 0 then the function does not block. The function and its input parameters are listed below.
    ``select(int nfds, fd_set* read, fd_set* write, fd_set* exception, struct timeval* time)`` 

	>For the example server we do not consider write when selecting the tcp so it is set to NULL. For ease the example code uses enumerated values for which state the function select returns. This then makes the next loop discussed easier.
	
5. Next is to add a loop for handling when to read and write. This loop uses the select tcp function to continually check on the status of the tcp connection and when it is ready or has an exception the CyaSSL_accept function is called.

6. The final thing added is a loop around CyaSSL_read. This is done so that when encountering the error SSL_ERROR_WANT_READ the server gives the client some time to send the message.

    ```
	/* timed loop to continue checking for a client message */
	do {
     	if (n < 0) {
         	err = CyaSSL_get_error(ssl, 0);
         	if (err != SSL_ERROR_WANT_READ)
             	err_sys("respond: read error");
         	n = CyaSSL_read(ssl, buf, MAXLINE);
         	time(&current_time);
     	}
 	} while (err == SSL_ERROR_WANT_READ && n < 0 &&
          	difftime(current_time, start_time) < seconds);
    ```


	

## Concurrent Server


The main thread accepts clients and for each client accepted a new thread is spawned that then handles the typical server processes.


1. To use multiple threads include the pthread header file.
    ``#include <pthread.h>``


2. When creating multiple threads the state of variables can become an issue. Since in the example, CYASSL_CTX* is not changed after being initially set we can make it a global variable and allow all threads read access while they are processing without having to lock the memory.


3. After the main thread accepts a client, call the pthread_create function.
    ``pthread_create(pthread_t* thread, int attribute, void* function, void* arg)``

4. In the example the function passed to pthread_create accepts one void * argument which is the socket the client is on. The function then performs the process of creating a new SSL object, reading and writing to the client, freeing the SSL object, and then terminating the thread.
    ```
    /*
    *Process handled by a thread.
    */
    void* cyassl_thread(void* fd)
    {
          CYASSL* ssl;
          int connfd = (int)fd;
          int  n;              /* length of string read */
          char buf[MAXLINE];   /* string read from client */
          char response[22] = "I hear ya for shizzle";
 
          /* create CYASSL object and respond */
          if ((ssl = CyaSSL_new(ctx)) == NULL)
              err_sys("CyaSSL_new error");
          CyaSSL_set_fd(ssl, connfd);            
                                             
          /* respond to client */                            
          n = CyaSSL_read(ssl, buf, MAXLINE);                
          if (n > 0) {                                       
              printf("%s\n", buf);                           
              if (CyaSSL_write(ssl, response, 22) > 22) {
                  err_sys("respond: write error");
              }
          }
          if (n < 0) {
              err_sys("respond: read error");
          }
                                                       
          /* closes the connections after responding */
          CyaSSL_shutdown(ssl);
         CyaSSL_free(ssl);           
         if (close(connfd) == -1)   
             err_sys("close error");
         pthread_exit( NULL);
    }
    ```
    
5. Void* arg is the argument that gets passed into cyassal_thread when pthread_create is called. In this example that argument is used to pass the socket value that the client for the current thread is on.
