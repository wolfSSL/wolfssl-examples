###### AES CTR ######
wolfssl -encrypt aes-ctr-128 -pwd hello128 -in ../somejunk/somejunk_100000.txt -out enc_aes_ctr_128.txt
wolfssl -decrypt aes-ctr-128 -in enc_aes_ctr_128.txt -out dec_aes_ctr_128.txt -pwd hello128

wolfssl -encrypt aes-ctr-192 -pwd hello192 -in ../somejunk/somejunk_100000.txt -out enc_aes_ctr_192.txt
wolfssl -decrypt aes-ctr-192 -in enc_aes_ctr_192.txt -out dec_aes_ctr_192.txt -pwd hello192

wolfssl -encrypt aes-ctr-256 -pwd hello256 -in ../somejunk/somejunk_100000.txt -out enc_aes_ctr_256.txt
wolfssl -decrypt aes-ctr-256 -in enc_aes_ctr_256.txt -out dec_aes_ctr_256.txt -pwd hello256
