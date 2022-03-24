#pragma once

/* 
embedded version of KEY_FILE

example to generate this CA_FILE text for a new file:

sed 's/\(.*\)\r/"\1\\n"/g' client-key.pem

see also this DER to C script:

https://github.com/wolfSSL/wolfssl/blob/master/scripts/dertoc.pl

*/


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-sign"
const unsigned char * KEY_FILE = ""
    
/* TODO this is a test example! DO NOT USE IN PRODUCTION!! */

/* insert your ca-cert.pem text below */ 
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpAIBAAKCAQEAwwPRK/45pDJFO1PIhCsqfHSavaoqUgdH1qY2sgcyjtC6aXvG\n"
"w0Se1IFI/S1oootnu6F1yDYsStIb94u6zw357+zxgR57mwNHmr9lzH9lJGmm6BSJ\n"
"W+Q098WwFJP1Z3s6enjhAVZWkaYTQo3SPECcTO/Rht83URsMoTv18aNKNeThzpbf\n"
"G36/TpfQEOioCDCBryALQxTFdGe0MoJvjYbCiECZNoO6HkByIhfXUmUkc7DO7xnN\n"
"rv94bHvAEgPUTnINUG07ozujmV6dyNkMhbPZitlUJttt+qy7/yVMxNF59HHThkAY\n"
"E7BjtXJOMMSXhIYtVi/XFfd/wK71/Fvl+6G60wIDAQABAoIBAQCi5thfEHFkCJ4u\n"
"bdFtHoXSCrGMR84sUWqgEp5T3pFMHW3qWXvyd6rZxtmKq9jhFuRjJv+1bBNZuOOl\n"
"yHIXLgyfb+VZP3ZvSbERwlouFikN3reO3EDVou7gHqH0vpfbhmOWFM2YCWAtMHac\n"
"PM3miO5HknkLWgDiXl8RfH35CLcgBokqXf0AqyLh8LO8JKleJg4fAC3+IZpTW23T\n"
"K6uUgmhDNtj2L8Yi/LVBXQ0zYOqkfX7oS1WRVtNcV48flBcvqt7pnqj0z4pMjqDk\n"
"VnOyz0+GxWk88yQgi1yWDPprEjuaZ8HfxpaypdWSDZsJQmgkEEXUUOQXOUjQNYuU\n"
"bRHej8pZAoGBAOokp/lpM+lx3FJ9iCEoL0neunIW6cxHeogNlFeEWBY6gbA/os+m\n"
"bB6wBikAj+d3dqzbysfZXps/JpBSrvw4kAAUu7QPWJTnL2p+HE9BIdQxWR9OihqN\n"
"p1dsItjl9H4yphDLZKVVA4emJwWMw9e2J7JNujDaR49U0z2LhI2UmFilAoGBANU4\n"
"G8OPxZMMRwtvNZLFsI1GyJIYj/WACvfvof6AubUqusoYsF2lB9CTjdicBBzUYo6m\n"
"JoEB/86KKmM0NUCqbYDeiSNqV02ebq2TTlaQC22dc4sMric93k7wqsVseGdslFKc\n"
"N2dsLe+7r9+mkDzER8+Nlp6YqbSfxaZQ3LPw+3QXAoGAXoMJYr26fKK/QnT1fBzS\n"
"ackEDYV+Pj0kEsMYe/Mp818OdmxZdeRBhGmdMvPNIquwNbpKsjzl2Vi2Yk9d3uWe\n"
"CspTsiz3nrNrClt5ZexukU6SIPb8/Bbt03YM4ux/smkTa3gOWkZktF63JaBadTpL\n"
"78c8Pvf9JrggxJkKmnO+wxkCgYEAukSTFKw0GTtfkWCs97TWgQU2UVM96GXcry7c\n"
"YT7Jfbh/h/A7mwOCKTfOck4R1bHBDAegmZFKjX/sec/xObXphexi99p9vGRNIjwO\n"
"8tZR9YfYmcARIF0PKf1b4q7ZHNkhVm38hNBf7RAVHBgh58Q9S9fQnmqVzyLJA3ue\n"
"42AB/C8CgYAR0EvPG2e5nxB1R4ZlrjHCxjCsWQZQ2Q+1cAb38NPIYnyo2m72IT/T\n"
"f1/qiqs/2Spe81HSwjA34y2jdQ0eTSE01VdwXIm/cuxKbmjVzRh0M06MOkWP5pZA\n"
"62P5GYY6Ud2JS7Dz+Z9dKJU4vjWrylznk1M0oUVdEzllQkahn831vw==\n"
"-----END RSA PRIVATE KEY-----\n"
"\x00"
; 

long sizeof_KEY_FILE() {
    unsigned char * p;
    p = strchr(CERT_FILE, 0);
    return p - CERT_FILE + 1;
}
;
#pragma GCC diagnostic pop
