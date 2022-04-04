ets Jun  8 2016 00:22:57

rst:0x1 (POWERON_RESET),boot:0x17 (SPI_FAST_FLASH_BOOT)
ets Jun  8 2016 00:22:57

rst:0x10 (RTCWDT_RTC_RESET),boot:0x17 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:6628
load:0x40078000,len:14780
load:0x40080400,len:3792
entry 0x40080694
I (27) boot: ESP-IDF v4.4-263-g000d3823bb-dirty 2nd stage bootloader
I (27) boot: compile time 12:02:57
I (28) boot: chip revision: 0
I (34) boot.esp32: SPI Speed      : 40MHz
I (37) boot.esp32: SPI Mode       : DIO
I (41) boot.esp32: SPI Flash Size : 4MB
I (46) boot: Enabling RNG early entropy source...
I (51) boot: Partition Table:
I (55) boot: ## Label            Usage          Type ST Offset   Length
I (62) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (69) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (77) boot:  2 factory          factory app      00 00 00010000 00100000
I (84) boot: End of partition table
I (89) esp_image: segment 0: paddr=00010020 vaddr=3f400020 size=27b58h (162648) map
I (156) esp_image: segment 1: paddr=00037b80 vaddr=3ffb0000 size=02514h (  9492) load
I (160) esp_image: segment 2: paddr=0003a09c vaddr=40080000 size=05f7ch ( 24444) load
I (172) esp_image: segment 3: paddr=00040020 vaddr=400d0020 size=6ed3ch (453948) map
I (336) esp_image: segment 4: paddr=000aed64 vaddr=40085f7c size=072a8h ( 29352) load
I (349) esp_image: segment 5: paddr=000b6014 vaddr=50000000 size=00010h (    16) load
I (355) boot: Loaded app from partition at offset 0x10000
I (356) boot: Disabling RNG early entropy source...
I (369) cpu_start: Pro cpu up.
I (369) cpu_start: Starting app cpu, entry point is 0x400811a8
I (0) cpu_start: App cpu up.
I (385) cpu_start: Pro cpu start user code
I (385) cpu_start: cpu freq: 160000000
I (385) cpu_start: Application information:
I (390) cpu_start: Project name:     enc28j60
I (395) cpu_start: App version:      v0.1-896-g0684a6c-dirty
I (401) cpu_start: Compile time:     Mar 20 2022 12:02:40
I (407) cpu_start: ELF file SHA256:  bf95d02d456649ea...
I (413) cpu_start: ESP-IDF:          v4.4-263-g000d3823bb-dirty
I (420) heap_init: Initializing. RAM available for dynamic allocation:
I (427) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (433) heap_init: At 3FFBA000 len 00026000 (152 KiB): DRAM
I (439) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (446) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (452) heap_init: At 4008D224 len 00012DDC (75 KiB): IRAM
I (460) spi_flash: detected chip: gd
I (463) spi_flash: flash io: dio
I (468) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (478) gpio: GPIO[4]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (488) enc28j60: revision: 6
I (508) esp_eth.netif.netif_glue: 02:00:00:12:34:56
I (508) esp_eth.netif.netif_glue: ethernet attached to netif
I (2508) enc28j60: working in 10Mbps
I (2508) enc28j60: working in half duplex
I (4478) esp_netif_handlers: eth ip: 192.168.75.39, mask: 255.255.255.0, gw: 192.168.75.1
I (5508) wolfssl: inet_pton
I (5508) wolfssl: socket creation successful

I (5508) wolfssl: sockfd connect successful

I (5508) wolfssl: calling wolfSSL_Init
I (5508) wolfssl: wolfSSL Entering wolfSSL_Init
I (5518) wolfssl: wolfSSL Entering wolfCrypt_Init
I (5518) wolfssl: wolfSSL_Init successful

