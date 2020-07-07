# WOLFSSL UDP & DTLS TUTORIAL
## Contents:
- Chapter 1: A Simple UDP Server & Client
  - 1.1.1. Introduction and Description of UDP
  - 1.1.2. Creating a UDP/IP Server
    - 1.1.2.1. Create the Socket
    - 1.1.2.2. Identify/Name the Socket
    - 1.1.2.3. Begin a Loop
    - 1.1.2.4. Wait for a Message
    - 1.1.2.5. Reply to Message
    - 1.1.2.6. Close the Socket
    - 1.1.2.7. End the Loop
  - 1.2. Create Basic UDP Client
- Chapter 2: Layering DTLS onto Simple Server and Client
  - 2.1.1. New Imports
  - 2.1.2. Increase MSGLEN
  - 2.1.3. Shifting Variables, Adding Functionality
    - 2.1.3.1. Create Cleanup
    - 2.1.3.2. Create Ctx
  - 2.1.4. Tie Client Connection Together in Main
    - 2.1.4.1. Add an Acknowledge
    - 2.1.4.2. Remove Unnecessary Variables and Move all Declarations Together
    - 2.1.4.3. Loop Shift
  - 2.1.5. Update Main Function
    - 2.1.5.1. If Defined, Turn on wolfSSL Debugging
    - 2.1.5.2. Initialize wolfSSL, Load Certificates and Keys
    - 2.1.5.3. Add Memory Freeing Functionality
  - 2.1.6. Quick Recap
  - 2.1.7. Adding DTLS to Server
    - 2.1.7.1. Avoid Socket in Use Error
    - 2.1.7.2. Await Datagram Arrival
    - 2.1.7.3. Using wolfSSL to Open a Session with Client
    - 2.1.7.4. Read the Message, Acknowledge that Message was Read
    - 2.1.7.5. Free the Memory
  - 2.2.1. Enable DTLS
  - 2.2.2. wolfSSL Tutorial
    - 2.2.2.1. Port Definition
    - 2.2.2.2. Edit Arguments
    - 2.2.2.3. Implement wolfSSL Read/Write Functions
  - 2.2.3. Set Peer
  - 2.2.4. Connect
  - 2.2.5. Write/Read
  - 2.2.6. Shutdown, Free, Cleanup
  - 2.2.7. Adjust Makefile
- Chapter 3: Multithreading a DTLS Server with POSIX Threads
  - 3.1.1. Elements Required for Thread Management
  - 3.1.1.1. threadArgs Struct
  - 3.1.1.2. Thread Creation and Deletion
  - 3.1.1.3. ThreadHandler Method
    - 3.1.1.3.1. pthread_detach
    - 3.1.1.3.2. Instance Variables
    - 3.1.1.3.3. memcpy
    - 3.1.1.3.4. 'ssl' Object Creation
    - 3.1.1.3.5. wolfSSL_accept, wolfSSL_read, wolfSSL_write, etc.
     - 3.1.1.3.6. Thread Memory Cleanup
    - 3.1.2. Review
- Chapter 4: Session Resumption with DTLS
  - 4.1.1. Storage of the Previous Session Information
  - 4.1.2. Memory Cleanup and Management
  - 4.1.3. Reconnect with Old Session Data
  - 4.1.4. Cleanup
- Chapter 5: Convert Server and Client to Nonblocking
  - 5.1. A Nonblocking DTLS Client
    - 5.1.1. A Note About Functions
    - 5.1.2. Add New Headers to Top of File
    - 5.1.3. Add Enum Variables for Testing Functions
    - 5.1.4. Adding Nonblocking DTLS Functionality to Client
      - 5.1.4.1. Variables
      - 5.1.4.2. Adding a Loop
    5.1.5. Datagram Sending
  - 5.2. A Nonblocking DTLS Server
    - 5.2.1. Add New Headers to Top of File
    - 5.2.2. Add Enum Variables for Testing Functions
    - 5.2.3. Handling Signals
      - 5.2.3.1. Static variable `cleanup`
      - 5.2.3.2. Defining a Method to Handle Signals
      - 5.2.3.3. Telling the Program to Use `sig_handler()
    - 5.2.4. Adding Nonblocking DTLS Functionality to Server
      - 5.2.4.1. Variables
      - 5.2.4.2. Adding a Loop
    - 5.2.5. Final Note
- References
##  CHAPTER 1: A Simple UDP Server & Client
###  Section 1: By Kaleb Himes
####  1.1.1. Introduction and Description of UDP
What is UDP? User Datagram Protocol (UDP) is a core member of Internet Protocol Suite (IPS). Messages sent using UDP are called datagrams. A client can send messages to other hosts (clients or servers that have host capabilities) on an Internet Protocol (IP) network without any prior communications to set up special transmissions channels or data paths. A formal definition can be found in [Request for Comments (RFC) 768](http://tools.ietf.org/html/rfc768).

UDP uses a minimal amount of protocol mechanisms in that it requires no handshake. Rather UDP uses a checksum for data integrity and port numbers for addressing different functions at the source and destination of the datagram.

> PROS: UDP is ideal for time conservation. The lack of having to re-transmit messages (if packets are lost) makes it suitable for real-time applications such as voice over IP or live streaming.

> CONS: UDP provides no guarantees to the upper layer protocol for message delivery and the UDP protocol layer retains no backup of UDP messages once sent. Therefore messages are non-recoverable once sent.

In summary, these are the key points regarding UDP:
- No connection to create and maintain
- More control over when data is sent
- No error recovery
- No compensation for lost packets
- Packets may arrive out of order
- No congestion control
- Overall, UDP is lightweight but unreliable. Some applications where UDP is used include DNS, NFS, and SNMP.

UDP header consists of 4 fields, each of which is 16 bits.
1. Field 1 contains information about the port from which the message originated.
2. Field 2 contains information about the destination port.
3. Field 3 defines the length in bytes of the UDP header and data. Allows for datagram of maximum size 65,535 bytes to be defined. If larger, set to 0. (IPv6 will allow larger).
4. Field 4 contains a Checksum for security purposes. Contains some checksum of the header and data.

###### CHECKSUM
All 16 bit words are summed using “one's complement arithmetic”. The sum is then “one's complemented” and this is the value placed in the UDP checksum field.

##### Figure 1.1: Checksum Example
Example: you have two 16 bit words as follows:
```c
    ONE's COMPLEMENT OF WORD1:     1 0 0 1 1 1 1 0 0 1 1 0 0 0 1 0
    ONE's COMPLEMENT OF WORD2:     0 1 0 1 0 0 0 1 0 1 0 0 1 1 0 1

                          SUM:     1 1 1 0 1 1 1 1 1 0 1 0 1 1 1 1
                                   -------------------------------
      ONE's COMPLEMENT OF SUM:     0 0 0 1 0 0 0 0 0 1 0 1 0 0 0 0
```
The final value would be placed in the Checksum Field.

####  1.1.2. Creating a UDP/IP Server
There are seven initial steps to creating a UDP/IP Server.

```c
1. Create the socket
2. Identify the socket (IE give it a name)
3. <Begin a loop>
4. On the server wait for a message
5. Send a response to the client once message is received
6. Close the socket (in our case, just return to looking for packets arriving).
7. <End of loop>
```

####  1.1.2.1. Create the Socket

A socket is created with the socket system call.

##### Figure 1.2: Create a Socket
```c
int sockfd = socket(domain, type, protocol);
```
Let's briefly discuss the parameters domain, type, and protocol.

1. Domain
    The domain can also be referred to as the address family. It is the communication domain in which the socket should be created. Below you will see some of the examples domains or address families that we  could work with. At the end will be a description of what we will choose specifically for a UDP server.
```c
AF_INET:     Internet Protocol (IP)
AF_INET6:    IP version 6 (IPv6)
AF_UNIX:     local channel, similar to pipes
AF_ISO:      “In Search Of” (ISO) protocols
AF_NS:       Xerox Network Systems protocols
```
2. Type
    This is the type of service we will be providing with our UDP server. This is selected based on the requirements of the application, and will likely aid in determining which Domain (above) you will select ultimately.
```c
SOCK_STREAM:     a virtual circuit service
SOCK_DGRAM:      a datagram service
SOCK_RAW:        a direct IP service
```
3. Protocol
    A protocol supports the sockets operation. This parameter is optional but is helpful in cases where the domain (family) supports multiple protocols. In these cases we can specify which protocol to use for said family. If the family supports only one type of protocol this value will be zero.

For this tutorial we want to select domain `(domain = AF_NET)` and the datagram service `(type = SOCK_DGRAM)`. There is only one form of datagram service therefore we do not need to specify a protocol for a UDP/IP server `(protocol = 0)`.

##### Figure 1.3: Setting Protocol
```c
#include <sys/socket.h>
…
int sockfd;

sockfd = socket(AF_INET, SOCK_DGRAM, 0);

