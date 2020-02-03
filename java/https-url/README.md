Example HTTPS URL use with wolfJSSE

Example of Building
javac -cp ~/Documents/wolfssljni/lib/wolfssl-jsse.jar URLClient.java

Example of Running

java -Djava.library.path=/Users/<username>/Documents/wolfssljni/lib/ -classpath ~/Documents/wolfssljni/lib/wolfssl-jsse.jar:./ URLClient -h https://www.google.com -A ../example-keystores/external.jks:"wolfSSL test"