I (5528) wolfssl: wolfSSL Entering TLSv1_3_client_method_ex
I (5528) wolfssl: wolfSSL Entering wolfSSL_CTX_new_ex
I (5538) wolfssl: wolfSSL Entering wolfSSL_CertManagerNew
I (5548) wolfssl: wolfSSL Leaving WOLFSSL_CTX_new, return 0
I (5548) wolfssl: Loading cert
I (5558) wolfssl: wolfSSL Entering wolfSSL_CTX_use_certificate_buffer
I (5558) wolfssl: wolfSSL Entering PemToDer
I (5568) wolfssl: Checking cert signature type
I (5568) wolfssl: wolfSSL Entering GetExplicitVersion
I (5578) wolfssl: wolfSSL Entering GetSerialNumber
I (5578) wolfssl: Got Cert Header
I (5588) wolfssl: wolfSSL Entering GetAlgoId
I (5588) wolfssl: wolfSSL Entering GetObjectId()
I (5598) wolfssl: Got Algo ID
I (5598) wolfssl: Getting Cert Name
I (5608) wolfssl: Getting Cert Name
I (5608) wolfssl: Getting Cert Name
I (5608) wolfssl: Getting Cert Name
I (5618) wolfssl: Got Subject Name
I (5618) wolfssl: wolfSSL Entering GetAlgoId
I (5628) wolfssl: wolfSSL Entering GetObjectId()
I (5628) wolfssl: Got Key
I (5638) wolfssl: Not ECDSA cert signature
I (5638) wolfssl: wolfSSL Leaving wolfSSL_CTX_use_certificate_buffer, return 1
I (5648) wolfssl: wolfSSL_CTX_use_certificate_buffer successful

I (5658) wolfssl: wolfSSL Entering wolfSSL_CTX_use_PrivateKey_buffer
I (5658) wolfssl: wolfSSL Entering PemToDer
I (5668) wolfssl: wolfSSL Entering GetAlgoId
I (5668) wolfssl: wolfSSL Leaving wolfSSL_CTX_use_PrivateKey_buffer, return 1
I (5678) wolfssl: wolfSSL_CTX_use_PrivateKey_buffer successful

I (5688) wolfssl: wolfSSL Entering wolfSSL_CTX_load_verify_buffer_ex
I (5688) wolfssl: Processing CA PEM file
I (5698) wolfssl: wolfSSL Entering PemToDer
I (5708) wolfssl: Adding a CA
I (5708) wolfssl: wolfSSL Entering GetExplicitVersion
I (5708) wolfssl: wolfSSL Entering GetSerialNumber
I (5718) wolfssl: Got Cert Header
I (5718) wolfssl: wolfSSL Entering GetAlgoId
I (5728) wolfssl: wolfSSL Entering GetObjectId()
I (5728) wolfssl: Got Algo ID
I (5738) wolfssl: Getting Cert Name
I (5738) wolfssl: Getting Cert Name
I (5748) wolfssl: Getting Cert Name
I (5748) wolfssl: Getting Cert Name
I (5748) wolfssl: Got Subject Name
I (5758) wolfssl: wolfSSL Entering GetAlgoId
I (5758) wolfssl: wolfSSL Entering GetObjectId()
I (5768) wolfssl: Got Key
I (5768) wolfssl: Parsed Past Key
I (5768) wolfssl: wolfSSL Entering DecodeCertExtensions
I (5778) wolfssl: wolfSSL Entering GetObjectId()
I (5788) wolfssl: wolfSSL Entering DecodeSubjKeyId
I (5788) wolfssl: wolfSSL Entering GetObjectId()
I (5798) wolfssl: wolfSSL Entering DecodeAuthKeyId
I (5798) wolfssl: wolfSSL Entering GetObjectId()
I (5808) wolfssl: wolfSSL Entering DecodeBasicCaConstraint
I (5808) wolfssl: wolfSSL Entering GetObjectId()
I (5818) wolfssl: wolfSSL Entering DecodeAltNames
I (5818) wolfssl:       Unsupported name type, skipping
I (5828) wolfssl: wolfSSL Entering GetObjectId()
I (5838) wolfssl: wolfSSL Entering DecodeExtKeyUsage
I (5838) wolfssl: wolfSSL Entering GetObjectId()
I (5848) wolfssl: wolfSSL Entering GetObjectId()
I (5848) wolfssl: wolfSSL Entering GetAlgoId
I (5858) wolfssl: wolfSSL Entering GetObjectId()
I (5858) wolfssl:       Parsed new CA
I (5868) wolfssl:       Freeing Parsed CA
I (5868) wolfssl:       Freeing der CA
I (5868) wolfssl:               OK Freeing der CA
I (5878) wolfssl: wolfSSL Leaving AddCA, return 0
I (5878) wolfssl:    Processed a CA
I (5888) wolfssl: Processed at least one valid CA. Other stuff OK
I (5888) wolfssl: wolfSSL Leaving wolfSSL_CTX_load_verify_buffer_ex, return 1
I (5898) wolfssl: wolfSSL_CTX_load_verify_buffer successful

