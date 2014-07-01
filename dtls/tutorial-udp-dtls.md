##CHAPTER 1: A Simple UDP SERVER & Client

###Section 1: By Kaleb Himes

####1.1. Introduction and description of UDP
What is UDP? User Datagram Protocol (UDP) is a core member of Internet Protocol Suite (IPS). Messages sent using UDP are called datagrams. A client can send messages to other hosts (clients or servers that have host capabilities) on an Internet Protocol (IP) network without any prior communications to set up special transmissions channels or data paths. A formal definition can be found in Request for Comments (RFC) 768 (http://tools.ietf.org/html/rfc768).

UDP uses a minimum of protocol mechanisms in that it requires no handshake. Rather UDP uses a checksum for data integrity and port numbers for addressing different functions at the source and destination of the datagram.

UDP is ideal for time conservation. The lack of having to re-transmit messages (if packets are lost) makes it suitable for real-time applications such as voice over IP or live streaming.

CONS: UDP provides no guarantees to the upper layer protocol for message delivery and the UDP protocol layer retains no backup of UDP messages once sent. Therefore messages are non-recoverable once sent.

UDP header consists of 4 fields, each of which is 16 bits.
Field 1: Contains information about the port from which the message originated.
Field 2: Contains information about the destination port.
Field 3: Defines the length in bytes of the UDP header and data. Allows for datagram of
maximum size 65,535 bytes to be defined. If larger, set to 0. (IPv6 will allow larger).
Field 4: Contains a Checksum for security purposes. Contains some checksum of the header and data.

CHECKSUM
All 16 bit words are summed using “one`s complement arithmetic”. The sum is then “one`s complemented” and this is the value placed in the UDP checksum field.

Figure 1.1 Check Sum Example
EXAMPLE: you have two 16 bit words as follows :

ONE`s COMPLEMENT OF WORD1:     1 0 0 1 1 1 1 0 0 1 1 0 0 0 1 0
ONE`s COMPLEMENT OF WORD2:     0 1 0 1 0 0 0 1 0 1 0 0 1 1 0 1

SUM:     1 1 1 0 1 1 1 1 1 0 1 0 1 1 1 1

ONE`s COMPLEMENT OF SUM:   0 0 0 1 0 0 0 0 0 1 0 1 0 0 0 0

The final value would be placed in the Checksum Field.


####1.2. Creating a UDP/IP Server
There are five initial steps to creating a UDP/IP Server.

    1. Create the socket
    2. Identify the socket (IE give it a name)
        <Begin a loop>
    3. On the server wait for a message
    4. Send a response to the client once message is received
    5. Close the socket (in our case, just return to looking for packets arriving).
        <End of loop>

####1.3.  STEP 1: CREATE THE SOCKET

    A socket is created with the socket system call.

    Figure 1.2 Create a Socket
    int sockfd = socket(domain, type, protocol);

    Let`s briefly discuss those parameters domain, type, and protocol.

    1. Domain
    The domain can also be referred to as the address family. It is the communication domain in which the socket should be created. Below you will see some of the examples domains or address families that we  could work with. At the end will be a description of what we will choose specifically for a UDP server.

    ```c
    AF_INET:     Internet Protocol (IP)
    AF_INET6:    IP version 6 (IPv6)
    AF_UNIX:     local channel, similar to pipes
    AF_ISO:     “In Search Of” (ISO) protocols
    AF_NS:    Xerox Network Systems protocols
```
    2. Type
    This is the type of service we will be providing with our UDP server. This is selected based on the requirements of the application, and will likely aid in determining which Domain (above) you will select ultimately.
```c
    SOCK_STREAM:     a virtual circuit service
    SOCK_DGRAM:     a datagram service
    SOCK_RAW:     a direct IP service
```
    3. Protocol
    A protocol supports the sockets operation. This parameter is optional but is helpful in cases where the domain (family) supports multiple protocols. In these cases we can specify which protocol to use for said family. If the family supports only one type of protocol this value will be zero.

    For this tutorial we want to select domain (domain = AF_NET) and the datagram service (type = SOCK_DGRAM). There is only one form of datagram service therefore we do not need to specify a protocol for a UDP/IP server  (protocol = 0).
    Defined:

    Figure 1.3 Setting Protocol
    
   ```c
    #include <sys/socket.h>
    …
    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("cannot create socket");
        return 0;
    }
    printf("created socket: descriptor=%d\n", sockfd);```



####1.4. STEP 2: IDENTIFY/NAME THE SOCKET
By naming the socket we are assigning a transport address to the socket (I.E. a port number in IP networking). This is more commonly referred to as “binding” an address. The bind system call is used to do this. In other words we are giving our server a unique address so that communication with our server can be established much like a person with a mailing address. They can receive communiques (letters) via their mailbox. Our server can do the same once it has a “bound” socket.

The transport address is defined in the socket address structure. Since sockets can receive and send data  using a myriad of communication interfaces, the interface is very general. Rather than accepting a port number as a parameter, it will look for a “sockaddr” (short for socket address)structure whose format is based off the address family we chose.

`#include <sys/socket.h>` contains the relevant “bind” call we will need. Since we already used it in STEP 1 we do not need to include it a second time.

Figure 1.4 Bind the Socket
```c
    int
    bind(int socket, const struct sockaddr *address, socklen_t address_len);```


The first parameter “socket” is the socket we created in STEP 1 (sockfd)
    The second parameter (sockaddr) will allow the operating system (OS) to read the first couple bytes that identify the address family. For UDP/IP networking we will use struct sockaddr_in, which is defined in the netinet/in.h system library.
    Defined:


    Figure 1.5 Explain address family
    ```c
    struct sockaddr_in{
        __uint8_t        sin_len;
        sa_family_t      sin_family;
        in_port_t        sin_port;
        struct in_addr   sin_addr;
        char             sin_zero[8];
    };```


NOTE: this code will not be found in our example server. It is imported with the following call (figure 1.6).

Figure 1.6 
`#include <netinet/in.h>`

Before calling bind, we need to fill this struct. The three key parts we need to set are:

    1. sin_family
The address family we used in STEP 1 (AF_INET).

    2. sin_port
The port number (transport address). This can either be explicitly declared, or you can allow the OS to assign one. Since we are creating a server, ideally we would want to explicitly declare a well known port so that clients know where to address their messages. However for this particular tutorial we will use the generic 11111 (five ones). This will be defined directly beneath the include section of our code.(figure 1.1.7)

    Figure 1.7
`#define SERV_PORT  11111`

    We can then call SERV_PORT where it is needed and if you, the client, are already using port 11111 for any particular reason, you can then easily redefine it as needed for this tutorial. Additionally if you use #define SERV_PORT 0, your system will use any available port.

    3. sin_addr
    The address for our socket (your server machines IP address). With UDP/IP our server will have one IP address for each network interface. Since the address can vary based on transport methods and we are using a client computer to simulate a server, we will use the INADDR_ANY.

    4. Descriptions of number conversions utilized in Networking
    4.1 “htons”
    host to network - short : convert a number into a 16-bit network representation. This is commonly used to store a port number into a sockaddr structure.



    4.2 “htonl”
    host to network - long : convert a number into a 32-bit network representation. This is commonly used to store an IP address into a sockaddr structure.

    4.3 “ntohs”
    network to host - short : convert a 16-bit number from a network representation into the local processor`s format. This is commonly used to read a port number from a sockaddr structure.

    4.4 “ntohl”
    network to host - long : convert a 32-bit number from a network representation into the local processor`s format. This is commonly used to read an IP address from a sockaddr structure.

    Using any of the above 4.4 macros will guarantee that your code remains portable regardless of the architecture you use in compilation.

####1.5. <BEGIN LOOP>:
    WAIT FOR A MESSAGE
    Later when we layer on DTLS our server will set up a socket for listening via the “listen” system call. The server would then call “accept” upon hearing a request to communicate, and then wait for a connection to be established. UDP however in it`s base for is connectionless. So our server, as of right now, is capable of listening for a message purely due to the fact that it has a socket! We use recvfrom system call to wait for an incoming datagram on a specific transport address (IP address, and port number).
    The recvfrom call is included with the #include <sys/socket.h> therefore we do not need to include this library again since we already included it in STEP 1.
    Defined:

    Figure 1.8 “recvfrom”
int recvfrom(int socket, void* restrict buffer, size_t length,
        int socklen_t *restrict *src_len)

    5.1 PARAMETERS DEFINED
    5.1.1 “ int socket ”
    The first parameter “socket” is the socket we created and bound in STEPS 1 & 2. The port number assigned to that socket via the “bind” tells us what port recvfrom will “watch” while awaiting incoming data transmissions.
    5.1.2 “ void* restrict buffer ”
    The incoming data will be placed into memory at buffer.
    5.1.3 “ size_t length “
    No more than length bytes will be transferred (that`s the size of your buffer).
    5.1.4 “ int socklen_t *restrict *src_len “
    For this tutorial we can ignore this last flags. However this parameter will allow us to “peek” at an incoming message without removing it from the queue or block until the request is fully satisfied. To ignore these flags, simply place a zero in as the parameter. See the man page for recvfrom to see an  in-depth description of the last parameter.
    Defined:

    Figure 1.9 Looping Receive
    `for (;;) {
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
    }`

