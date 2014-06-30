TCP/PSK Tutorial
================

< TODO >

# **Concurrent Server**


The main thread accepts clients and for each client accepted a new thread is spawned that then handles the typical server processes.


1. To use multiple threads include the pthread header file.
        **#include <pthread.h>**


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
          int  n;                  /* length of string read */
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

