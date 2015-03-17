
###### AES CBC ######
wolfssl -e aes-cbc-128 -p hello128 -i ../somejunk/somejunk_100000.txt -o enc_aes_cbc_128.txt
wolfssl -d aes-cbc-128 -i enc_aes_cbc_128.txt -o dec_aes_cbc_128.txt -p hello128

wolfssl -e aes-cbc-192 -p hello192 -i ../somejunk/somejunk_100000.txt -o enc_aes_cbc_192.txt
wolfssl -d aes-cbc-192 -i enc_aes_cbc_192.txt -o dec_aes_cbc_192.txt -p hello192

wolfssl -e aes-cbc-256 -p hello256 -i ../somejunk/somejunk_100000.txt -o enc_aes_cbc_256.txt
wolfssl -d aes-cbc-256 -i enc_aes_cbc_256.txt -o dec_aes_cbc_256.txt -p hello256

###### DES CBC ######
wolfssl -e 3des-cbc-056 -p hello056 -i ../somejunk/somejunk_100000.txt -o enc_3des_cbc_056.txt
wolfssl -d 3des-cbc-056 -i enc_3des_cbc_056.txt -o dec_3des_cbc_056.txt -p hello056

wolfssl -e 3des-cbc-112 -p hello112 -i ../somejunk/somejunk_100000.txt -o enc_3des_cbc_112.txt
wolfssl -d 3des-cbc-112 -i enc_3des_cbc_112.txt -o dec_3des_cbc_112.txt -p hello112

wolfssl -e 3des-cbc-168 -p hello168 -i ../somejunk/somejunk_100000.txt -o enc_3des_cbc_168.txt
wolfssl -d 3des-cbc-168 -i enc_3des_cbc_168.txt -o dec_3des_cbc_168.txt -p hello168

###### CAMELLIA CBC ######
wolfssl -e camellia-cbc-128 -p hello128 -i ../somejunk/somejunk_100000.txt -o enc_camellia_cbc_128.txt
wolfssl -d camellia-cbc-128 -i enc_camellia_cbc_128.txt -o dec_camellia_cbc_128.txt -p hello128

wolfssl -e camellia-cbc-192 -p hello192 -i ../somejunk/somejunk_100000.txt -o enc_camellia_cbc_192.txt
wolfssl -d camellia-cbc-192 -i enc_camellia_cbc_192.txt -o dec_camellia_cbc_192.txt -p hello192

wolfssl -e camellia-cbc-256 -p hello256 -i ../somejunk/somejunk_100000.txt -o enc_camellia_cbc_256.txt
wolfssl -d camellia-cbc-256 -i enc_camellia_cbc_256.txt -o dec_camellia_cbc_256.txt -p hello256