####1.6. REPLY TO MESSAGE

Now we are able to receive messages from our clients but how do we let clients know their messages are being received? We have no connection to the server and we don’t know their IP address. Fortunately the recvfrom call gave us the address, and it was placed in remaddr:
Defined:

Figure 1.10
`recvlen = recvfrom(sockfd, buf, MSGLEN, 0, (struct sockaddr *)&cliaddr, &addrlen);`

The server can use that address in sendto and send a message back to the recipient’s address.
Defined:

    Figure 1.11
`sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&cliaddr, addrlen)`

####1.7. Close the socket
    This step is not necessary for our examples server however can easily be accomplished with a call to close().

    Figure 1.12
    `close(sockfd);`

    This concludes the simple UDP server portion of Chapter 1. Section 2 will now cover a Simple UDP Client.

    Section 2: By Leah Thompson

####2.1 UDP(User Datagram Protocol) Definitions:
        No connection to create and maintain
        More control over when data is sent
        No error recovery
        No compensation for lost packets
        Packets may arrive out of order
        No congestion control
        Overall, UDP is lightweight but unreliable. Some applications where UDP is used include DNS, NFS, and SNMP.

####2.2 Create Basic UDP Client:
    Create a function to send and receive data
    This function will send a message to the server and then loop back. The function does not return anything and takes in 4 objects: the input variable name, a socket, a pointer to a socket address structure, and a length for the address.

    Within this function, we will read in user input (fgets) from the client and loop while this input is valid. This loop will send the input to the server using the sendto() function. It will then read back the server’s echo with recvfrom() and print this echo(fputs). Our function:

    Figure 1.13        
    `void DatagramClient (FILE* clientInput, CYASSL* ssl) {

        int  n = 0;
        char sendLine[MAXLINE], recvLine[MAXLINE - 1];

        fgets(sendLine, MAXLINE, clientInput);

        if ( ( CyaSSL_write(ssl, sendLine, strlen(sendLine))) !=  
                strlen(sendLine))
            err_sys("SSL_write failed");

        n = CyaSSL_read(ssl, recvLine, sizeof(recvLine)-1); 
        if (n < 0) {
            int readErr = CyaSSL_get_error(ssl, 0);
            if(readErr != SSL_ERROR_WANT_READ)
                err_sys("CyaSSL_read failed");
        }

        recvLine[n] = '\0';
        fputs(recvLine, stdout);
    }`
**This function can be accomplished within main without creating an additional function. 

####2.3. Main Function
    #####2.3.1. Create a socket
The socket should be of type int in the form:


Figure 1.14    
`int yourSocket = socket(domain, type, protocol);`
Example Code:

    Figure 1.15        
`if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    err_sys("cannot create a socket.");`
    This method checks for a socket creation error, a good idea when setting up any socket implementation. Include the socket header:

    Figure 1.16        
`#include <sys/socket.h>`
    domain: the address family that is used for the socket you created, typically the internet protocol address AF_INET is used here. 
    type: a datagram socket, in this case we will use SOCK_DGRAM which is in the IPv4 protocol.
    protocol: we use 0 because there is only one type of datagram service.

#####2.3.2. Set up the server
    Create a socket address structure. Typically declared as:

    Figure 1.17        
`struct sockaddr_in servaddr;`

    This struct is contained in the header:

        Figure 1.18        
`#include <netinet/in.h>`

        This socket address structure will then be initialized to 0 using the bzero() or memset() functions. It will be filled with the IP address and port number of the server and then passed to the function you will create next. Here are the assignments to the servaddr:

        sin_family: the address family we used when setting up the socket, in our case, AF_INET.
        sin_port: the port number. This can be assigned or you can allow the operating system to assign one. Specifying the port to 0 allows the OS to pick any available port number.
        sin_addr: the address for the socket(i.e., your machine’s IP address).

        Example code:

        Figure 1.19        
       `bzero(&servAddr, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(SERV_PORT);
        inet_pton(AF_INET, argv[1], &servAddr.sin_addr);`


##CHAPTER 2: 
        Layering DTLS onto Simple Server and Client.

###Section 1: Kaleb
####1.1. New imports
        We will begin by adding the following libraries to pull from.

        Figure 2.1
`#include <cyassl/ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>`

