#pragma once

/* 
embedded version of KEY_FILE

#define KEY_FILE  "../../../../certs/server-key.pem"

example to generate this KEY_FILE text for a new file:

sed 's/\(.*\)\r/"\1\\n"/g' ../../../certs/server-key.pem

see also this DER to C script:

https://github.com/wolfSSL/wolfssl/blob/master/scripts/dertoc.pl

*/


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-sign"
const unsigned char * KEY_FILE = ""
    
/* TODO this is a test example! DO NOT USE IN PRODUCTION!! */

/* insert your server-key.pem text below */ 
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpQIBAAKCAQEAwJUI4VdB8nFtt9JFQScBZcZFrvK8JDC4lc4vTtb2HIi8fJ/7\n"
"qGd//lycUXX3isoH5zUvj+G9e8AvfKtkqBf8yl17uuAh5XIuby6G2JVz2qwbU7lf\n"
"P9cZDSVP4WNjUYsLZD+tQ7ilHFw0s64AoGPF9n8LWWh4c6aMGKkCba/DGQEuuBDj\n"
"xsxAtGmjRjNph27Euxem8+jdrXO8ey8htf1mUQy9VLPhbV8cvCNz0QkDiRTSELlk\n"
"wyrQoZZKvOHUGlvHoMDBY3gPRDcwMpaAMiOVoXe6E9KXc+JdJclqDcM5YKS0sGlC\n"
"Qgnp2Ai8MyCzWCKnquvE4eZhg8XSlt/Z0E+t1wIDAQABAoIBAQCa0DQPUmIFUAHv\n"
"n+1kbsLE2hryhNeSEEiSxOlq64t1bMZ5OPLJckqGZFSVd8vDmp231B2kAMieTuTd\n"
"x7pnFsF0vKnWlI8rMBr77d8hBSPZSjm9mGtlmrjcxH3upkMVLj2+HSJgKnMw1T7Y\n"
"oqyGQy7E9WReP4l1DxHYUSVOn9iqo85gs+KK2X4b8GTKmlsFC1uqy+XjP24yIgXz\n"
"0PrvdFKB4l90073/MYNFdfpjepcu1rYZxpIm5CgGUFAOeC6peA0Ul7QS2DFAq6EB\n"
"QcIw+AdfFuRhd9Jg8p+N6PS662PeKpeB70xs5lU0USsoNPRTHMRYCj+7r7X3SoVD\n"
"LTzxWFiBAoGBAPIsVHY5I2PJEDK3k62vvhl1loFk5rW4iUJB0W3QHBv4G6xpyzY8\n"
"ZH3c9Bm4w2CxV0hfUk9ZOlV/MsAZQ1A/rs5vF/MOn0DKTq0VO8l56cBZOHNwnAp8\n"
"yTpIMqfYSXUKhcLC/RVz2pkJKmmanwpxv7AEpox6Wm9IWlQ7xrFTF9/nAoGBAMuT\n"
"3ncVXbdcXHzYkKmYLdZpDmOzo9ymzItqpKISjI57SCyySzfcBhh96v52odSh6T8N\n"
"zRtfr1+elltbD6F8r7ObkNtXczrtsCNErkFPHwdCEyNMy/r0FKTV9542fFufqDzB\n"
"hV900jkt/9CE3/uzIHoumxeu5roLrl9TpFLtG8SRAoGBAOyY2rvV/vlSSn0CVUlv\n"
"VW5SL4SjK7OGYrNU0mNS2uOIdqDvixWl0xgUcndex6MEH54ZYrUbG57D8rUy+UzB\n"
"qusMJn3UX0pRXKRFBnBEp1bA1CIUdp7YY1CJkNPiv4GVkjFBhzkaQwsYpVMfORpf\n"
"H0O8h2rfbtMiAP4imHBOGhkpAoGBAIpBVihRnl/Ungs7mKNU8mxW1KrpaTOFJAza\n"
"1AwtxL9PAmk4fNTm3Ezt1xYRwz4A58MmwFEC3rt1nG9WnHrzju/PisUr0toGakTJ\n"
"c/5umYf4W77xfOZltU9s8MnF/xbKixsX4lg9ojerAby/QM5TjI7t7+5ZneBj5nxe\n"
"9Y5L8TvBAoGATUX5QIzFW/QqGoq08hysa+kMVja3TnKW1eWK0uL/8fEYEz2GCbjY\n"
"dqfJHHFSlDBD4PF4dP1hG0wJzOZoKnGtHN9DvFbbpaS+NXCkXs9P/ABVmTo9I89n\n"
"WvUi+LUp0EQR6zUuRr79jhiyX6i/GTKh9dwD5nyaHwx8qbAOITc78bA=\n"
"-----END RSA PRIVATE KEY-----\n"
/* insert your server-key.pem text above */    
"\x00"
; 

long sizeof_KEY_FILE() {
    unsigned char * p;
    p = strchr(CERT_FILE, 0);
    return p - CERT_FILE + 1;
}
;
#pragma GCC diagnostic pop
