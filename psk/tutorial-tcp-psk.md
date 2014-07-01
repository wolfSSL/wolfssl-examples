TCP/PSK Tutorial
================

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
    

# **Tutorial for adding Cyassl Security and PSK (Pre shared Keys) to a Simple Server.**
 
1. Include the CyaSSL compatibility header:
``` 
#include <cyassl/ssl.h>
```

* Change all calls from read() or recv() to CyaSSL_read(), in the simple server
``` 
    read(sockfd, recvline, MAXLINE)
``` 
becomes
```
    CyaSSL_read(ssl, recvline, MAXLINE)
```	
(CyaSSL_read on first use also calls CyaSSL_accept if not explicitly called earlier in code.)
 
3. Change all calls from write() or send() to CySSL_write(), in the simple server
```c 
    write(sockfd, sendline, strlen(sendline))
``` 
becomes
```c 
    CyaSSL_write(ssl, sendline, strlen(sendline))
```
4. Run the CyaSSL method to initalize CyaSSL
`	
CyaSSL_Init()
`
5. Create a ctx pointer that contains using the following process.
```c	
CYASSL_CTX* ctx;
	
if ((ctx = CyaSSL_CTX_new(CyaSSLv23_server_method())) == NULL)
	err_sys(“CyaSSL_CTX_new error”);
```

6. In the servers main loop for accepting clients create a CYASSL pointer. Once a new client is accepted create a CyaSSL object and associate that object with the socket that the client is on. After using the CyaSSL object it should be freed and also before closing the program the ctx pointer should be freed and a CyaSSL cleanup method called.
```c
CYASSL* ssl;
	
	CyaSSL_set_fd(ssl, “integer returned from accept”);

	CyaSSL_free(ssl);

	CyaSSL_CTX_free(ctx);
	CyaSSL_Cleanup();
```

	
# Now adding Pre-Shared Keys (PSK) to the CyaSSL Simple Server:

1. Build CyaSSL with pre shared keys enabled executing the following commands in CyaSSL’s root directory. Depending on file locations sudo may be needed when running the commands.
`
./configure --enable-psk
make
make install
`

2. Set up the psk suit with using the CyaSSL callback, identity hint, and cipher list methods. These methods get called immediately after the process of setting up ctx.
```c
CyaSSL_CTX_set_psk_server_callback(ctx, my_psk_server_cb);
CyaSSL_CTX_use_psk_identity_hint(ctx, “cyassl server”);
CyaSSL_CTX_set_cipher_list(ctx, “PSK-AES128-CBC-SHA256”);
```

PSK-AES128-CBC-SHA256 creates the cipher list of having pre shared keys with advanced encryption security using 128 bit key with cipher block chaining using secure hash algorithm.

3. Add the my_psk_server_cb function as follows. This is a function needed that is passed in as an argument to the CyaSSL callback.

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

	

# **Concurrent Server**


The main thread accepts clients and for each client accepted a new thread is spawned that then handles the typical server processes.


1. To use multiple threads include the pthread header file.
        **`#include <pthread.h>`**


2. When creating multiple threads the state of variables can become an issue. Since in the example, CYASSL_CTX* is not changed after being initially set we can make it a global variable and allow all threads read access while they are processing without having to lock the memory.


3. After the main thread accepts a client, call the pthread_create function.
```c
pthread_create(pthread_t* thread, int attribute, void* function, void* arg)
```
4. In the example the function passed to pthread_create accepts one void * argument which is the socket the client is on. The function then performs the process of creating a new SSL object, reading and writing to the client, freeing the SSL object, and then terminating the thread.
```c
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