####1.2. Increase MSGLEN
        Next change the size of our MSGLEN to 4096 to be more universal. This step is unnecessary if you’re testing against the client.c located in cyassl/examples/client as it will only send a message of length 14 or so but why not be able to handle a little user input if we want to test against a friends client or something!

####1.3. Shifting Variables, Making new Methods
#####1.3.1 Move sockaddr_in’s
        Now move our structs of type sockaddr_in so they are within scope of the entire program. We do this in preparation for the next step which will be to bust our client handling out of main. 

#####1.3.2 Create Signal Handler
        Additionally we will create a static int called cleanup here. This variable will be our signal to run CyaSSL_cleanup(); which will free the CyaSSL libraries and all allocated memory at the end of our program.

#####1.3.3 Create ctx and sig_handler Method
        Now we declare a CYASSL_CTX pointer and call it “ctx” for simplicity, and declare a void sig_handler method that takes a constant integer as an argument. 

#####1.3.4 Declare AwaitDGram()
    Finally we will declare a method AwaitDGram(). We will break our client handling out of main() and handle those connection in our new method. This is in preparation for Chapter 3 where we will be handling multiple client connections simultaneously.Your variable section should now look something like Figure 2.2:



    Figure 2.2
`#includes here...
    CYASSL_CTX* ctx;
    static int cleanup;                 /* To handle shutdown */
    struct sockaddr_in servaddr;        /* our server's address */
    struct sockaddr_in cliaddr;         /* the client's address */

    void AwaitDGram();                  /* Separate out Handling Datagrams */
    void sig_handler(const int sig);`

####1.4. Break Client Connection out of Main
#####1.4.1 Write Skeleton
    Just below sig_handler insert the following lines:

    Figure 2.3
 `void AwaitDGram()
     {
         
             }`



#####1.4.2 Move Variables from main()
    Literally cut and paste variable section from main() into our skeleton. Additionally add the variable “char ack”. This will be a reply message that we send to our clients. Ack is short for “Acknowledge”. So our clients have some sort of feedback letting them know that we received their communication successfully. See section 2.1.7.4 to see how ack is used in the code.

#####1.4.3 Remove un-needed Variables and Move all Declarations Together
    We will no longer refer to the open socket as “sockfd”, instead we will now call it “listenfd”. This reminds us that the socket is just listening for packets to arrive and we are not actually handling those packets immediately like we did in Chapter 1. We will want to confirm our client is encrypting their data with an acceptable cypher suite prior to opening a communication channel with them.
    With this change we will also rename “addrlen” to “clilen” to remind us that this socklen_t is the length of the clients address and not to be confused with our socket’s address. We will no longer assign the length of “clilen” upon declaration either. That will be handled later in our program. Remove msgnum altogether. Take note: “recvlen” is being declared here however is not used until we have verified our client is encrypting with DTLS version 1.2. See sub-section 2.1.7.4 to see “recvlen” used in code. Our variable section should now look something like this:

    Figure 2.4
`void AwaitDGram()
{
    int                  on = 1;
    int                 res = 1;
    int              connfd = 0; 
    int             recvlen = 0;    /* length of message */
    int            listenfd = 0;    /* Initialize our socket */
    CYASSL* ssl =          NULL;
    socklen_t            clilen;
    socklen_t len =  sizeof(on);
    unsigned char       b[1500];    /* watch for incoming messages */
    char           buff[MSGLEN];    /* the incoming message */
    char ack[] = "I hear you fashizzle!\n";
}`


#####1.4.4 Loop shift
        With the layering on of dtls we will need to re-allocate our socket and re-bind our socket for each client connection. Since we will need to free up all memory allocated to handle these connections and additional security our loop will now change to a “while” loop instead of a for loop. We will loop on the condition that cleanup is not equal to 1. If cleanup equals 1 we will run CyaSSL_cleanup() remember?
        So while not 1 we will keep our socket open and listening for packets to arrive!
        Start a while loop just below your variable declarations in method “AwaitDGram”. 

#####1.4.5 Move main() body into AwaitDGram()
                Now cut and paste all remaining code from main() into the while loop you just made, and delete the beginning of the “for” loop. Your while loop should now look like this: 
                NOTE: (ignore the line numbering DO NOT worry about the line numbers on the left as I am pulling straight from my old code just like you would be. The line numbers are irrelevant)

                Figure 2.6
                  `while (cleanup != 1) {  
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
                     
                     
                    (line deleted)
                    (line deleted)
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
                     
                    sprintf(buf, "Message #%d received\n", msgnum++);
                    printf("reply sent \"%s\"\n", buf);
                     
                    if (sendto(sockfd, buf, strlen(buf), 0,
                                        (struct sockaddr *)&cliaddr, addrlen) < 0)
                        perror("sendto");
                     
                    /* continues to loop, use "Ctrl+C" to terminate listening */
                    }
                }`
    Your main() should now just be an empty shell ready for the next step.

####1.5 New “main()”
#####1.5.1 Signal handlers for program termination
        Since our program will be running in an infinite loop just listening for clients we will need a way to terminate the program if necessary. For this we will need some signal action variables that will send a kill signal to our sig_handler method that we created at the beginning of chapter 2.

        Figure 2.7
    `struct sigaction    act, oact;  /* structures for signal handling */

    /* Some comment */
    act.sa_handler = sig_handler;                                
    sigemptyset(&act.sa_mask);   
    act.sa_flags = 0;            
    sigaction(SIGINT, &act, &oact);`

#####1.5.2 If Defined, turn on CyaSSL Debugging
        This is pretty self-explanatory.

        Figure 2.8
      `CyaSSL_Debugging_ON();`

