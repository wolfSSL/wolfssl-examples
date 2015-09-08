
###### AES CBC ######
wolfssl -encrypt aes-cbc-128 -pwd hello128 -in ../somejunk/somejunk_100000.txt -out enc_aes_cbc_128.txt
wolfssl -decrypt aes-cbc-128 -in enc_aes_cbc_128.txt -out dec_aes_cbc_128.txt -pwd hello128

wolfssl -encrypt aes-cbc-192 -pwd hello192 -in ../somejunk/somejunk_100000.txt -out enc_aes_cbc_192.txt
wolfssl -decrypt aes-cbc-192 -in enc_aes_cbc_192.txt -out dec_aes_cbc_192.txt -pwd hello192

wolfssl -encrypt aes-cbc-256 -pwd hello256 -in ../somejunk/somejunk_100000.txt -out enc_aes_cbc_256.txt
wolfssl -decrypt aes-cbc-256 -in enc_aes_cbc_256.txt -out dec_aes_cbc_256.txt -pwd hello256

###### DES CBC ######
wolfssl -encrypt 3des-cbc-056 -pwd hello056 -in ../somejunk/somejunk_100000.txt -out enc_3des_cbc_056.txt
wolfssl -decrypt 3des-cbc-056 -in enc_3des_cbc_056.txt -out dec_3des_cbc_056.txt -pwd hello056

wolfssl -encrypt 3des-cbc-112 -pwd hello112 -in ../somejunk/somejunk_100000.txt -out enc_3des_cbc_112.txt
wolfssl -decrypt 3des-cbc-112 -in enc_3des_cbc_112.txt -out dec_3des_cbc_112.txt -pwd hello112

wolfssl -encrypt 3des-cbc-168 -pwd hello168 -in ../somejunk/somejunk_100000.txt -out enc_3des_cbc_168.txt
wolfssl -decrypt 3des-cbc-168 -in enc_3des_cbc_168.txt -out dec_3des_cbc_168.txt -pwd hello168

###### CAMELLIA CBC ######
wolfssl -encrypt camellia-cbc-128 -pwd hello128 -in ../somejunk/somejunk_100000.txt -out enc_camellia_cbc_128.txt
wolfssl -decrypt camellia-cbc-128 -in enc_camellia_cbc_128.txt -out dec_camellia_cbc_128.txt -pwd hello128

wolfssl -encrypt camellia-cbc-192 -pwd hello192 -in ../somejunk/somejunk_100000.txt -out enc_camellia_cbc_192.txt
wolfssl -decrypt camellia-cbc-192 -in enc_camellia_cbc_192.txt -out dec_camellia_cbc_192.txt -pwd hello192

wolfssl -encrypt camellia-cbc-256 -pwd hello256 -in ../somejunk/somejunk_100000.txt -out enc_camellia_cbc_256.txt
wolfssl -decrypt camellia-cbc-256 -in enc_camellia_cbc_256.txt -out dec_camellia_cbc_256.txt -pwd hello256