if (sockfd < 0) {
    perror("cannot create socket");
    return 0;
}
printf("created socket: descriptor=%d\n", sockfd);
```

####  1.1.2.2. Identifty/Name the Socket

By naming the socket we are assigning a transport address to the socket (I.E. a port number in IP networking). This is more commonly referred to as “binding” an address. The bind system call is used to do this. In other words we are giving our server a unique address so that communication with our server can be established much like a person with a mailing address. They can receive communiques (letters) via their mailbox. Our server can do the same once it has a “bound” socket.

The transport address is defined in the socket address structure. Since sockets can receive and send data  using a myriad of communication interfaces, the interface is very general. Rather than accepting a port number as a parameter, it will look for a “sockaddr” (short for socket address)structure whose format is based off the address family we chose.

`# include <sys/socket.h>` contains the relevant “bind” call we will need. Since we already used it in 1.1.2.1 we do not need to include it a second time.

##### Figure 1.4: Bind the Socket
```c
int
bind(int socket, const struct sockaddr *address, socklen_t address_len);
```
The first parameter “socket” is the socket we created in 1.1.2.1 `(sockfd)`
The second parameter `(sockaddr)` will allow the operating system (OS) to read the first couple bytes that identify the address family. For UDP/IP networking we will use `struct sockaddr_in`, which is defined in the `netinet/in.h` system library.

##### Figure 1.5: Explain Address Family
```c
struct sockaddr_in{
    __uint8_t        sin_len;
    sa_family_t      sin_family;
    in_port_t        sin_port;
    struct in_addr   sin_addr;
    char             sin_zero[8];
};
```
NOTE: this code will not be found in our example server. It is imported with the following call:
`# include <netinet/in.h>`

Before calling bind, we need to fill this struct. The three key parts we need to set are:
1. `sin_family`
    The address family we used in 1.1.3.1 `(AF_INET)`.
2. `sin_port`
    The port number (transport address). This can either be explicitly declared, or you can allow the OS to assign one. Since we are creating a server, ideally we would want to explicitly declare a well known port so that clients know where to address their messages. However for this particular tutorial we will use the generic `11111` (five ones). This will be defined directly beneath the include section of our code: `# define SERV_PORT  11111`

    We can then call `SERV_PORT` where it is needed and if you, the client, are already using port `11111` for any particular reason, you can then easily redefine it as needed for this tutorial. Additionally if you use `# define SERV_PORT 0`, your system will use any available port.
3. `sin_addr`
    The address for our socket (your server machines IP address). With UDP/IP our server will have one IP address for each network interface. Since the address can vary based on transport methods and we are using a client computer to simulate a server, we will use the `INADDR_ANY`.

##### Figure 1.6
```c
memset(&servAddr, 0, sizeof(servAddr));
servAddr.sin_family = AF_INET;
servAddr.sin_port = htons(SERV_PORT);
inet_pton(AF_INET, argv[1], &servAddr.sin_addr);
```

###### DESCRIPTIONS OF NUMBER CONVERSIONS UTILIZED IN NETWORKING
1. “htons”
    host to network - short : convert a number into a 16-bit network representation. This is commonly used to store a port number into a sockaddr structure.
2. “htonl”
    host to network - long : convert a number into a 32-bit network representation. This is commonly used to store an IP address into a sockaddr structure.
3. “ntohs”
    network to host - short : convert a 16-bit number from a network representation into the local processor`s format. This is commonly used to read a port number from a sockaddr structure.
4. “ntohl”
    network to host - long : convert a 32-bit number from a network representation into the local processor`s format. This is commonly used to read an IP address from a sockaddr structure.

Using any of the above 4 macros will guarantee that your code remains portable regardless of the architecture you use in compilation.

####  1.1.2.3. <Begin a Loop>
#### 1.1.2.4. Wait for a Message
Later when we layer on DTLS our server will set up a socket for listening via the “listen” system call. The server would then call “accept” upon hearing a request to communicate, and then wait for a connection to be established. UDP however in its base for is connectionless. So our server, as of right now, is capable of listening for a message purely due to the fact that it has a socket! We use `recvfrom` system call to wait for an incoming datagram on a specific transport address (IP address, and port number).

The `recvfrom` call is included with the `# include <sys/socket.h>` therefore we do not need to include this library again since we already included it in 1.1.2.1.

##### Figure 1.7: “recvfrom”
```c
int recvfrom(int socket, void* restrict buffer, size_t length,
             int flags, struct sockaddr* restrict src_addr, socklen_t *src_len);
```

###### PARAMETERS DEFINED
1. `int socket`
    The first parameter `socket` is the socket we created and bound in 1.1.2.1 & 1.1.2.2. The port number assigned to that socket via the “bind” tells us what port `recvfrom` will “watch” while awaiting incoming data transmissions.
2. `void* restrict buffer`
    The incoming data will be placed into memory at buffer.