#####1.5.3 Initialize CyaSSL, Load Certificates and Keys 
        In order for these to load properly you will need to place a copy of the “certs” file one directory above your current working directory. You can find a copy of the “certs” file in cyassl home directory. Simply copy and paste this file into the directory one up from your working directory, or change the file path in the code to search your cyassl home directory for the certs file.
        Figure 2.9
        211     CyaSSL_Init();                      /* Initialize CyaSSL */
    212                                                                
        213     /* Set ctx to DTLS 1.2 */                                  
        214     if ( (ctx = CyaSSL_CTX_new(CyaDTLSv1_2_server_method())) == NULL){
            215         fprintf(stderr, "CyaSSL_CTX_new error.\n");                   
            216         exit(EXIT_FAILURE);                                           
            217     }                                              
    218     /* Load CA certificates */
        219     if (CyaSSL_CTX_load_verify_locations(ctx,"../certs/ca-cert.pem",0) !=
                220             SSL_SUCCESS) {                                               
            221         fprintf(stderr, "Error loading ../certs/ca-cert.pem, "
                    222                 "please check the file.\n");
            223         exit(EXIT_FAILURE);
            224     }
    225     /* Load server certificates */
        226     if (CyaSSL_CTX_use_certificate_file(ctx,"../certs/server-cert.pem",
                    227                 SSL_FILETYPE_PEM) != SSL_SUCCESS) {
            228         fprintf(stderr, "Error loading ../certs/server-cert.pem, "
                    229                 "please check the file.\n");
            230         exit(EXIT_FAILURE);
            231     }
    232     /* Load server Keys */
        233     if (CyaSSL_CTX_use_PrivateKey_file(ctx,"../certs/server-key.pem",
                    234                 SSL_FILETYPE_PEM) != SSL_SUCCESS) {
            235         fprintf(stderr, "Error loading ../certs/server-key.pem, "
                    236                 "please check the file.\n");
            237         exit(EXIT_FAILURE);
            238     }

    1.5.4 Call AwaitDGram() and add cleanup conditional
        To finish our main() method we will call our method that handles client connections and add a conditional statement that will free up any allocated memory at the termination of our program. Last, add a return method for main().

        Figure 2.10
        240     AwaitDGram();
    241     if (cleanup == 1)
        242         CyaSSL_CTX_free(ctx);
    243     return 0;

    1.6 Quick recap
        So we’ve loaded all the certificates and keys we will need to encrypt any and all communications sent between our server and client. This encryption will be of type DTLS version 1.2 as seen on line 214 of figure 2.1.5.3, CyaDTLSv1_2_server_method(). In order for a client to now talk to our DTLS encrypted server they themselves will have to have certificates to verify our encryption, accept our key, and perform a DTLS handshake. See section 2 of this chapter for a tutorial on encrypting a client with DTLS version 1.2. 

        1.7 Adding DTLS to AwaitDGram()
        1.7.1 Avoid Socket in Use Error
        Our client handling is now running in a while loop, so it will continue to listen for clients even after a client has communicated with us and the closed their port. Our program will re-allocate that socket, rebind that socket and continue to await the arrival of more datagrams from that same or different clients. Our first step to avoid potential errors with our program will be to avoid “socket already in use” errors. Initialize a two dummy integers, “res” and “on”. set both equal to “1”. Then initialize a struct of type socklen_t (same as our clilen for getting the length of the clients address) call it “len” and set it equal to the size of “on”. We will use these variables to set the socket options to avoid that error.

        Figure 2.11
        Our code:                                       (socket)        (level) (option_name)(option_value) (option_len)
        |                   |                    |                 |             /
        |                   |                    |                 |           /        
        V                 V                  V               V        V
        88         res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);



    What it does:
        “int setsockopt(int socket, int level, int option_name, const void*option_value, socklen_t option_len);

    The setsockopt() function sets the option specified by the option_name argument, at the protocol level specified by the level argument, to the value pointed to by the option_value argument for the socket associated with the file descriptor specified by the socket argument.
        The level argument specifies the protocol level at which the option resides. To set options at the socket level, specify the level argument as SOL_SOCKET. To set options at other levels, supply the appropriate protocol number for the protocol controlling the option. For example, to indicate that an option will be interpreted by the TCP (Transport Control Protocol), set level to the protocol number of TCP, as defined in the <netinet/in.h> header, or as determined by using” [2]
        We will also check to ensure that the method did not fail as if it did it we would think we were listening for clients when in reality our server would not be able to receive any datagrams on that socket and clients would be getting rejected without our knowledge.

        Figure 2.12
        84         /* Eliminate socket already in use error */
        85         int res = 1;
    86         int on = 1;
    87         socklen_t len = sizeof(on);
    88         res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
    89         if (res < 0) {
        90             printf("Setsockopt SO_REUSEADDR failed.\n");
        91             cleanup = 1;
        92         }
    This code error avoidance code will be inserted just after we finish our host to network short conversion of SERV_PORT and just before we bind the socket. 

        NOTE: We have now changed our error handling from:
        perror(“some message”); and return 0; 
