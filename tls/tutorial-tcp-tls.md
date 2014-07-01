TCP/TLS Tutorial
================

<<<<<<< Updated upstream
<<<<<<< HEAD
## Client TLS Tutorial

### client-tls.c

Again, we will need to import the security library.  Just like in the server, add an '#include' statement in your client program.  Next we will need to add a global 'cert' variable: 
'''c
const char* cert = "./certs/ca-cert.pem";
'''
Now comes changing the 'ClientGreet()' function so its arguments and functions incorporate the security library.
'''c
void ClientGreet(int sock, CYASSL* ssl)

if (CyaSSL_write(ssl, send, strlen(send)) != strlen(send)) {

if (CyaSSL_read(ssl, receive, MAXDATASIZE) == 0) {
'''
You can think of this as, instead of just a normal read and write, it is now a “secure” read and write.  We also need to change the call to 'ClientGreet()' in 'main()'.  Instead of calling directly to it, we should make a call to a 'Security()' that will then check the server for the correct 'certs'.  To do this, change:
'''c
ClientGreet(sock(fd));
'''
to
'''c
Security(sockfd);
'''
Now we just have to make the 'Security()' 
=======
>>>>>>> a62664a86549dd488e2fa5ae902aa66da8b7a85d
=======
```
This tutorial will teach you how to install and run a basic TCP Server and Client. As well as how to incorporate CyaSSL TLS and some additional features on top of those basic programs. 

First you will need “gcc” and “make” installed on your terminal. Do this by typing:
 		
sudo apt-get install gcc make 

into your terminal and pressing enter.
```
>>>>>>> Stashed changes