I (5908) wolfssl: wolfSSL Entering SSL_new
I (5918) wolfssl: wolfSSL Leaving SSL_new, return 0
I (5918) wolfssl: wolfSSL Entering SSL_set_fd
I (5918) wolfssl: wolfSSL Entering SSL_set_read_fd
I (5928) wolfssl: wolfSSL Leaving SSL_set_read_fd, return 1
I (5938) wolfssl: wolfSSL Entering SSL_set_write_fd
I (5938) wolfssl: wolfSSL Leaving SSL_set_write_fd, return 1
I (5948) wolfssl: wolfSSL_set_fd successful

I (5948) wolfssl: wolfSSL Entering SSL_connect()
I (5958) wolfssl: wolfSSL Entering SendTls13ClientHello
I (5958) wolfssl: Adding signature algorithms extension
I (5968) wolfssl: Adding supported versions extension
I (5978) wolfssl: wolfSSL Entering EccMakeKey
I (6568) wolfssl: wolfSSL Leaving EccMakeKey, return 0
I (6568) wolfssl: growing output buffer
I (6568) wolfssl: Key Share extension to write
I (6568) wolfssl: Supported Versions extension to write
I (6578) wolfssl: Signature Algorithms extension to write
I (6588) wolfssl: Supported Groups extension to write
I (6598) wolfssl: Shrinking output buffer
I (6598) wolfssl: wolfSSL Leaving SendTls13ClientHello, return 0
I (6598) wolfssl: connect state: CLIENT_HELLO_SENT
I (6608) wolfssl: growing input buffer
I (6608) wolfssl: received record layer msg
I (6618) wolfssl: got HANDSHAKE
I (6618) wolfssl: wolfSSL Entering DoTls13HandShakeMsg()
I (6628) wolfssl: wolfSSL Entering DoTls13HandShakeMsgType
I (6628) wolfssl: processing server hello
I (6638) wolfssl: wolfSSL Entering DoTls13ServerHello
I (6638) wolfssl: Supported Versions extension received
I (6648) wolfssl: Key Share extension received
I (6658) wolfssl: wolfSSL Entering EccSharedSecret
I (6658) wolfssl: wolfSSL Entering wc_ecc_shared_secret_gen_sync
I (7238) wolfssl: wolfSSL Leaving wc_ecc_shared_secret_gen_sync, return 0
I (7238) wolfssl: wolfSSL Leaving wc_ecc_shared_secret_ex, return 0
I (7238) wolfssl: wolfSSL Leaving EccSharedSecret, return 0
I (7248) wolfssl: Skipping Supported Versions - already processed
I (7248) wolfssl: wolfSSL Entering VerifyClientSuite
I (7258) wolfssl: wolfSSL Leaving DoTls13ServerHello, return 0
I (7268) wolfssl: Shrinking input buffer
I (7268) wolfssl: Derive Early Secret
I (7268) wolfssl: Derive Handshake Secret
I (7278) wolfssl: Derive Client Handshake Secret
I (7278) wolfssl: Derive Server Handshake Secret
I (7288) wolfssl: Derive Client Key
I (7288) wolfssl: Derive Server Key
I (7298) wolfssl: Derive Client IV
I (7298) wolfssl: Derive Server IV
I (7308) wolfssl: wolfSSL Leaving DoTls13HandShakeMsgType(), return 0
I (7308) wolfssl: wolfSSL Leaving DoTls13HandShakeMsg(), return 0
I (7318) wolfssl: wolfSSL Entering wolfSSL_connect_TLSv13()
I (7328) wolfssl: connect state: HELLO_AGAIN
I (7328) wolfssl: connect state: HELLO_AGAIN_REPLY
I (7338) wolfssl: growing input buffer
I (7338) wolfssl: wolfSSL Entering DecryptTls13
I (7348) wolfssl: received record layer msg
I (7348) wolfssl: got HANDSHAKE
I (7348) wolfssl: wolfSSL Entering DoTls13HandShakeMsg()
I (7358) wolfssl: wolfSSL Entering DoTls13HandShakeMsgType
I (7368) wolfssl: processing encrypted extensions
I (7368) wolfssl: wolfSSL Entering DoTls13EncryptedExtensions
I (7378) wolfssl: wolfSSL Leaving DoTls13EncryptedExtensions, return 0
I (7388) wolfssl: Shrinking input buffer
I (7388) wolfssl: wolfSSL Leaving DoTls13HandShakeMsgType(), return 0
I (7398) wolfssl: wolfSSL Leaving DoTls13HandShakeMsg(), return 0
I (7398) wolfssl: growing input buffer
I (7408) wolfssl: wolfSSL Entering DecryptTls13
I (7408) wolfssl: received record layer msg
I (7418) wolfssl: got HANDSHAKE
I (7418) wolfssl: wolfSSL Entering DoTls13HandShakeMsg()
I (7428) wolfssl: wolfSSL Entering DoTls13HandShakeMsgType
I (7428) wolfssl: processing certificate request
I (7438) wolfssl: wolfSSL Entering DoTls13CertificateRequest
I (7448) wolfssl: Signature Algorithms extension received
I (7448) wolfssl: wolfSSL Leaving DoTls13CertificateRequest, return 0
I (7458) wolfssl: Shrinking input buffer
I (7458) wolfssl: wolfSSL Leaving DoTls13HandShakeMsgType(), return 0
I (7468) wolfssl: wolfSSL Leaving DoTls13HandShakeMsg(), return 0
I (7478) wolfssl: growing input buffer
I (7478) wolfssl: wolfSSL Entering DecryptTls13
I (7488) wolfssl: received record layer msg
I (7488) wolfssl: got HANDSHAKE
I (7498) wolfssl: wolfSSL Entering DoTls13HandShakeMsg()
I (7498) wolfssl: wolfSSL Entering DoTls13HandShakeMsgType
I (7508) wolfssl: processing certificate
I (7508) wolfssl: wolfSSL Entering DoTls13Certificate
I (7518) wolfssl: wolfSSL Entering ProcessPeerCerts
I (7518) wolfssl: Loading peer's cert chain
I (7528) wolfssl:       Put another cert into chain
I (7528) wolfssl: Verifying Peer's cert
I (7538) wolfssl: wolfSSL Entering GetExplicitVersion
I (7538) wolfssl: wolfSSL Entering GetSerialNumber
I (7548) wolfssl: Got Cert Header
I (7548) wolfssl: wolfSSL Entering GetAlgoId
I (7558) wolfssl: wolfSSL Entering GetObjectId()
I (7558) wolfssl: Got Algo ID
I (7568) wolfssl: Getting Cert Name
I (7568) wolfssl: Getting Cert Name
I (7578) wolfssl: Getting Cert Name
I (7578) wolfssl: Getting Cert Name
I (7588) wolfssl: Got Subject Name
I (7588) wolfssl: wolfSSL Entering GetAlgoId
I (7588) wolfssl: wolfSSL Entering GetObjectId()
I (7598) wolfssl: Got Key
I (7598) wolfssl: Parsed Past Key
I (7608) wolfssl: wolfSSL Entering DecodeCertExtensions
I (7608) wolfssl: wolfSSL Entering GetObjectId()
I (7618) wolfssl: wolfSSL Entering DecodeSubjKeyId
I (7618) wolfssl: wolfSSL Entering GetObjectId()
I (7628) wolfssl: wolfSSL Entering DecodeAuthKeyId
I (7628) wolfssl: wolfSSL Entering GetObjectId()
I (7638) wolfssl: wolfSSL Entering DecodeBasicCaConstraint
I (7638) wolfssl: wolfSSL Entering GetObjectId()
I (7648) wolfssl: wolfSSL Entering DecodeAltNames
I (7658) wolfssl:       Unsupported name type, skipping
I (7658) wolfssl: wolfSSL Entering GetObjectId()
I (7668) wolfssl: wolfSSL Entering DecodeExtKeyUsage
I (7668) wolfssl: wolfSSL Entering GetObjectId()
I (7678) wolfssl: wolfSSL Entering GetObjectId()
I (7678) wolfssl: wolfSSL Entering GetAlgoId
I (7688) wolfssl: wolfSSL Entering GetObjectId()
I (7688) wolfssl: CA found
I (7698) wolfssl: wolfSSL Entering ConfirmSignature
I (7728) wolfssl: wolfSSL Leaving ConfirmSignature, return 0
I (7728) wolfssl: Verified Peer's cert
I (7728) wolfssl: wolfSSL Leaving ProcessPeerCerts, return 0
I (7738) wolfssl: wolfSSL Leaving DoTls13Certificate, return 0
I (7748) wolfssl: Shrinking input buffer
I (7748) wolfssl: wolfSSL Leaving DoTls13HandShakeMsgType(), return 0
I (7758) wolfssl: wolfSSL Leaving DoTls13HandShakeMsg(), return 0
I (7768) wolfssl: growing input buffer
I (7768) wolfssl: wolfSSL Entering DecryptTls13
I (7778) wolfssl: received record layer msg
I (7778) wolfssl: got HANDSHAKE
I (7778) wolfssl: wolfSSL Entering DoTls13HandShakeMsg()
I (7788) wolfssl: wolfSSL Entering DoTls13HandShakeMsgType
I (7798) wolfssl: processing certificate verify
I (7798) wolfssl: wolfSSL Entering DoTls13CertificateVerify
I (7808) wolfssl: Peer sent RSA sig
I (7808) wolfssl: wolfSSL Entering RsaVerify
I (7848) wolfssl: wolfSSL Using RSA PSS un-padding
I (7848) wolfssl: wolfSSL Leaving RsaVerify, return 64
I (7848) wolfssl: wolfSSL Leaving DoTls13CertificateVerify, return 0
I (7848) wolfssl: Shrinking input buffer
I (7858) wolfssl: wolfSSL Leaving DoTls13HandShakeMsgType(), return 0
I (7858) wolfssl: wolfSSL Leaving DoTls13HandShakeMsg(), return 0
I (7868) wolfssl: growing input buffer
I (7878) wolfssl: wolfSSL Entering DecryptTls13
I (7878) wolfssl: received record layer msg
I (7888) wolfssl: got HANDSHAKE
I (7888) wolfssl: wolfSSL Entering DoTls13HandShakeMsg()
I (7898) wolfssl: wolfSSL Entering DoTls13HandShakeMsgType
I (7898) wolfssl: processing finished
I (7908) wolfssl: wolfSSL Entering DoTls13Finished
I (7908) wolfssl: Derive Finished Secret
I (7918) wolfssl: Derive Finished Secret
I (7918) wolfssl: wolfSSL Leaving DoTls13Finished, return 0
I (7928) wolfssl: Shrinking input buffer
I (7928) wolfssl: Derive Master Secret
I (7938) wolfssl: Derive Client Traffic Secret
I (7938) wolfssl: Derive Server Traffic Secret
I (7948) wolfssl: Derive Client Key
I (7948) wolfssl: Derive Server Key
I (7948) wolfssl: Derive Client IV
I (7958) wolfssl: Derive Server IV
I (7958) wolfssl: wolfSSL Leaving DoTls13HandShakeMsgType(), return 0
I (7968) wolfssl: wolfSSL Leaving DoTls13HandShakeMsg(), return 0
I (7978) wolfssl: connect state: FIRST_REPLY_DONE
I (7978) wolfssl: connect state: FIRST_REPLY_FIRST
I (7988) wolfssl: connect state: FIRST_REPLY_SECOND
I (7988) wolfssl: wolfSSL Entering SendTls13Certificate
I (7998) wolfssl: growing output buffer
I (7998) wolfssl: wolfSSL Entering BuildTls13Message
I (8008) wolfssl: wolfSSL Entering EncryptTls13
I (8018) wolfssl: wolfSSL Leaving BuildTls13Message, return 0
I (8018) wolfssl: Shrinking output buffer
I (8028) wolfssl: wolfSSL Leaving SendTls13Certificate, return 0
I (8028) wolfssl: sent: certificate
I (8038) wolfssl: connect state: FIRST_REPLY_THIRD
I (8038) wolfssl: wolfSSL Entering SendTls13CertificateVerify
I (8048) wolfssl: growing output buffer
I (8048) wolfssl: Trying RSA private key
I (8058) wolfssl: wolfSSL Entering GetAlgoId
I (8058) wolfssl: Using RSA private key
I (8068) wolfssl: wolfSSL Entering RsaSign
I (8068) wolfssl: wolfSSL Using RSA PSS padding
I (8478) wolfssl: wolfSSL Leaving RsaSign, return 0
I (8478) wolfssl: wolfSSL Entering VerifyRsaSign
I (8508) wolfssl: wolfSSL Using RSA PSS un-padding
I (8508) wolfssl: wolfSSL Leaving VerifyRsaSign, return 0
I (8508) wolfssl: wolfSSL Entering BuildTls13Message
I (8518) wolfssl: wolfSSL Entering EncryptTls13
I (8518) wolfssl: wolfSSL Leaving BuildTls13Message, return 0
I (8528) wolfssl: Shrinking output buffer
I (8528) wolfssl: wolfSSL Leaving SendTls13CertificateVerify, return 0
I (8538) wolfssl: sent: certificate verify
I (8538) wolfssl: connect state: FIRST_REPLY_FOURTH
I (8548) wolfssl: wolfSSL Entering SendTls13Finished
I (8548) wolfssl: growing output buffer
I (8558) wolfssl: wolfSSL Entering BuildTls13Message
I (8558) wolfssl: wolfSSL Entering EncryptTls13
I (8568) wolfssl: wolfSSL Leaving BuildTls13Message, return 0
I (8568) wolfssl: Derive Resumption Secret
I (8578) wolfssl: Shrinking output buffer
I (8578) wolfssl: wolfSSL Leaving SendTls13Finished, return 0
I (8588) wolfssl: sent: finished
I (8588) wolfssl: connect state: FINISHED_DONE
I (8598) wolfssl: wolfSSL Entering FreeHandshakeResources
I (8608) wolfssl: wolfSSL Leaving wolfSSL_connect_TLSv13(), return 1
I (8608) wolfssl: wolfSSL_connect successful