to: 
    printf(“some message”); and cleanup = 1;? 
        All active error situations will now change to match this format and all future error situations will be of the same format. This will ensure that all memory is freed at the termination of our program.

        1.7.2 Await Datagram arrival
        Here is where we will now set clilen = sizeof(cliaddr); as well. We will declare a unsigned char that will behave as buff does. Buff will now be used by “ssl” to read client messages and the new buff (we will just call it “b”) is for one purpose only. To “peek” at any incoming messages. We will not actually read those messages until later on. Right now we just want to see if there is a message waiting to be read or not. We must do this on a UDP server because no actual “connection” takes place… perseh. Packets arrive or they don’t, that’s how UDP works. Then we will perform some error handling on the fact that there are Datagrams waiting or there aren’t. If none are waiting we will want our while loop to cycle and continue to await packet arrivals. If there are packets sitting there then we want to know about it.

        Figure 2.13
        /* set clilen to |cliaddr| */
        104         clilen =    sizeof(cliaddr); /* will be moved to the variable section later */
    105         unsigned char       b[1500]; /* will be moved to the variable section later */
    106         int              connfd = 0; /* will be moved to the variable section later */
    107 
        108 
        109         connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
                110                 (struct sockaddr*)&cliaddr, &clilen);
    111 
        112         if (connfd < 0){
            113             printf("No clients in que, enter idle state\n");
            114             continue;
            115         }
    116 
        117         else if (connfd > 0) {
            118             if (connect(listenfd, (const struct sockaddr *)&cliaddr,
                        119                         sizeof(cliaddr)) != 0) {
                120                 printf("Udp connect failed.\n");
                121                 cleanup = 1;
                122             }
            123         }
    124         else {
        125             printf("Recvfrom failed.\n");
        126             cleanup = 1;
        127         }
    128         printf("Connected!\n");
    We say “Connected” for user friendly interpretation, but what we really mean is: “Datagrams have arrived, a client is attempting to communicate with us… let’s perform a handshake and see if they are using DTLS version 1.2… if so then ok, we’ll read their Datagrams and see what they have to say”.

        1.7.3 Using CyaSSL to Open a Session with Client.
        First we must declare an object that points to a CYASSL structure. We will just call it “ssl”. We will then assign it to use the correct cypher suite as previously defined by “ctx”. We will again perform some error handling on this assignment and then set the file descriptor that will handle all incoming and outgoing messages for this session. 
        Once all that has been set up we are ready to check and see if our client is using an acceptable cypher suite. We accomplish this by making a call to CyaSSL_accept on our ssl object that is now pointing to the file descriptor that has an awaiting Datagram in it. ( That’s a lot i know). We’ll use some fancy calls to error get methods so that if this part fails we will have a little bit of an idea as to why it failed, and how to fix it.

        Figure 2.14
        130         /* initialize arg */   
        131         CYASSL* ssl;        
    132                             
        133         /* Create the CYASSL Object */
        134         if (( ssl = CyaSSL_new(ctx) ) == NULL) {
            135             printf("CyaSSL_new error.\n");      
            136             cleanup = 1;                        
            137         }                                 
    138                         
        139          
        140         /* set the session ssl to client connection port */
        141         CyaSSL_set_fd(ssl, listenfd);                      
    142                                                            
        143                                      
        144         if (CyaSSL_accept(ssl) != SSL_SUCCESS) {
            145             int err = CyaSSL_get_error(ssl, 0); 
            146             char buffer[80];
            147             printf("error = %d, %s\n", err, CyaSSL_ERR_error_string(err, buffe    r));
            148             buffer[sizeof(buffer)-1]= 0;
            149             printf("SSL_accept failed.\n");
            150             cleanup = 1;
            151         }


    1.7.4 Read the Message, acknowledge that message was Read.
        Ok so our client is using DTLS version 1.2, we have a message waiting for us… so let’s read it! Then let’s send our client an acknowlegement that we have received their message. Don’t forget to handle any potential errors along the way!

        Figure 2.15
        154         if (( recvlen = CyaSSL_read(ssl, buff, sizeof(buff)-1)) > 0){
            155 
                156             printf("heard %d bytes\n", recvlen);
            157 
                158             buff[recvlen - 1] = 0;
            159             printf("I heard this: \"%s\"\n", buff);
            160         }
    161 
        162 
        163         if (recvlen < 0) {
            164             int readErr = CyaSSL_get_error(ssl, 0);
            165             if(readErr != SSL_ERROR_WANT_READ) {
                166                 printf("SSL_read failed.\n");
                167                 cleanup = 1;
                168             }
            169         }
    170
        171         if (CyaSSL_write(ssl, ack, sizeof(ack)) < 0) {
            172             printf("CyaSSL_write fail.\n");
            173             cleanup = 1;
            174         }
    175 
        176         else
        177             printf("lost the connection to client\n");
    178 
        179         printf("reply sent \"%s\"\n", ack);

    1.7.5 Free up the Memory
        Finally we need to free up all memory that was allocated on our server for this particular session. If the client so chooses they can connect with us again using the same session key. But we do not want to bog down our server with unnecessary messages from previous sessions, once those messages have been handled. So we reset the file descriptor to 0 allocated bits, and shutdown our active ssl object and free the memory. Lastly we print out a line letting us know that we have reached the end of our loop successfully, all memory is free, and we are returning to the top to listen for the next client that might want to talk to us.

        Figure 2.16
        183         CyaSSL_set_fd(ssl, 0);
    184         CyaSSL_shutdown(ssl);
    185         CyaSSL_free(ssl);
    186 
        187         printf("Client left return to idle state\n");
    This concludes Section 1 of Chapter 2 on “Layering DTLS onto a UDP Server”. Secition 2 will now cover Layering DTLS onto a UDP Client.
        A working version of this server code can be found at:
        https://github.com/kaleb-himes/wolfSSL_Interns/blob/master/project1/dtls_secured/server-dtls.c

        Section 2: Leah

        2.1. Enable DTLS
        As stated in chapter 4 of the CyaSSL manual, DTLS is enabled by using the --enable-dtls build option when building CyaSSL. If you have not done so, this should be your first step.  

        2.2. CyaSSL Tutorial
        Walk through chapter 11 in the CyaSSL tutorial. Follow the guides for TLS/SSL using the correct CyaDTLS client method. A few adjustments for dtls:

        2.2.1.    
        Make sure you have the correct port defined in your program, i.e., #define 
        SERV_PORT 11111 for server with 11111 set as the port.

        2.2.2.    
        Edit the arguments in your send and receive function to just 2 arguments - a FILE* 
        object and a CYASSL* object (previously this function had 4 arguments).

        2.2.3.    
        Change sendto and recvfrom functions to CyaSSL_write and CyaSSL_read. 
        Delete the last 3 arguments that were in sendto and recvfrom. In the CyaSSL_read() call, change the first argument from a socket to the CYASSL* object from the original function call.

        2.3. Set Peer
        Make a call to the CyaSSL_dtls_set_peer() function. It will take in as arguments your CYASSL* object, a point to the address carrying your sockaddr_in structure, and size of the structure. Example:

        Figure 2.17            
        CyaSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));

    This function will be called between where you built the sockaddr_in structure and your socket creation.
        2.4. Connect
        Add a CyaSSL connect function below the call to CyaSSL_set_fd() and pass the CYASSL* object you created as the argument. Include error checking. Example:

        Figure 2.18            
        if  (CyaSSL_connect(ssl) != SSL_SUCCESS) {
            int err1 = CyaSSL_get_error(ssl, 0);
            char buffer[80];              
            printf("err = %d, %s\n", err1,yaSSL_ERR_error_string(err1, buffer));
            err_sys("SSL_connect failed");                                   
        }

    2.5. Write/Read
        Call your CyaSSL_write/CyaSSL_read function. Example:

        Figure 2.19            
        DatagramClient(stdin, ssl);

    2.6. Shutdown, free, cleanup
        Make calls to CyaSSL_shutdown(), CyaSSL_free(), CyaSSL_CTX_free(), and CyaSSL_Cleanup() with correct parameters in each. This can be done in the read/write function or at the end of the main method. 

        2.7. Adjust Makefile
        Include -DCYASSL_DTLS before -o in your compilation line. This will include the DTLS method you chose. 



        CHAPTER 3: 
        Multi-threading the DTLS server.

        1. Include the pthread library
        To begin multi-threading our server we will need to include the library called pthread.h, (#include <pthread.h>). Add that to the rest of your #include statements and initialize a new method called “void* ThreadControl(void*);”. We will make this method our model that controls thread behavior. Then scroll to the line in the method “AwaitDGram” that sais: 

        Figure 3.1
        110         connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
                111                 (struct sockaddr*)&cliaddr, &clilen);


    2. Spinning a New thread
        Recall this line of code is simply “watching” for a message to arrive thus the “MSG_PEEK”. Once a message arrives we want to spin off a thread separate from the main thread of our server to handle the client session. In this way many clients can be handled simultaneously. Remember we included the library for pthread’s so we can now use the following code to initialize a new thread upon each message arrival (Figure 3.2). 

        Figure 3.2 Spinning a new thread
        116         pthread_t threadID;
    117 
        118         if (pthread_create(&threadID, NULL,
                    119                     ThreadControl, (void *)&connfd) < 0) {
            120             printf("pthread_create failed.\n");
            121         }
    122         printf("Connection being re-routed to Thread Control.\n");
    123         }
    124         else {
        125             printf("Recvfrom failed.\n");
        126             cleanup = 1;
        127         }

        We initialize a thread ID that will be a unique identifier for each individual thread. Next we call pthread_create on that id, some thread attribute (NULL here), a start routine (ThreadControl here), and our socket that the connection attempt was made on.  Some error handling and we’re done here. Now let’s go build our thread layout so each individual thread will know how it is supposed to behave upon instantiation.

        3. Thread Model
        We want each thread we spin to behave in the same manner as our previous server that wold loop on client messages until no further messages were sent. This is accomplished by never closing the socket the client initiated contact on, however we will exit our thread upon thread completion. In this way we do retain the socket descriptor but nothing else in memory. This will reduce our overhead cost of handling multiple clients simultaneously. In order to close a thread upon completion and have that memory be re-allocated we must detach the thread immediately upon creation. We can accomplish this by calling pthread_detach on itself as seen in line 141 of figure 3.3. Next we have all the variables previously in our AwaitDGram() method moved here for every call beyond the listening for datagrams to arrive line (line 110-111 in figure 3.1.

                Other than that our behavior for our thead is nearly identicle to what we had previously in AwaitDGram() after we layered on dtls in chapter 2. At the end of ThreadControl, we need to also initialize a dummy int that will become a void pointer, and call pthread_exit on that dummy variable. Our thread will then return a void pointer that can be accessed by pthread_join to make use of the memory space this current thread had used,  after it is exited. (Sounds confusing).

                Figure 3.3 Thread Control
                138 void* ThreadControl(void* openSock)
                139 {
                140 
                141     pthread_detach(pthread_self());
                142     char ack[] = "I hear you fashizzle!\n";
                143     int listenfd = 0;
                144     socklen_t clilen;               /* length of address' */
                145     int recvlen = 0;                /* length of message */
                146     char buff[MSGLEN];              /* the incoming message */


                THE RESET OF OUR BODY HERE...

                245 
                246     printf("reply sent \"%s\"\n", ack);
                247     printf("No further packets, severing connection to client.\n");
                248 
                    249     CyaSSL_set_fd(ssl, 0);
                250     CyaSSL_shutdown(ssl);
                251     CyaSSL_free(ssl);
                252 
                    253 
                    254     printf("Returning to idle state\n");
                255     int dummy;
                256     pthread_exit((void*)&dummy);
                257 }
                This completes the section on multi-threading our dtls wrapped, UDP server. Next I will cover non-blocking in chapter 5.
                A working version of the multi-threaded server can be found at: https://github.com/kaleb-himes/wolfSSL_Interns/blob/master/project1/dtls_secured_multithreaded/server-dtls-threaded.c







                CHAPTER 4: 
                Session Resumption with DTLS client.
                Leah

                1. Main Method Changes
                1.1     
                All of the code up until the main method will remain the same for session 
                resumption.however, you may want to add some of the main method calls into their own functions to shorten main.

                1.2 
                Add a few new objects at the top of main 

                1.2.1. 
                A new CyaSSL object for the resumption portion.

                1.2.2. 
                A CyaSSL session object to create the session.

                1.2.3.
                A char* object to print a message that session resume is testing.

Figure 4.1 (1.2.1-1.2.3 Example code:)        
    CYASSL* sslResume = 0;
    CYASSL_SESSION* session = 0;
    char* srTest = "testing session resume";

    2. Start session resumption
    2.1.     
    After the call to your read/write function, you will need to write again. Make a call 
    to CyaSSL_write() using your original ssl object, your char* resume test object, and size of resume test object as the 3 arguments. This will send the message that you are trying to test for session resumption to the server.
    2.2.     
    Set your CyaSSL session object to get the session with  your Cyassl object.
    2.3.     
    Set up a new CyaSSL object for the resumption stage using sslResume.

Figure 4.2 (2.1-2.3 Example code:)        
    CyaSSL_write(ssl, srTest, sizeof(srTest));
    session = CyaSSL_get_session(ssl);
    sslResume = CyaSSL_new(ctx);


    2.4.     
    Shutdown and free your original CYASSL object, ssl in our case, and close the     socket.

    Figure 4.3
    CyaSSL_shutdown(ssl);
    CyaSSL_free(ssl);
    close(sockfd);

    2.5.     
    Rebuild the server address structure the same way you did in setting up UDP. 
    Simply repeat the four lines of code at the end of II.B. in Create Basic UDP Client).
    2.6.     
    Reset the DTLS peer by calling the CyaSSL_dtls_set_peer function again 
    changing only the first parameter from your original CYASSL object to your resumption object → we changed ssl to sslResume.    

    Figure 4.4
    CyaSSL_dtls_set_peer(sslResume, &servAddr, sizeof(servAddr));
    Next, re-create your socket using the method used prior - same code as in part 
    2.1 of Create Basic UDP Client.
    2.7.    
    Set the file descriptor by calling CyaSSL_set_fd again with the new CYASSL 
    2.8.    
    Set the session. Use CyaSSL_set_session(sslResume, session).
    object, i.e., sslResume. Use the same socket you just recreated.
    2.9.    
    Connect again using CyaSSL_connect with your new resume CYASSL 
    object(sslResume).
    2.10.    
    Check if the session was actually reused. 
    2.11.    
    Call your read and write function a second time with the sslResume as your new 
    CYASSL object. 
    2.12.    
    Call CyaSSL_write with the same parameters as step A. only changing ssl to 
    sslResume.

