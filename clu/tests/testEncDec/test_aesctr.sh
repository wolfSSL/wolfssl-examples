###### AES CTR ######
wolfssl -e aes-ctr-128 -p hello128 -i ../somejunk/somejunk_100000.txt -o enc_aes_ctr_128.txt
wolfssl -d aes-ctr-128 -i enc_aes_ctr_128.txt -o dec_aes_ctr_128.txt -p hello128

wolfssl -e aes-ctr-192 -p hello192 -i ../somejunk/somejunk_100000.txt -o enc_aes_ctr_192.txt
wolfssl -d aes-ctr-192 -i enc_aes_ctr_192.txt -o dec_aes_ctr_192.txt -p hello192

wolfssl -e aes-ctr-256 -p hello256 -i ../somejunk/somejunk_100000.txt -o enc_aes_ctr_256.txt
wolfssl -d aes-ctr-256 -i enc_aes_ctr_256.txt -o dec_aes_ctr_256.txt -p hello256