I (8618) wolfssl: wolfSSL Entering SSL_write()
I (8618) wolfssl: growing output buffer
I (8628) wolfssl: wolfSSL Entering BuildTls13Message
I (8628) wolfssl: wolfSSL Entering EncryptTls13
I (8638) wolfssl: wolfSSL Leaving BuildTls13Message, return 0
I (8648) wolfssl: Shrinking output buffer
I (8648) wolfssl: wolfSSL Leaving SSL_write(), return 12
I (8648) wolfssl: wolfSSL_write message sent successfully:

I (8658) wolfssl: Hello World

I (8668) wolfssl: wolfSSL Entering wolfSSL_read()
I (8668) wolfssl: wolfSSL Entering wolfSSL_read_internal()
I (8678) wolfssl: wolfSSL Entering ReceiveData()
I (8678) wolfssl: growing input buffer
I (8688) wolfssl: wolfSSL Entering DecryptTls13
I (8688) wolfssl: received record layer msg
I (8698) wolfssl: got app DATA
I (8698) wolfssl: Shrinking input buffer
I (8698) wolfssl: wolfSSL Leaving ReceiveData(), return 22
I (8708) wolfssl: wolfSSL Leaving wolfSSL_read_internal(), return 22
I (8718) wolfssl: wolfSSL_read received message:

I (8718) wolfssl: I hear ya fa shizzle!

I (8728) wolfssl: wolfSSL Entering SSL_free
I (8728) wolfssl: CTX ref count not 0 yet, no free
I (8738) wolfssl: Free'ing client ssl
I (8738) wolfssl: wolfSSL Entering ClientSessionToSession
I (8748) wolfssl: wolfSSL Leaving SSL_free, return 0
I (8748) wolfssl: wolfSSL Entering SSL_CTX_free
I (8758) wolfssl: CTX ref count down to 0, doing full free
I (8768) wolfssl: wolfSSL Entering wolfSSL_CertManagerFree
I (8768) wolfssl: wolfSSL Leaving SSL_CTX_free, return 0
I (8778) wolfssl: wolfSSL Entering wolfSSL_Cleanup
I (8778) wolfssl: wolfSSL Entering wolfCrypt_Cleanup
I (8788) wolfssl: wolfSSL Leaving tls_smp_client_task, return 1
I (8798) wolfssl: tls_smp_client_task done!