Figure 4.5 (2.7 - 2.12 Example Code:)
    CyaSSL_set_fd(sslResume, sockfd);
    CyaSSL_set_session(sslResume, session);

if (CyaSSL_connect(sslResume) != SSL_SUCCESS)
    err_sys("SSL_connect failed");

if(CyaSSL_session_reused(sslResume))
    printf("reused session id\n");
    else
    printf("didn't reuse session id!!!\n");

    DatagramClient(stdin, sslResume);
    CyaSSL_write(sslResume, srTest, sizeof(srTest));



    2.13.    
    Shutdown and free sslResume and close the socket. Make sure you still have 
    the call to free your ctx and cleanup.     

    Figure 4.6
    CyaSSL_shutdown(sslResume);
    CyaSSL_free(sslResume);
    close(sockfd);
    CyaSSL_CTX_free(ctx);
    CyaSSL_Cleanup();

    3. Adjust Main Method
    3.1.    
    After the first call to CyaSSL_set_fd and again after the first call to 
    CyaSSL_set_session(), replace the following:

    Figure 4.7
    if (CyaSSL_connect(ssl) != SSL_SUCCESS) {
        int err1 = CyaSSL_get_error(ssl, 0);
        char buffer[80];
        printf("err = %d, %s\n", err1, CyaSSL_ERR_error_string(err1, buffer));  
        err_sys("SSL_connect failed");
    }
with:

Figure 4.8
CyaSSL_set_using_nonblock(ssl, 1);
fcntl(sockfd, F_SETFL, O_NONBLOCK);
NonBlockingDTLS_Connect(ssl);
Replace:

    Figure 4.9
if (CyaSSL_connect(sslResume) != SSL_SUCCESS)
    err_sys("SSL_connect failed");
with:

Figure 4.10
CyaSSL_set_using_nonblock(sslResume, 1);
fcntl(sockfd, F_SETFL, O_NONBLOCK);
NonBlockingDTLS_Connect(sslResume);





3.2.    
Finally, add a call to sleep after the last call to write:

Figure 4.11
sleep(1);
This concludes the simple UDP client portion of Chapter 1.

CHAPTER 5:
Convert Server and Client to non-blocking.

Section 1: Kaleb
1.1 To begin non-blocking
Go back to our dtls wrapped udp server example (Not the multi-threaded version, just the dtls). We will include the library “fcntl.h” that will allow us to set our socket to a non-blocking state.
Next we are going to need some methods if we do not want the main body of our code to get rediculously long. So initialize the methods as seen in figure 5.1. The model for our nonblocking server can be seen in sub-Figure 5.1.1

Figure 5.1 methods declaration
static int cleanup;                 /* To handle shutdown */
CYASSL_CTX* ctx;

void sig_handler(const int sig);    /* handles ctrl+c termination */
void AwaitDGram();                  /* Separate out Handling Datagrams */
void NonBlockingSSL_Accept(CYASSL*);/* non-blocking accept */
void dtls_set_nonblocking(int*);    /* set the socket non-blocking */
int udp_read_connect(int);          /* broken out to improve readability */
int dtls_select();

/* costumes for select_ret to wear */
enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};


Picture enum variables as costumes. One variable we will call “bill”. Bill can either dress up like a pirate, a ninja, a soldier, or a nurse, but whatever costum he’s wearing, it’s still Bill inside. Our variable here can either dress up as a TEST_SELECT_FAIL, or a TEST_TIMEOUT… etc. reference figure 5.3 to see how enum is used.

Figure 5.1.1 non blocking server model
1. Create Socket
2. Bind Socket
3. Await DGram arrival

4. Set socket to nonblocking  <--------------      
5. select                                    |
6. accept                                    |
7. Read Message                              |
8. Reply to Message                          |
9. Loop to step 4-----------------------------



1.2 udp_read_connect (int)
    We previously had this code in the body of AwaitDGram() however we broke it out to reduce the size of AwaitDGram() as well as to improve readability of the code. This is just our method to “watch” for datagrams to arrive.

    Figure 5.2 watch for datagrams
int udp_read_connect(int listenfd)
{
    int connfd;
    unsigned char  b[1500];
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    /* ensure b is empty upon each call */
    memset(b, 0, sizeof(b));

    connfd = (int)recvfrom(listenfd, (char*)b, sizeof(b), MSG_PEEK,
            (struct sockaddr*)&cliaddr, &clilen);
    if (connfd > 0) {
        if (connect(listenfd, (const struct sockaddr*)&cliaddr,
                    sizeof(cliaddr)) != 0) {
            printf("udp connect failed.\n");
        }
    }
    else {
        printf("recvfrom failed.\n");
    }
    return listenfd;
}

1.3 NonBlockingSSL_Accept(CYASSL* ssl)
    Here we will see the variable “select_ret” dress up in different costumes. In order to get a costume we reference method “dtls_select()” to see how different costumes are picked by “select_ret” (see figure 5.5) Based on the costume “select_ret” is wearing we will make some decisions. This method is pulled in directly from the library test.h in the cyassl libraries and can be found in the following location: Starting from the root directory of cyassl type “cd cyassl” this is where test.h is located. Search for NonBlockingSSL_Accept to see this code in it’s original format. Figure 5.3 has some very minor edits to make it work with our example server.

    Figure 5.3 NonBlockingSSL_Accept
void NonBlockingSSL_Accept(CYASSL* ssl)
{
    int select_ret;
    int currTimeout = 1;
    int ret = CyaSSL_accept(ssl);
    int error = CyaSSL_get_error(ssl, 0);
    int listenfd = (int)CyaSSL_get_fd(ssl);

    while (cleanup != 1 && (ret != SSL_SUCCESS &&
                (error == SSL_ERROR_WANT_READ ||
                 error == SSL_ERROR_WANT_WRITE))) {
        if (cleanup == 1) {
            CyaSSL_free(ssl);
            CyaSSL_shutdown(ssl);
            break;
        }

        if (error == SSL_ERROR_WANT_READ)
            printf("... server would read block\n");
        else
            printf("... server would write block\n");

        currTimeout = CyaSSL_dtls_get_current_timeout(ssl);
        select_ret = dtls_select(listenfd, currTimeout);

        if ((select_ret == TEST_RECV_READY) ||
                (select_ret == TEST_ERROR_READY)) {
            ret = CyaSSL_accept(ssl);
            error = CyaSSL_get_error(ssl, 0);
        }
        else if (select_ret == TEST_TIMEOUT && !CyaSSL_dtls(ssl)) {
            error = SSL_ERROR_WANT_READ;
        }
        else if (select_ret == TEST_TIMEOUT && CyaSSL_dtls(ssl) &&
                CyaSSL_dtls_got_timeout(ssl) >= 0) {
            error = SSL_ERROR_WANT_READ;
        }
        else {
            error = SSL_FATAL_ERROR;
        }
    }
    if (ret != SSL_SUCCESS) {
        printf("SSL_accept failed.\n");
    }
}

