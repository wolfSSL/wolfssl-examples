## Steps to build:
    
```
# clone or download the wolfssl bundle and put it in the subdirectory wolfssl
git clone https://github.com/wolfssl/wolfssl
mkdir build
cd build
cmake .. -DCMAKE_C_FLAGS=-I../include/
make
./hash example_string
```