3. `size_t length`
    No more than `length` bytes will be transferred (that's the size of your buffer).
4. `int flags`
    For this tutorial we can ignore this last flags. However this parameter will allow us to “peek” at an incoming message without removing it from the queue or block until the request is fully satisfied. To ignore these flags, simply place a zero in as the parameter. See the man page for `recvfrom` to see an  in-depth description of this parameter.
5. `struct sockaddr* restrict src_addr`
    If `src_addr` is not `NULL`, the IP address and port number of the sender of the data will be placed into memory at `src_addr`.
6. `socklen_t *src_len`
    The size of the memory at `src_addr`. If `src_addr` is `NULL`, then `src_len` should also be `NULL`.
    Example:
    ```c
    struct sockaddr_in* cliaddr;
    socklen_t addrlen;
    addrlen = sizeof(struct sockaddr_in);
    ```
###### RETURN VALUE
`recvfrom` returns the number of bytes received, or `-1` if an error occurred.

##### Figure 1.8: Looping Receive
```c
for (;;) {
    printf("waiting for client message on port %d\n", SERV_PORT);

    recvlen = recvfrom(sockfd, buf, MSGLEN, 0,
                       (struct sockaddr *)&cliaddr, &addrlen);

    printf("heard %d bytes\n", recvlen);

    if (recvlen > 0) {
        buf[recvlen] = 0;
        printf("I heard this: \"%s\"\n", buf);
    }
    else
        printf("lost the connection to client\n");
}
```

####  1.1.2.5. Reply to Message
Now we are able to receive messages from our clients but how do we let clients know their messages are being received? We have no connection to the server and we don’t know their IP address. Fortunately the `recvfrom` call gave us the address, and it was placed in `remaddr`:

##### Figure 1.9
```c
recvlen = recvfrom(sockfd, buf, MSGLEN, 0, (struct sockaddr *)&cliaddr, &addrlen);
```
The server can use that address in `sendto` and send a message back to the recipient’s address.

##### Figure 1.10
```c
sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&cliaddr, addrlen)
```

####  1.1.2.6. Close the Socket
This step is not necessary for our examples server, however can easily be accomplished with a call to `close()`.

##### Figure 1.11
```c
close(sockfd);
```
#### 1.1.2.7 <End the Loop>
This concludes the simple UDP server portion of Chapter 1. Section 2 will now cover a Simple UDP Client.

### Section 2: By Leah Thompson
####  1.2. Create Basic UDP Client:
Create a function to send and receive data.

This function will send a message to the server and then loop back. The function does not return anything and takes in 4 objects: the input variable name, a socket, a pointer to a socket address structure, and a length for the address.

Within this function, we will read in user input `(fgets)` from the client and loop while this input is valid. This loop will send the input to the server using the `sendto()` function. It will then read back the server’s echo with `recvfrom()` and print this `echo(fputs)`. Our function:

##### Figure 1.12
```c
void DatagramClient (FILE* clientInput, WOLFSSL* ssl) {

    int  n = 0;
    char sendLine[MAXLINE], recvLine[MAXLINE - 1];

    fgets(sendLine, MAXLINE, clientInput);

    if ( ( wolfSSL_write(ssl, sendLine, strlen(sendLine))) !=
           strlen(sendLine))
        err_sys("SSL_write failed");

    n = wolfSSL_read(ssl, recvLine, sizeof(recvLine)-1);
    if (n < 0) {
        int readErr = wolfSSL_get_error(ssl, 0);
        if(readErr != SSL_ERROR_WANT_READ)
            err_sys("wolfSSL_read failed");
    }

    recvLine[n] = '\0';
    fputs(recvLine, stdout);
}
```
This function can be accomplished within main without creating an additional function.

##  CHAPTER 2: Layering DTLS onto Simple Server and Client
###  Section 1:
####  2.1.1. New Imports
We will begin by adding the following libraries to pull from.
##### Figure 2.1
```c
#include <wolfssl/ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
```

####  2.1.2. Increase `MSGLEN`
Next change the size of our `MSGLEN` to 4096 to be more universal. This step is unnecessary if you’re testing against the client.c located in `wolfssl/examples/client` as it will only send a message of length 14 or so. With this said, why not be able to handle a little user input if we want to test against a friend's client for example?


####  2.1.3. Shifting Variables, Adding Functionality
####  2.1.3.1. Create `cleanup`
We will create a global `static int` called `cleanup`. This variable will be our signal to run `wolfSSL_cleanup();` which will free the wolfSSL libraries and all allocated memory at the end of our program.

####  2.1.3.2. Create `ctx`
Now we declare a `WOLFSSL_CTX` pointer and call it `ctx` for simplicity.
Your global variable section should now look something like this:
##### Figure 2.2
```c
# includes here...
    WOLFSSL_CTX* ctx;
    static int cleanup;                 /* To handle shutdown */
    struct sockaddr_in servaddr;        /* our server's address */
    struct sockaddr_in cliaddr;         /* the client's address */
```
####  2.1.4. Tie Client Connection Together in Main
####  2.1.4.1. Add an Acknowledge
Add the variable `char ack` inside the main function. This will be a reply message that we send to our clients. Ack is short for “Acknowledge”. So our clients have some sort of feedback letting them know that we received their communication successfully. Section 7 of Chatper 2.1 sees the use of `ack`.

#### 2.1.4.2. Remove Unnecessary Variables and Move all Declarations Together
We will no longer refer to the open socket as `sockfd`, instead we will now call it `listenfd`. This reminds us that the socket is just listening for packets to arrive and we are not actually handling those packets immediately like we did in Chapter 1. We will want to confirm our client is encrypting their data with an acceptable cypher suite prior to opening a communication channel with them.

With this change we will also rename `addrlen` to `clilen` to remind us that this `socklen_t` is the length of the clients address and not to be confused with our socket’s address. We will no longer assign the length of `clilen` upon declaration either. That will be handled later in our program. Remove `msgnum` altogether. Take note: `recvlen` is being declared here however is not used until we have verified our client is encrypting with DTLS version 1.2. Section 7 of Chapter 2.1 sees the use of `recvlen`. Our variable section should now look something like this:

##### Figure 2.3
```c
int                  on = 1;
int                 res = 1;
int              connfd = 0;
int             recvlen = 0;    /* length of message */
int            listenfd = 0;    /* Initialize our socket */
WOLFSSL* ssl =          NULL;
socklen_t            clilen;
socklen_t len =  sizeof(on);
unsigned char       b[1500];    /* watch for incoming messages */
char           buff[MSGLEN];    /* the incoming message */
char ack[] = "I hear you fashizzle!\n";
```

#### 2.1.4.3. Loop Shift
With the layering on of DTLS we will need to re-allocate our socket and re-bind our socket for each client connection. Since we will need to free up all memory allocated to handle these connections and additional security our loop will now change to a `while` loop instead of a `for` loop. We will loop on the condition that `cleanup != 1`. If `cleanup == 1` we will run `wolfSSL_cleanup()`.

So while not 1 we will keep our socket open and continue listening for packets to arrive. Start a `while` in your main function. Move the current code from main into the loop. At this point, you should have a loop similar to this inside your main function:

##### Figure 2.4
```c
while (cleanup != 1) {
    /* create a UDP/IP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket");
        return 0;
    }

    /* INADDR_ANY = IPaddr, socket =  11111, modify SERV_PORT to change */
    memset((char *)&servaddr, 0, sizeof(servaddr));
    /* host-to-network-long conversion (htonl) */
    /* host-to-network-short conversion (htons) */

    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(SERV_PORT);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        return 0;
    }

    printf("waiting for client message on port %d\n", SERV_PORT);
    recvlen = recvfrom(sockfd, buf, MSGLEN, 0,
                        (struct sockaddr *)&cliaddr, &addrlen);
    printf("heard %d bytes\n", recvlen);

    if (recvlen > 0) {
        buf[recvlen] = 0;
        printf("I heard this: \"%s\"\n", buf);
    }
    else {
        printf("lost the connection to client\n");
    }
    sprintf(buf, "Message # %d received\n", msgnum++);
    printf("reply sent \"%s\"\n", buf);

    if (sendto(sockfd, buf, strlen(buf), 0,
               (struct sockaddr *)&cliaddr, addrlen) < 0) {
        perror("sendto");
    }
    /* continues to loop, use "Ctrl+C" to terminate listening */
}
```

####  2.1.5. Update Main Function
####  2.1.5.1. If Defined, Turn on wolfSSL Debugging
This is pretty self-explanatory.
##### Figure 2.5
`wolfSSL_Debugging_ON();`

#### 2.1.5.2. Initialize wolfSSL, Load Certificates and Keys
In order for these to load properly you will need to place a copy of the `certs` file one directory above your current working directory. You can find a copy of the `certs` file in wolfssl home directory. Simply copy and paste this file into the directory one up from your working directory, or change the file path in the code to search your wolfssl home directory for the certs file.
##### Figure 2.6: Main After Additions
```c
wolfSSL_Init();                      /* Initialize wolfSSL */

/* Set ctx to DTLS 1.2 */
if ( (ctx = wolfSSL_CTX_new(wolfDTLSv1_2_server_method())) == NULL){
    fprintf(stderr, "wolfSSL_CTX_new error.\n");
    exit(EXIT_FAILURE);
}
/* Load CA certificates */
if (wolfSSL_CTX_load_verify_locations(ctx,"../certs/ca-cert.pem",0) !=
                                                             SSL_SUCCESS) {
    fprintf(stderr, "Error loading ../certs/ca-cert.pem, "
                       "please check the file.\n");
    exit(EXIT_FAILURE);
}
/* Load server certificates */
if (wolfSSL_CTX_use_certificate_file(ctx,"../certs/server-cert.pem",
                                    SSL_FILETYPE_PEM) != SSL_SUCCESS) {
    fprintf(stderr, "Error loading ../certs/server-cert.pem, "
                                   "please check the file.\n");
    exit(EXIT_FAILURE);
}
/* Load server Keys */
if (wolfSSL_CTX_use_PrivateKey_file(ctx,"../certs/server-key.pem",
                                   SSL_FILETYPE_PEM) != SSL_SUCCESS) {
    fprintf(stderr, "Error loading ../certs/server-key.pem, "
                                     "please check the file.\n");
    exit(EXIT_FAILURE);
}
```

#### 2.1.5.3. Add Memory Freeing Functionality
Add a conditional statement that will free up any allocated memory at the termination of our program.
Last, add a return method for `main()`.

##### Figure 2.7: Cleanup for Main
```c
if (cleanup == 1)
    wolfSSL_CTX_free(ctx);
return 0;
```
#### 2.1.6. Quick Recap
So we’ve loaded all the certificates and keys we will need to encrypt any and all communications sent between our server and client. This encryption will be of type DTLS version 1.2 as seen in Figure 2.6, where you see `wolfDTLSv1_2_server_method()`. In order for a client to now talk to our DTLS encrypted server they themselves will have to have certificates to verify our encryption, accept our key, and perform a DTLS handshake. See section 2 of this chapter for a tutorial on encrypting a client with DTLS version 1.2.


#### 2.1.7. Adding DTLS to Server
#### 2.1.7.1. Avoid Socket in Use Error
Our client handling is now running in a `while` loop, so it will continue to listen for clients even after a client has communicated with us and the closed their port. Our program will re-allocate that socket, rebind that socket and continue to await the arrival of more datagrams from that same or different clients. Our first step to avoid potential errors with our program will be to avoid “socket already in use” errors. Initialize two dummy integers, `res` and `on`. set both equal to `1`. Then initialize a `struct` of type `socklen_t` (same as our `clilen` for getting the length of the clients address) call it `len` and set it equal to the size of `on`. We will use these variables to set the socket options to avoid that error.

##### Figure 2.8: Our Code
(socket)(level)(option_name)(option_value) (option_len)
```c
int setsockopt(int socket, int level, int option_name, const void*option_value, socklen_t option_len);
```

`res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);`

The `setsockopt()` function sets the option specified by the `option_name` argument, at the protocol level specified by the `level` argument, to the value pointed to by the `option_value` argument for the socket associated with the file descriptor specified by the `socket` argument.

The `level` argument specifies the protocol level at which the option resides. To set options at the socket level, specify the level argument as `SOL_SOCKET`. To set options at other levels, supply the appropriate protocol number for the protocol controlling the option. For example, to indicate that an option will be interpreted by the TCP (Transport Control Protocol), set level to the protocol number of TCP, as defined in the `<netinet/in.h>` header.

We will also check to ensure that the method did not fail as if it did it we would think we were listening for clients when in reality our server would not be able to receive any datagrams on that socket and clients would be getting rejected without our knowledge.

##### Figure 2.9: Eliminate Socket Already in Use Error
```c
int res = 1;
int on = 1;
socklen_t len = sizeof(on);
res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
if (res < 0) {
    printf("Setsockopt SO_REUSEADDR failed.\n");
    cleanup = 1;
}
```
This error avoidance code will be inserted just after we finish our host to network short conversion of `SERV_PORT` and just before we bind the socket.

NOTE: We have now changed our error handling from:
`perror(“some message”); and return 0;`
to:
`printf(“some message”); and cleanup = 1;`
 All active error situations will now change to match this format and all future error situations will be of the same format. This will ensure that all memory is freed at the termination of our program.


#### 2.1.7.2. Await Datagram Arrival
Here is where we will now set `clilen = sizeof(cliaddr);` as well. We will declare an `unsigned char` that will behave as `buff` does. `Buff` will now be used by `ssl` to read client messages and the new `buff` (we will just call it `b`) is for one purpose only: to “peek” at any incoming messages. We will not actually read those messages until later on. Right now we just want to see if there is a message waiting to be read or not. We must do this on a UDP server because no actual “connection” takes place… perseh. Packets arrive or they don’t -  that’s how UDP works. Then we will perform some error handling on the fact that there are Datagrams waiting or there aren’t. If none are waiting we will want our `while` loop to cycle and continue to await packet arrivals. If there are packets sitting there then we want to know about it.

##### Figure 2.10
```c
/* set clilen to |cliaddr| */
clilen =    sizeof(cliaddr); /* will be moved to the variable section later */
unsigned char       b[1500]; /* will be moved to the variable section later */
int              connfd = 0; /* will be moved to the variable section later */

connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
                                  (struct sockaddr*)&cliaddr, &clilen);
if (connfd < 0){
    printf("No clients in que, enter idle state\n");
    continue;
}

else if (connfd > 0) {
    if (connect(listenfd, (const struct sockaddr *)&cliaddr,
        sizeof(cliaddr)) != 0) {
        printf("Udp connect failed.\n");
        cleanup = 1;
    }
}
else {
    printf("Recvfrom failed.\n");
    cleanup = 1;
}
printf("Connected!\n");
```
We say “Connected” for user friendly interpretation, but what we really mean is: “Datagrams have arrived, a client is attempting to communicate with us… let’s perform a handshake and see if they are using DTLS version 1.2… if so, we’ll read their Datagrams and see what they have to say”.

#### 2.1.7.3. Using wolfSSL to Open a Session with Client.
First we must declare an object that points to a `WOLFSSL` structure. We will just call it `ssl`. We will then assign it to use the correct cypher suite as previously defined by `ctx`. We will again perform some error handling on this assignment and then set the file descriptor that will handle all incoming and outgoing messages for this session.

Once all that has been set up we are ready to check and see if our client is using an acceptable cypher suite. We accomplish this by making a call to `wolfSSL_accept` on our `ssl` object that is now pointing to the file descriptor that has an awaiting Datagram in it. (That's a lot - we know). We’ll use some fancy calls to error get methods so that if this part fails we will have a little bit of an idea as to why it failed and how to fix it.

##### Figure 2.11
```c
/* initialize arg */
WOLFSSL* ssl;

/* Create the WOLFSSL Object */
if (( ssl = wolfSSL_new(ctx) ) == NULL) {
    printf("wolfSSL_new error.\n");
    cleanup = 1;
}

/* set the session ssl to client connection port */
wolfSSL_set_fd(ssl, listenfd);

if (wolfSSL_accept(ssl) != SSL_SUCCESS) {
    int err = wolfSSL_get_error(ssl, 0);
    char buffer[80];
    printf("error = %d, %s\n", err, wolfSSL_ERR_error_string(err, buffer));
    buffer[sizeof(buffer)-1]= 0;
    printf("SSL_accept failed.\n");
    cleanup = 1;
}
```

#### 2.1.7.4. Read the Message, Acknowledge that Message was Read
So our client is using DTLS version 1.2, we have a message waiting for us… so let’s read it! Then let’s send our client an acknowledgment that we have received their message. Don’t forget to handle any potential errors along the way!

##### Figure 2.12
```c
if (( recvlen = wolfSSL_read(ssl, buff, sizeof(buff)-1)) > 0){
    printf("heard %d bytes\n", recvlen);
    buff[recvlen - 1] = 0;
    printf("I heard this: \"%s\"\n", buff);
}

if (recvlen < 0) {
    int readErr = wolfSSL_get_error(ssl, 0);
    if(readErr != SSL_ERROR_WANT_READ) {
    printf("SSL_read failed.\n");
    cleanup = 1;
    }
}

if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0) {
    printf("wolfSSL_write fail.\n");
    cleanup = 1;
}
else
    printf("lost the connection to client\n");
    printf("reply sent \"%s\"\n", ack);
```
#### 2.1.7.5. Free the Memory
Finally we need to free up all memory that was allocated on our server for this particular session. If the client so chooses they can connect with us again using the same session key. But we do not want to bog down our server with unnecessary messages from previous sessions, once those messages have been handled. So we reset the file descriptor to 0 allocated bits, and shutdown our active `ssl` object and free the memory. Lastly we print out a line letting us know that we have reached the end of our loop successfully, all memory is free, and we are returning to the top to listen for the next client that might want to talk to us.

##### Figure 2.13: Freeing Memory and Shutting Down
```c
wolfSSL_set_fd(ssl, 0);
wolfSSL_shutdown(ssl);
wolfSSL_free(ssl);
printf("Client left return to idle state\n");
```
This concludes Section 1 of Chapter 2 on “Layering DTLS onto a UDP Server”. Section 2 will now cover Layering DTLS onto a UDP Client.

### Section 2:
#### 2.2.1. Enable DTLS
As stated in chapter 4 of the wolfSSL manual, DTLS is enabled by using the `--enable-dtls` build option when building wolfSSL. If you have not done so, this should be your first step.

#### 2.2.2. wolfSSL Tutorial
Walk through chapter 11 in the wolfSSL tutorial. Follow the guides for TLS/SSL using the correct wolfDTLS client method. There are a few adjustments to be made for DTLS, detailed in the next few steps.


#### 2.2.2.1. Port Definition
Make sure you have the correct port defined in your program. For example,
`# define SERV_PORT 11111` for server with `11111` set as the port.

#### 2.2.2.2. Edit Arguments
Edit the arguments in your send and receive function to just 2 arguments: a `FILE*` object and a `WOLFSSL*` object (previously this function had 4 arguments).

#### 2.2.2.3. Implement wolfSSL Read/Write Functions
Change `sendto` and `recvfrom` functions to `wolfSSL_write` and `wolfSSL_read`.
Delete the last 3 arguments that were in `sendto` and `recvfrom`. In the `wolfSSL_read()` call, change the first argument from a socket to the `WOLFSSL*` object from the original function call.

#### 2.2.3. Set Peer
Make a call to the `wolfSSL_dtls_set_peer()` function. It will take in as arguments your `WOLFSSL*` object, a pointer to the address carrying your `sockaddr_in` structure, and `size` of the structure. Example:
##### Figure 2.14
```c
wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));
```
This function will be called between where you built the `sockaddr_in` structure and your socket creation.
#### 2.2.4. Connect
Add a wolfSSL connect function below the call to `wolfSSL_set_fd()` and pass the `WOLFSSL*` object you created as the argument. Include error checking. Example:
##### Figure 2.15

```c
if  (wolfSSL_connect(ssl) != SSL_SUCCESS) {
    int err1 = wolfSSL_get_error(ssl, 0);
    char buffer[80];
    printf("err = %d, %s\n", err1,yaSSL_ERR_error_string(err1, buffer));
    err_sys("SSL_connect failed");
}
```
#### 2.2.5. Write/Read
Call your `wolfSSL_write`/`wolfSSL_read` functions. Example:
##### Figure 2.16
```c
wolfSSL_read(ssl, buff, sizeof(buff)-1);
wolfSSL_write(ssl, ack, sizeof(ack));
```

#### 2.2.6. Shutdown, Free, Cleanup
Make calls to `wolfSSL_shutdown()`, `wolfSSL_free()`, `wolfSSL_CTX_free()`, and `wolfSSL_Cleanup()` with correct parameters in each. This can be done in the read/write function or at the end of the main method.

#### 2.2.7. Adjust Makefile
Include `-DWOLFSSL_DTLS` before `-o` in your compilation line. This will include the DTLS method you chose.

##  Chapter 3: Multithreading a DTLS Server with POSIX Threads
### Section 1: by Alex Abrahamson

To make a DTLS server multithreaded, the most effective way to do this would be to use POSIX threads.

From Wikipedia's [POSIX Threads page](https://en.wikipedia.org/wiki/POSIX_Threads):

> POSIX Threads ... allows a program to control multiple different flows of work that overlap in time. Each flow of work is referred to as a thread, and creation and control over these flows is achieved by making calls to the POSIX Threads API.

In order to multithread a DTLS server, three things will need to be completed.
1. Elements required for thread management should be added
2. Creation of a `ThreadHandler` method
3. Cleanup

(*a quick note: be sure to have gone over signal handling and have code for signal handling in your main method prior to attempting multithread your server. Information on signal handling can be found in section 11.12 of the* [SSL tutorial](https://wolfssl.com/wolfSSL/Docs-wolfssl-manual-11-ssl-tutorial.html))

#### 3.1.1. Elements Required for Thread Management
The main elements of thread management using `pthread` are

- Data given to each thread created (usually in the form of a struct)
- Thread creation and deletion method calls (usually `pthread_create`, `pthread_join`, `pthread_detach`, etc.)
- A thread ID
- A ThreadHandler method

The following sections go over each of the items listed above.

#### 3.1.1.1. `threadArgs` struct

The `threadArgs` struct is needed to so that we can simultaneously pass multiple pieces of information to our `pthread` being created. It will allow us to pass in a file descriptor, message, size, and whatever else we can think of - but for now it will just contain those three things. Below is the definition of the `threadArgs` struct.
##### Figure 3.1
```c
typedef struct {
    int activefd;
    char b[MSGLEN];
    int size;
} threadArgs;
```
The actual creation of each `threadArgs*` object will be done at the beginning of each iteration of the while loop located within `AwaitDGram`. Inside of that loop, we will create the object, and also dynamically allocate it with `malloc` so that it is accessible to the thread and us from anywhere.
##### Figure 3.2
```c
while (cleanup != 1) {
    threadArgs* args;
    args = (threadArgs *) malloc(sizeof(threadArgs));
      .
      .
      .
}
```

#### 3.1.1.2. Thread Creation and Deletion
After a client has connected to the server, we need to create a thread and "spin" it to handle receiving and sending messages (spinning a thread is just spinning it off of the current process and giving it a method to take care of itself). For thread creation, there are two things to do - create a thread ID, and call `pthread_create`.

From the man page of `pthread_create`,
```
NAME
       pthread_create - create a new thread

SYNOPSIS
       #include <pthread.h>

       int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);
```

When we call `pthread_create`, it should be done like this:
```c
if (cleanup != 1) {
    pthread_t threadid;
    pthread_create(&threadid, NULL, ThreadHandler, args);
}
```
*note `ThreadHandler` being passed as the 3rd argument - this is a method we will define*

To control what the thread does when it is deleted/its resources are freed, we will have to define a `ThreadHandler` method.

#### 3.1.1.3. `ThreadHandler` Method

When writing a method to handle thread execution, imagine that every thread is its own server being executed, and that the thread handler method is the `main` method of each of these servers. But it isn't entirely the same as a main method - it takes a void pointer as input, and requires some special features to keep itself in check. A common and simple outline of what a `ThreadHandler` should include is listed below.

1. A `pthread_detach( pthread_self() )` call
2. Instance variables (file descriptors, message length, reply, `WOLFSSL*` object)
3. A `memcpy` call
4. `ssl` object creation
5. `wolfSSL_accept`, `wolfSSL_read` and `wolfSSL_write` calls with error checking for all three
6. Memory cleanup (`wolfSSL_shutdown`, `wolfSSL_free`, etc.)

As we go over items **1** through **6**, we'll continue adding meat to our `ThreadHandler` method.

Each commented section will be filled in as each of the following sections is completed.


####  3.1.1.3.1. `pthread_detach`
Calling `pthread_detach` should be the very first thing done within the method - it specifies that when the thread terminates, its resources will be released. The parameter passed to `pthread_detach` should be the thread itself.

Our `ThreadHandler` code now looks like this:
```c
void* ThreadHandler(void *input)
{
    /* pthread_detach call */
    pthread_detach( pthread_self() );

    /* Instance variables */

    /* memcpy call */

    /* miscellaneous standard server calls */

    /* Thread memory cleanup */
}
```

####  3.1.1.3.2. Instance variables
In total, there are 7 local variables used inside of `ThreadHandler`. They are all listed below with a brief description of their purposes.
```c
threadArgs*     args = (threadArgs*) input;
int             recvLen = 0;                /* Length of message received */
int             activefd = args->activefd;  /* active file descriptor */
int             msgLen = args->size;        /* Length of message */
unsigned char   buff[msgLen];               /* Incoming message from client */
char            back[] = "MESSAGE RECEIVED" /* Message sent to client */
WOLFSSL*        ssl;                        /* SSL object */
```

Putting these variables into our code gives us a new `ThreadHandler` method:

```c
void* ThreadHandler(void *input)
{
    /* pthread_detach call */
    pthread_detach( pthread_self() );

    /* Instance variables */
    threadArgs*     args = (threadArgs*) input;
    int             recvLen = 0;                /* Length of message received */
    int             activefd = args->activefd;  /* active file descriptor */
    int             msgLen = args->size;        /* Length of message */
    unsigned char   buff[msgLen];               /* Incoming message from client */
    char            back[] = "MESSAGE RECEIVED" /* Message sent to client */
    WOLFSSL*        ssl;                        /* SSL object */

    /* memcpy call */

    /* miscellaneous standard server calls */

    /* Thread memory cleanup */
}
```
####  3.1.1.3.3. `memcpy`
Calling `memcpy` allows the user to clear out the previous data located in `args->b`. The method should be called like the example is below.
```c
memcpy(buff, args->b, msgLen);
```

Putting this call into our code gives this `ThreadHandler` method:
```c
void* ThreadHandler(void *input)
{
    /* pthread_detach call */
    pthread_detach( pthread_self() );

    /* Instance variables */
    threadArgs*     args = (threadArgs*) input;
    int             recvLen = 0;                /* Length of message received */
    int             activefd = args->activefd;  /* active file descriptor */
    int             msgLen = args->size;        /* Length of message */
    unsigned char   buff[msgLen];               /* Incoming message from client */
    char            back[] = "MESSAGE RECEIVED" /* Message sent to client */
    WOLFSSL*        ssl;                        /* SSL object */

    /* memcpy call */
    memcpy(buff, args->b, msgLen);

    /* miscellaneous standard server calls */

    /* Thread memory cleanup */
}
```

####  3.1.1.3.4. 'ssl' Object Creation
There shouldn't be anything special about the creation of the `ssl` object. It should be identical to a normal creation, but should be assigned inside of `ThreadHandler`. The purpose of creating a `WOLFSSL` object in `ThreadHandler` is that each thread will maintain a DTLS connection to a client.

```c
void* ThreadHandler(void *input)
{
    /* pthread_detach call */
    pthread_detach( pthread_self() );

    /* Instance variables */
    threadArgs*     args = (threadArgs*) input;
    int             recvLen = 0;                /* Length of message received */
    int             activefd = args->activefd;  /* active file descriptor */
    int             msgLen = args->size;        /* Length of message */
    unsigned char   buff[msgLen];               /* Incoming message from client */
    char            back[] = "MESSAGE RECEIVED" /* Message sent to client */
    WOLFSSL*        ssl;                        /* SSL object */

    /* memcpy call */
    memcpy(buff, args->b, msgLen);

    /* miscellaneous standard server calls */

    /* Create the WOLFSSL Object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        cleanup = 1;
        return NULL;
    }

    /* Thread memory cleanup */
}
```
#### 3.1.1.3.5.`wolfSSL_accept`, `wolfSSL_read`, `wolfSSL_write`, etc.
For the same reason that each thread will create a `WOLFSSL` object, each thread will also make calls to `wolfSSL_accept`, `wolfSSL_read`, and `wolfSSL_write`. The calls should be almost identical to the regular calls inside of `server-dtls.c`, but with minor changes.

*Important: don't forget to set the file descriptor as well*

- `continue` keywords should be changed to `return NULL`, as `ThreadHandler` does not loop
- If `wolfSSL_write` returns a negative number in `ThreadHandler`, there should be a `return NULL` instead of `return 1` (`ThreadHandler` is invoked differently from `AwaitDGram`.

 After making these changes and adding them to `ThreadHandler`, it now looks like this:
```c
void* ThreadHandler(void *input)
{
    /* pthread_detach call */
    pthread_detach( pthread_self() );

    /* Instance variables */
    threadArgs*     args = (threadArgs*) input;
    int             recvLen = 0;                /* Length of message received */
    int             activefd = args->activefd;  /* active file descriptor */
    int             msgLen = args->size;        /* Length of message */
    unsigned char   buff[msgLen];               /* Incoming message from client */
    char            back[] = "MESSAGE RECEIVED" /* Message sent to client */
    WOLFSSL*        ssl;                        /* SSL object */

    /* memcpy call */
    memcpy(buff, args->b, msgLen);

    /* miscellaneous standard server calls */

    /* Create the WOLFSSL Object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        cleanup = 1;
        return NULL;
    }

    wolfSSL_set_fd(ssl, activefd);

    /* Attempt to accept connection */
    if (wolfSSL_accept(ssl) != SSL_SUCCESS) {

        int e = wolfSSL_get_error(ssl, 0);

        printf("error = %d, %s\n", e, wolfSSL_ERR_reason_error_string(e));
        printf("SSL_accept failed.\n");
        return NULL;
    }
    /* Attempt to read a message from the client, check for errors twice */
    if ((recvLen = wolfSSL_read(ssl, buff, msgLen-1)) > 0) {
        printf("heard %d bytes\n", recvLen);

        buff[recvLen] = 0;
        printf("I heard this: \"%s\"\n", buff);
    }
    else if (recvLen < 0) {
        int readErr = wolfSSL_get_error(ssl, 0);
        if(readErr != SSL_ERROR_WANT_READ) {
            printf("SSL_read failed.\n");
            cleanup = 1;
            return NULL;
        }
    }
    /* Send reply to the client */
    if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0) {
        printf("wolfSSL_write fail.\n");
        cleanup = 1;
        return NULL;
    }
    else {
        printf("Sending reply.\n");
    }

    printf("reply sent \"%s\"\n", ack);

    /* Thread memory cleanup */
}
```

##### 3.1.1.3.6. Thread Memory Cleanup
To clean up the mess left behind when the client and server are finished with their business, we'll need to free/close the objects created at the beginning of the `ThreadHandler` method, and call a few subtle thread memory management calls. Below is a list of items that need to be freed, and our finished `ThreadHandler` code is immediately following.

- `ssl`
- `activefd`
- `input`
- The current thread

```c
void* ThreadHandler(void *input)
{
    /* pthread_detach call */
    pthread_detach( pthread_self() );

    /* Instance variables */
    threadArgs*     args = (threadArgs*) input;
    int             recvLen = 0;                /* Length of message received */
    int             activefd = args->activefd;  /* active file descriptor */
    int             msgLen = args->size;        /* Length of message */
    unsigned char   buff[msgLen];               /* Incoming message from client */
    char            back[] = "MESSAGE RECEIVED" /* Message sent to client */
    WOLFSSL*        ssl;                        /* SSL object */

    /* memcpy call */
    memcpy(buff, args->b, msgLen);

    /* miscellaneous standard server calls */

    /* Create the WOLFSSL Object */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        cleanup = 1;
        return NULL;
    }

    wolfSSL_set_fd(ssl, activefd);

    /* Attempt to accept connection */
    if (wolfSSL_accept(ssl) != SSL_SUCCESS) {

        int e = wolfSSL_get_error(ssl, 0);

        printf("error = %d, %s\n", e, wolfSSL_ERR_reason_error_string(e));
        printf("SSL_accept failed.\n");
        return NULL;
    }
    /* Attempt to read a message from the client, check for errors twice */
    if ((recvLen = wolfSSL_read(ssl, buff, msgLen-1)) > 0) {
        printf("heard %d bytes\n", recvLen);

        buff[recvLen] = 0;
        printf("I heard this: \"%s\"\n", buff);
    }
    else if (recvLen < 0) {
        int readErr = wolfSSL_get_error(ssl, 0);
        if(readErr != SSL_ERROR_WANT_READ) {
            printf("SSL_read failed.\n");
            cleanup = 1;
            return NULL;
        }
    }
    /* Send reply to the client */
    if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0) {
        printf("wolfSSL_write fail.\n");
        cleanup = 1;
        return NULL;
    }
    else {
        printf("Sending reply.\n");
    }

    printf("reply sent \"%s\"\n", ack);

    printf("Client left, return to idle state.\n"
           "Cleaning up & exiting thread.\n");

    /* Thread memory cleanup */
    wolfSSL_shutdown(ssl);  /* cleans up the ssl object */
    wolfSSL_free(ssl);
    close(activefd);        /* closes activefd (who'd have thought?) */
    free(input);            /* cleans up the input object */
    pthread_exit(input);    /* terminates the thread that it is called in */
}
```
And that's it - that is all that is required to create a multithreaded DTLS server.

#### 3.1.2. Review

There were several things added to a DTLS server to enable threading. Below is a concise list of all that was added and done to make a threaded DTLS server.
- Defined `threadArgs` struct with data about the current client and session
- Calling `pthread_create` and passing in client/session data with a defined `ThreadHandler` method.
- A method (ours was called `ThreadHandler`) to tell the threads how to behave
    - `ThreadHandler` used its own `WOLFSSL` objects and terminated itself at the end of the method

##  Chapter 4: Session Resumption with DTLS
### Section 1: by Alex Abrahamson

In the event that a connection between a client and a server is lost, it can be useful to resume with the previous session data. To implement this, all the work can be done on the client's side, since the client will be the one with direct access to their session info before a disconnect.

To utilize session resumption, 4 things have to be completed.

1. Storage of the previous session with a new WOLFSSL* object created with the same method call as the previous WOLFSSL* object
2. Memory cleanup and management of the previously interrupted session
3. Reconnect with the old session data
4. Cleanup

(*a quick note: to allow for infinite session resumptions, simply perform these actions inside of a loop.*)

#### 4.1.1. Storage of the Previous Session Information
When keeping track of previous sessions for resumption, the simplest and easiest way to do this is to create new objects and assign the old session data to them. An example of this is shown below.

```c
session = wolfSSL_get_session(ssl);
sslResume = wolfSSL_new(ctx);
```

The two variables above are the most important for session resumption.

- `session` is an object of type `WOLFSSL_SESSION*`
- `sslResume` is an object of type `WOLFSSL*`

###  4.1.2. Memory Cleanup and Management
Now it's time to clean up and clear out all the old objects. To clean up the memory, this can be done in standard fashion with calls to the functions `wolfSSL_shutdown` and `wolfSSL_free` - and don't forget to call `close` on the socket file descriptor.

 An example of this is shown below.

```c
wolfSSL_shutdown(ssl);
wolfSSL_free(ssl);
close(sockfd);
```

In addition, it is important to clear the memory holding the server address and reassign it. An example of this is shown below.

```c
memset(&servAddr, 0, sizeof(servAddr));
servAddr.sin_family = AF_INET;
servAddr.sin_port = htons(SERV_PORT);
if ( (inet_pton(AF_INET, host, &servAddr.sin_addr)) < 1) {
    printf("Error and/or invalid IP address");
    return 1;
}
```

###  4.1.3. Reconnect with Old Session Data
The reconnection is nearly identical to the way that a client will connect to a server the first time. There are calls to `wolfSSL_dtls_set_peer`, `socket`, `wolfSSL_set_fd`, and so on, but there are also calls to other methods.

All  the method calls needed with new variables and the distinct new method calls are listed below in the order that they need to be called.

1. `wolfSSL_dtls_set_peer(sslResume, &servAddr, sizeof(servAddr))`
 - This method call uses the old session data (`sslResume`) and with the same server address. It needs to be called after the `memset` call listed in section 2 (**Memory cleanup and management**).
2. `socket(AF_INET, SOCK_DGRAM, 0)` will need to be in an identical conditional as the prior call to `socket`.
3. `wolfSSL_set_fd(sslResume, sockfd)`
 - This method call uses the old session data (`sslResume`) and with the same socket file descriptor.
4. `wolfSSL_set_session(sslResume, session)`
 - This is a new method call. On failure, the return value is (you guessed it) an `SSL_FAILURE` code. On success, it will return (you could try out for jeopardy!) an `SSL_SUCCESS` code.
5. `wolfSSL_connect(sslResume)` will need to be called in an identical conditional as the prior call to `wolfSSL_connect`.
6. `wolfSSL_session_reused(sslResume)`
 - This method call returns the `resuming` value from inside the `sslResume` object.
 - This method call is used with a conditional to tell the user whether or not a session was reused. An example is shown below.
```c
if (wolfSSL_session_reused(sslResume)) {
    printf("Reused session ID\n");
} else {
    printf("Did not reuse session ID\n");
}
```
###  4.1.4. Cleanup
There isn't anything special about cleanup with reused sessions. All that needs to be done is to call `wolfSSL_shutdown` and `wolfssl_free` with the `sslResume` object.

Again, if wanted, this can all be isolated inside a loop that will break upon receiving a signal from the user. To find out more about signal handling, visit the wolfSSL [SSL Tutorial](https://wolfssl.com/wolfSSL/Docs-wolfssl-manual-11-ssl-tutorial.html), specifically section 11.12.

## CHAPTER 5: Convert Server and Client to Nonblocking
### Section 1: A nonblocking client
#### 5.1.1. A Note About Functions
If you compare this tutorial and the functions we discuss implementing to the available completed example code, you will notice these functions are missing from the example code. The functions have been combined into `main()` in order to allow the code to read linearly and be more understandable. For the purposes of this tutorial, you may still add the noted functions to achieve the same functionality. All of the functions are simply hidden within the main function. Most are commented with their original function names when they occur. This tutorial is in the process of being updated to match this more appropriately.

#### 5.1.2. Add New Headers to Top of File:
```c
# include <errno.h>    /* error checking */
# include <fcntl.h>    /* set file status flags */
```

#### 5.1.3. Add Enum Variables for Testing Functions
In both the server and client files, add the following before the first function:
```c
enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};
```

#### 5.1.4 Adding nonblocking DTLS connection functionality
From [Quora](https://www.quora.com/What-exactly-does-it-mean-for-a-web-server-to-be-blocking-versus-non-blocking),
> Many IO related system calls, like read(2), will block, that is not return, until there is activity.  A socket can be placed in "non-blocking mode" which means that these system calls will return immediately if there is nothing pending.

To make our client a nonblocking client, we need to add 2 things (they should come before we attempt to send a datagram to the server, as it would be much more difficult to send a datagram without having a connection!).

**1. Variables**

**2. A loop that iterates until an `SSL_SUCCESS` or error**
  - The loop will also have conditional statements inside that check the progress of the nonblocking code - they will be mainly used to determine the outcome of the loop and keep the user updated.

The only part of the functionality that is truly nonblocking is the selection of sockets. It will not wait until there is activity on the server side to return - it will return after a specified amount of time regardless of if it can connect or not.

Before adding anything else, there are 2 method calls to be added. In the `main()` method, add the following chunk **immediately** after the call to `wolfSSL_set_fd(ssl, sockfd)`:
```c
wolfSSL_dtls_set_using_nonblock(ssl, 1);
fcntl(sockfd, F_SETFL, O_NONBLOCK);
```
##### 5.1.4.1 - Variables
Almost all the variables that need to be added are `int` types. There are also `fd_set` and `timeval` variables. Instantiation at declaration is not necessary, as most of the variables will be reset at every iteration of the loop in mentioned above in **2**. In addition, some variables are also dependent upon other variables and their data in the `main()` method. **Figure 5...?** shows the variable declarations. They should be declared along with all other variables inside of the `main()` method.

**Figure 5.1**
```c
/* Variables used for nonblocking DTLS connect */
int                 ret;
int                 error;
int                 nb_sockfd;
int                 select_ret;
int                 currTimeout;

/* Variables used for nonblocking DTLS select */
int                 nfds;
int                 result;
fd_set              recvfds, errfds;
struct timeval      timeout;
```

Three of the variables used for nonblocking DTLS connect will be assigned prior to the loop. **Figure 5.2** shows where they are assigned.

**Figure 5.2**
```c
fcntl(sockfd, F_SETFL, O_NONBLOCK);

/*****************************************************************************/
/*                Connect using Nonblocking - DTLS version                   */
ret = wolfSSL_connect(ssl);             /* Connection test (should fail) */
error = wolfSSL_get_error(ssl, 0);      /* Find out why connect failed */
nb_sockfd = (int) wolfSSL_get_fd(ssl);  /* To protect the original socket */
```

1 variable from nonblocking DTLS connect and 2 from nonblocking DTLS select will be assigned at the beginning of each iteration.

2 variables from nonblocking DTLS select will be assigned in the middle of the loop.

The variable assignments will be shown when the loop is discussed in **Section 5.1.4.2** next.

##### 5.1.4.2 - A loop that iterates until an `SSL_SUCCESS` or error
The body of the nonblocking DTLS connection and functionality occur within a while loop that continues to loop while `ret` has not succeeded in a connection, and while `error` is at least an `SSL_ERROR_WANT_READ` or an `SSL_ERROR_WANT_WRITE`.

**Figure 5.3**
```c
/*****************************************************************************/
/*                Connect using Nonblocking - DTLS version                   */
ret = wolfSSL_connect(ssl);             /* Connection test (should fail) */
error = wolfSSL_get_error(ssl, 0);      /* Find out why connect failed */
nb_sockfd = (int) wolfSSL_get_fd(ssl);  /* To protect the original socket */

while (ret != SSL_SUCCESS &&
      (error = SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)) {

    /* Variables that reset every iteration */
    currTimeout = wolfSSL_dtls_get_current_timeout(ssl);
    nfds = nb_sockfd + 1;
    timeout = (struct timeval) { (currTimeout > 0) ? currTimeout : 0, 0};

    /* Keep the user updated */
    if (error == SSL_ERROR_WANT_READ) {
        printf("... client would read block.\n");
    } else {
        printf("... client would write block.\n");
    }

    /* nonblocking DTLS select variables that reset every iteration *
     * (recvfds, nb_sockfd and errfds must be manually reset)       */
    FD_ZERO(&recvfds);
    FD_SET(nb_sockfd, &recvfds);
    FD_ZERO(&errfds);
    FD_SET(nb_sockfd, &errfds);

    result = select(nfds, &recvfds, NULL, &errfds, &timeout);

    select_ret = TEST_SELECT_FAIL;

    if (result == 0) {
        select_ret = TEST_TIMEOUT;
    }
    else if (result > 0) {
        if (FD_ISSET(nb_sockfd, &recvfds)) {
            select_ret = TEST_RECV_READY;
        }
        else if (FD_ISSET(nb_sockfd, &errfds)) {
            select_ret = TEST_ERROR_READY;
        }
    }
    /* End nonblocking DTLS selection functionality */

    /* Determine the outcome of nonblocking selection */
    if (  select_ret == TEST_RECV_READY ||
          select_ret == TEST_ERROR_READY ) {
        ret = wolfSSL_connect(ssl);
        error = wolfSSL_get_error(ssl, 0);
    }
    else if (select_ret == TEST_TIMEOUT && !wolfSSL_dtls(ssl)) {
        error = 2;
    }
    else if (select_ret == TEST_TIMEOUT && wolfSSL_dtls(ssl) &&
          wolfSSL_dtls_got_timeout(ssl) >= 0) {
        error = 2;
    }
    else {
        error = SSL_FATAL_ERROR;
    }
}

/* Confirm that the loop exited properly */
if (ret != SSL_SUCCESS) {
    printf("SSL_connect failed with %d\n", ret);
}
/*                                                                           */
/*****************************************************************************/
```

#### 5.1.5 Datagram sending
Now the client needs to send a message to the server. If it didn't, then there would have been no point in connecting to the server.

The way that a datagram from a nonblocking DTLS client is sent is identical to the way that a datagram is sent from a regular DTLS client. Remember, what makes a DTLS client nonblocking is how it connects to a server, not how it sends its message.

The datagram sending code is included below for reference in **Figure 5.4.** It requires no special variable usage or extra conditionals/loops in a nonblocking client.

**Figure 5.4**
```c
/* Code for sending a datagram to the server */
int  n = 0;
char sendLine[MAXLINE], recvLine[MAXLINE - 1];

while (fgets(sendLine, MAXLINE, stdin) != NULL) {

    while  ( ( wolfSSL_write(ssl, sendLine, strlen(sendLine))) !=
          strlen(sendLine)) {
        printf("SSL_write failed");
    }

    while ( (n = wolfSSL_read(ssl, recvLine, sizeof(recvLine)-1)) <= 0) {
        int readErr = wolfSSL_get_error(ssl, 0);
        if(readErr != SSL_ERROR_WANT_READ) {
            printf("wolfSSL_read failed");
        }
    }

    recvLine[n] = '\0';
    fputs(recvLine, stdout);
}
```
### Section 2: A nonblocking DTLS server
#### 5.2.1 Add New Headers to Top of File:
```c
#include <errno.h>    /* error checking */
#include <fcntl.h>    /* set file status flags */
```
#### 5.2.2 Add Enum Variables for Testing Functions
In both the server and client files, add the following before the first function:
```c
enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};
```
#### 5.2.3 Handling signals
In the server file, it can be useful to get a user's input that tells the server to shut down. This is done with handling signals. To do this, we will need to have a static variable `cleanup` that tells the server when the signal has been passed, a method that will modify `cleanup`, and code that states this method is what needs to be executed when the server is passed a signal.
##### 5.2.3.1 Static variable `cleanup`
This variable `cleanup` is just a global static integer. It does not need to be initialized at declaration. Be sure to include in your while loop that you wish to loop until the original condition is true **or** `cleanup` is equal to 1.
##### 5.2.3.2 Defining a method to handle signals
To let the program know in an simple way that a signal has passed, all that needs to be done is writing a method that will reassign `cleanup`.

The method needs to take in a `const int` type, and if desired, you can also add a print statement that tells the user what has happened. An example of a `sig_handler` method is shown below in **Figure 5.5**.

**Figure 5.5**
```c
/* For handling ^C interrupts or other signals */
void sig_handler(const int sig)
{
    printf("\nSIGINT %d handled\n", sig);
    cleanup = 1;
    return;
}
```
##### 5.2.3.3 Telling the program to use `sig_handler()`
If we just leave the server with the `sig_handler` definition and it looping until cleanup is changed, nothing will actually change when a user passes a signal to the server. To fix this, add the code in **Figure 5.6** immediately after creating the variables in `main()`.

**Figure 5.6**
```c
/*Code for handling signals */
struct sigaction act, oact;
act.sa_handler = sig_handler;
sigemptyset(&act.sa_mask);
act.sa_flags = 0;
sigaction(SIGINT, &act, &oact);
```
#### 5.2.4 Adding nonblocking DTLS connection functionality
To make our server a nonblocking server, we need to add 2 things. They are the same as the two things we had to add for our client - variables and a while loop.

**1. Variables**

**2. A while loop that loops while cleanup is not 1 and the original conditions are true**

Don't forget that immediately after the call to `wolfSSL_set_fd()`, we need to make a call `wolfSSL_dtls_set_using_nonblock()`. The call to this method is shown below.
```c
wolfSSL_dtls_set_using_nonblock(ssl, 1);
```
##### 5.2.4.1 Variables
As with our client, there are several variables needed for our nonblocking DTLS server. **Figure 5.7** lists them.

**Figure 5.7**
```c
/* DTLS set nonblocking flag */
int           flags = fcntl(*(&listenfd), F_GETFL, 0);

/* NonBlockingSSL_Accept variables */
int           ret;
int           select_ret;
int           currTimeout;
int           error;
int           result;
int           nfds;
fd_set        recvfds, errfds;
struct        timeval timeout;

/* udp-read-connect variables */
int           bytesRecvd;
unsigned char b[MSGLEN];
struct        sockaddr_in cliAddr;
socklen_t     clilen;
```
##### 5.2.4.2 While loop
As with the client, the while loop has variables that are assigned prior to, at the beginning of, and in the center of the loop.

For brevity, they will not all be pointed out and explained - the code will be displayed in **Figure 5.8**, and all that is new will have descriptive comments that explain the differences.

**Figure 5.8**
```c
/*****************************************************************************/
/*                           AwaitDatagram code                              */
/* This code will loop until a ^C (SIGINT 2) is passed by the user */
cont = 0;
while (cleanup != 1) {

    clilen = sizeof(cliAddr);
    timeout.tv_sec = (currTimeout > 0) ? currTimeout : 0;

    /* Create a UDP/IP socket */
    if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        printf("Cannot create socket.\n");
        cont = 1;
    }

    printf("Socket allocated\n");

    /* DTLS set nonblocking */
    if (flags < 0) {
        printf("fcntl get failed");
        cleanup = 1;
    }
    flags = fcntl(*(&listenfd), F_SETFL, flags | O_NONBLOCK);
    if (flags < 0) {
        printf("fcntl set failed.\n");
        cleanup = 1;
    }

    /* Clear servAddr each loop */
    memset((char *)&servAddr, 0, sizeof(servAddr));

    /* host-to-network-long conversion (htonl) */
    /* host-to-network-short conversion (htons) */
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(SERV_PORT);

    /* Eliminate socket already in use error */
    res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
    if (res < 0) {
        printf("Setsockopt SO_REUSEADDR failed.\n");
        cont = 1;
    }

    /*Bind Socket*/
    if (bind(listenfd,
                (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        printf("Bind failed.\n");
        cont = 1;
    }

    printf("Awaiting client connection on port %d\n", SERV_PORT);

    /* UDP-read-connect */
    do {
        if (cleanup == 1) {
            cont = 1;
            break;
        }
        bytesRecvd = (int)recvfrom(listenfd, (char*)b, sizeof(b), MSG_PEEK,
            (struct sockaddr*)&cliAddr, &clilen);
    } while (bytesRecvd <= 0);

    if (bytesRecvd > 0) {
        if (connect(listenfd, (const struct sockaddr*)&cliAddr,
                    sizeof(cliAddr)) != 0) {
            printf("udp connect failed.\n");
        }
    }
    else {
        printf("recvfrom failed.\n");
    }

    printf("Connected!\n");
    /* ensure b is empty upon each call */
    memset(&b, 0, sizeof(b));
    clientfd = listenfd;

    /* Create the WOLFSSL Object */
    if (( ssl = wolfSSL_new(ctx)) == NULL) {
        printf("wolfSSL_new error.\n");
        cont = 1;
    }

    /* set clilen to |cliAddr| */
    printf("Connected!\n");

    /* set the/ session ssl to client connection port */
    wolfSSL_set_fd(ssl, clientfd);

    wolfSSL_dtls_set_using_nonblock(ssl, 1);

/*****************************************************************************/
/*                      NonBlockingDTLS_Connect code                         */

    /* listenfd states where to listen ()
    ret = wolfSSL_accept(ssl);
    currTimeout = 1;
    error = wolfSSL_get_error(ssl, 0);
    listenfd = (int) wolfSSL_get_fd(ssl);
    nfds = listenfd + 1;

    /* Loop until there has been a successful connection *
     * or until there has been a signal                  */
    while (cleanup != 1 && (ret != SSL_SUCCESS &&
                (error == SSL_ERROR_WANT_READ ||
                 error == SSL_ERROR_WANT_WRITE))) {

        if (cleanup == 1) {
            wolfSSL_free(ssl);
            wolfSSL_shutdown(ssl);
            break;
        }

        /* Keep the user updated */
        if (error == SSL_ERROR_WANT_READ)
            printf("... server would read block\n");
        else
            printf("... server would write block\n");

        currTimeout = wolfSSL_dtls_get_current_timeout(ssl);

        FD_ZERO(&recvfds);
        FD_SET(listenfd, &recvfds);
        FD_ZERO(&errfds);
        FD_SET(listenfd, &errfds);

        /* This is where the term 'nonblocking' comes into use */
        result = select(nfds, &recvfds, NULL, &errfds, &timeout);

        if (result == 0) {
            select_ret = TEST_TIMEOUT;
        }
        else if (result > 0) {
            if (FD_ISSET(listenfd, &recvfds)) {
                select_ret = TEST_RECV_READY;
            }
            else if (FD_ISSET(listenfd, &errfds)) {
                select_ret = TEST_ERROR_READY;
            }
        }
        else {
            select_ret = TEST_SELECT_FAIL;
        }

        if ((select_ret == TEST_RECV_READY) ||
            (select_ret == TEST_ERROR_READY)) {
            ret = wolfSSL_accept(ssl);
            error = wolfSSL_get_error(ssl, 0);
        }
        else if (select_ret == TEST_TIMEOUT && !wolfSSL_dtls(ssl)) {
            error = SSL_ERROR_WANT_READ;
        }
        else if (select_ret == TEST_TIMEOUT && wolfSSL_dtls(ssl) &&
                wolfSSL_dtls_got_timeout(ssl) >= 0) {
            error = SSL_ERROR_WANT_READ;
        }
        else {
            error = SSL_FATAL_ERROR;
        }
    }
    if (ret != SSL_SUCCESS) {
        printf("SSL_accept failed with %d.\n", ret);
        cont = 1;
    }
    else {
        cont = 0;
    }

    if (cont != 0) {
        printf("NonBlockingSSL_Accept failed.\n");
        cont = 1;
    }
/*                    end NonBlockingDTLS_Connect code                       */
/*****************************************************************************/
    /* Begin: Reply to the client */
    recvLen = wolfSSL_read(ssl, buff, sizeof(buff)-1);

    /* Begin do-while read */
    do {
        if (cleanup == 1) {
            memset(buff, 0, sizeof(buff));
            break;
        }
        if (recvLen < 0) {
            readWriteErr = wolfSSL_get_error(ssl, 0);
            if (readWriteErr != SSL_ERROR_WANT_READ) {
                printf("Read Error, error was: %d.\n", readWriteErr);
                cleanup = 1;
            }
            else {
                recvLen = wolfSSL_read(ssl, buff, sizeof(buff)-1);
            }
        }
    } while (readWriteErr == SSL_ERROR_WANT_READ &&
                                     recvLen < 0 &&
                                     cleanup != 1);
    /* End do-while read */

    if (recvLen > 0) {
        buff[recvLen] = 0;
        printf("I heard this:\"%s\"\n", buff);
    }
    else {
        printf("Connection Timed Out.\n");
    }

    /* Begin do-while write */
    do {
        if (cleanup == 1) {
            memset(&buff, 0, sizeof(buff));
            break;
        }
        readWriteErr = wolfSSL_get_error(ssl, 0);
        if (wolfSSL_write(ssl, ack, sizeof(ack)) < 0) {
            printf("Write error.\n");
            cleanup = 1;
        }
        printf("Reply sent:\"%s\"\n", ack);
    } while(readWriteErr == SSL_ERROR_WANT_WRITE && cleanup != 1);
    /* End do-while write */

    /* free allocated memory */
    memset(buff, 0, sizeof(buff));
    wolfSSL_free(ssl);

    /* End: Reply to the Client */
}
/*                          End await datagram code                          */
/*****************************************************************************/

/* End of the main method */
if (cont == 1 || cleanup == 1) {
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
}

return 0;
```

The code above was taken directly from the DTLS server nonblocking file. 

Be sure to keep in mind that the `AwaitDatagram` code is essentially one large loop that will attempt to listen for a client (in a nonblocking fashion) at every iteration, and will close the loop upon a signal passed by the user.

#### 5.2.5 Final note
And that's it! The server has been made into a nonblocking server, and the client has been made into a nonblocking client.

#### REFERENCES:

1. Paul Krzyzanowski, “Programming with UDP sockets”, Copyright 2003-2014, PK.ORG
2. The Open Group, “setsockopt - set the socket options”, Copyright © 1997, The Single UNIX ® Specification, Version 2
3. https://en.wikipedia.org/wiki/POSIX_Threads
4. https://www.quora.com/What-exactly-does-it-mean-for-a-web-server-to-be-blocking-versus-non-blocking 