1.4 dtls_set_nonblocking(int*)
    Again this method is pulled in from test.h however modified slightly to work with our server. (removed un-necessary #ifdefs). Here is where we use fcntl to set our socket to a non-blocking state.

    Figure 5.4 set nonblocking
235 static void dtls_set_nonblocking(int* listenfd)
    236 {
        237     int flags = fcntl(*listenfd, F_GETFL, 0);
        238     if (flags < 0) {
            239         printf("fcntl get failed");
            240         cleanup = 1;
            241     }
        242     flags = fcntl(*listenfd, F_SETFL, flags | O_NONBLOCK);
        243     if (flags < 0) {
            244         printf("fcntl set failed.\n");
            245         cleanup = 1;
            246     }
        247 }

1.5 Select()
    Again this is pulled from test.h and modified to work with our server.  Here you see how “select_ret” from section 5.1.3 picks a costume to wear as a result of given conditions.

    Figure 5.5 Picking a costume for select_ret
249 static int dtls_select(int socketfd, int to_sec)
    250 {
        251     fd_set recvfds, errfds;
        252     int nfds = socketfd + 1;
        253     struct timeval timeout = { (to_sec > 0) ? to_sec : 0, 0};
        254     int result;
        255 
            256     FD_ZERO(&recvfds);
        257     FD_SET(socketfd, &recvfds);
        258     FD_ZERO(&errfds);
        259     FD_SET(socketfd, &errfds);
        260 
            261     result = select(nfds, &recvfds, NULL, &errfds, &timeout);
        262 
            263     if (result == 0)
            264         return TEST_TIMEOUT;
        265     else if (result > 0) {
            266         if (FD_ISSET(socketfd, &recvfds))
                267             return TEST_RECV_READY;
            268         else if(FD_ISSET(socketfd, &errfds))
                269             return TEST_ERROR_READY;
            270     }
        271 
            272     return TEST_SELECT_FAIL;
        273 }

        1.6 AwaitDGram() after changes
        *note: all line references in this section are referencing figure 5.6 unless stated otherwise.

        After we initialize the CYASSL object in AwaitDGram() we will then let it know that it is going to using non-blocking methods (line 129) Then we will set our socket to a non-blocking state (line 130). Next accept the datagram packets arriving in a non-blocking manner, attempt to read the message arriving, attempt to reply, if any of those fail, loop and try to read and/or write again. 

        Figure 5.6 
        CyaSSL_set_using_nonblock(ssl, 1);
        dtls_set_nonblocking(&clientfd);
        NonBlockingSSL_Accept(ssl);

        /* Begin: Reply to the client */
        buff[sizeof(buff)-1] = 0;
        recvlen = CyaSSL_read(ssl, buff, sizeof(buff)-1);

        currTimeout = CyaSSL_dtls_get_current_timeout(ssl);

        readWriteErr = CyaSSL_get_error(ssl, 0);      
        do {
            if (cleanup == 1) {
                memset(buff, 0, sizeof(buff));
                break;
            }
            if (recvlen < 0) {
                readWriteErr = CyaSSL_get_error(ssl, 0);
                if (readWriteErr != SSL_ERROR_WANT_READ) {
                    printf("Read Error, error was: %d.\n", readWriteErr);
                    cleanup = 1;
                } else {
                    recvlen = CyaSSL_read(ssl, buff, sizeof(buff)-1);
                }
            }
        } while (readWriteErr == SSL_ERROR_WANT_READ && recvlen < 0 &&
                currTimeout >= 0 && cleanup != 1);

        if (recvlen > 0) {
            buff[recvlen] = 0;
            printf("I heard this:\"%s\"\n", buff);
        }
else {
    printf("Connection Timed Out.\n");
}

if (CyaSSL_write(ssl, ack, sizeof(ack)) < 0) {
    printf("Write error.\n");
    cleanup = 1;
}
printf("Reply sent:\"%s\"\n", ack);

/* free allocated memory */
memset(buff, 0, sizeof(buff));
CyaSSL_free(ssl);

/* End: Reply to the Client */
}
}

This concludes the tutorial section on making a dtls wrapped 

Section 2: Leah

2.1. Add new headers to top of file:    
#include <errno.h>    /* error checking */
#include <fcntl.h>    /* set file status flags */

2.2. Add enum variables for testing functions - before first function:    
enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};

2.3. Add a DTLS selection function 
This is similar to the tcp_select() function in CyaSSL. This function will also call 
select():
    35 /* tcp select using dtls nonblocking function*/
    36 static int dtls_select(int socketfd, int to_sec)
    37 {
        38
            39     fd_set         recvfds, errfds;
        40     int            nfds = socketfd +1;
        41     struct timeval timeout = { (to_sec > 0) ? to_sec : 0, 0};
        42     int            result;
        43
            44     FD_ZERO(&recvfds);
        45     FD_SET(socketfd, &recvfds);
        46     FD_ZERO(&errfds);
        47     FD_SET(socketfd, &errfds);
        48
            49     result = select(nfds, &recvfds, NULL, &errfds, &timeout);
        50
            51     if (result == 0)
            52         return TEST_TIMEOUT;
        53     else if (result > 0) {
            54         if (FD_ISSET(socketfd, &recvfds))
                55             return TEST_RECV_READY;
            56         else if (FD_ISSET(socketfd, &errfds))
                57             return TEST_ERROR_READY;
            58     }
        59     return TEST_SELECT_FAIL;
        60 }



        2.4. NonBlocking DTLS connect function:
        This function calls the connect function and checks for various errors within the connection attempts. We placed it before the DatagramClient() function:
        /*Connect using Nonblocking - DTLS version*/
63 static void NonBlockingDTLS_Connect(CYASSL* ssl)
    64 {
        65     int      ret = CyaSSL_connect(ssl);
        66     int      error = CyaSSL_get_error(ssl, 0);
        67     int      sockfd = (int)CyaSSL_get_fd(ssl);
        68     int      select_ret;
        69     while (ret != SSL_SUCCESS && (error == SSL_ERROR_WANT_READ ||
                    70                 error == SSL_ERROR_WANT_WRITE)) {
            71         int currTimeout = 1;
            72         if (error == SSL_ERROR_WANT_READ)
                73                 printf("... client would read block\n");
            74         else
                75                 printf("... client would write block\n");
            76         currTimeout = CyaSSL_dtls_get_current_timeout(ssl);
            77         select_ret = dtls_select(sockfd, currTimeout);
            78         if ( ( select_ret == TEST_RECV_READY) ||
                    79                 (select_ret == TEST_ERROR_READY)) {
                80                 ret = CyaSSL_connect(ssl);
                81                 error = CyaSSL_get_error(ssl, 0);
                82         }
            83         else if (select_ret == TEST_TIMEOUT && !CyaSSL_dtls(ssl)) {
                84                 error = 2;
                85         }
            86         else if (select_ret == TEST_TIMEOUT && CyaSSL_dtls(ssl) &&
                    87                 CyaSSL_dtls_got_timeout(ssl) >= 0) {
                88                 error = 2;
                89         }
            90         else{
                91                 error = SSL_FATAL_ERROR;
                92         }
            93     }
        94
            95     if (ret != SSL_SUCCESS)
            96         err_sys("SSL_connect failed with");
        97 }


        2.5.    Adjust Datagram Client Function (could be located within main method).
        Create while loops for CyaSSL_write() and CyaSSL_read() to check for error    
        void DatagramClient (FILE* clientInput, CYASSL* ssl) {

            int     n = 0;
            char    sendLine[MAXLINE], recvLine[MAXLINE - 1];

            fgets(sendLine, MAXLINE, clientInput);

            while  ( ( CyaSSL_write(ssl, sendLine, strlen(sendLine))) !=
                    strlen(sendLine)) 
                err_sys("SSL_write failed");

            while ( (n = CyaSSL_read(ssl, recvLine, sizeof(recvLine)-1)) <= 0) {

                int readErr = CyaSSL_get_error(ssl, 0);
                if(readErr != SSL_ERROR_WANT_READ)
                    err_sys("CyaSSL_read failed");
            }

            recvLine[n] = '\0';
            fputs(recvLine, stdout);

        }

REFERENCES:

1. Paul Krzyzanowski, “Programming with UDP sockets”, Copyright 2003-2014, PK.ORG

2. The Open Group, “setsockopt - set the socket options”, Copyright © 1997, The Single UNIX ® Specification, Version 2

