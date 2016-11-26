#!/bin/sh

function test() {
    CHECK=$1
    #UNCOMMENT FOR VERBOSE OUTPUT
#    echo "${2}"
    [ $CHECK != 23 ] && [ $CHECK != 22 ] && [ $CHECK != 0 ] \
                           && echo "CHECK = $CHECK" && exit 5
    [ $CHECK == 0 ] && echo "valid input: TEST PASSED"
    [ $CHECK == 23 ] && echo "successfully caught input error: TEST PASSED"
    [ $CHECK == 22 ] && echo "valid input, no input file specified: TEST PASSED"
    echo ""
}

function test_case() {
    echo "testing: ./wolfssl -x509 $1"
    OUTPUT=$(./wolfssl -x509 $1)
    RESULT=$?
    test $RESULT "$OUTPUT"
}

function cert_test_case() {
    echo "testing: ./wolfssl -x509 $1"
    OUTPUT=$(./wolfssl -x509 $1)
    RESULT=$?
    echo "RESULT: $RESULT"
    diff $2 $3
    RESULT=$?
    echo "RESULT OF DIFF: $RESULT"
    [ $RESULT != 0 ] && echo "DIFF FAILED" && exit 5
    echo ""
}

function run1() {
    echo "TEST 1: VALID"
    echo "TEST 1.a"
    test_case "-inform pem -outform pem"
    echo "TEST 1.b"
    test_case "-inform pem -outform der"
    echo "TEST 1.c"
    test_case "-inform der -outform pem"
    echo "TEST 1.d"
    test_case "-inform der -outform der"
    echo ""
    echo "TEST 1.e"
    test_case "-inform der -text -noout"
    echo ""
}

function run2() {
    echo "TEST 2: INVALID INPUT"
    echo "TEST 2.a"
    test_case "-inform pem -inform der"
    echo "TEST 2.b"
    test_case "-outform pem -outform der"
    echo "TEST 2.c"
    test_case "-inform -inform"
    echo "TEST 2.d"
    test_case "-outform -outform"
    echo "TEST 2.e"
    test_case "-inform pem -inform der -inform"
    echo "TEST 2.f"
    test_case "-outform pem -outform der -outform"
    echo "TEST 2.g"
    test_case "-inform pem -outform der -inform"
    echo "TEST 2.h"
    test_case "-outform pem -inform der -outform"
    echo "TEST 2.i"
    test_case "-inform"
    echo "TEST 2.j"
    test_case "-outform"
    echo "TEST 2.k"
    test_case "-outform pem -outform der -noout"
    echo "TEST 2.l"
    test_case "-outform -outform -noout"
    echo "TEST 2.m"
    test_case "-outform pem -outform der -outform -noout"
    echo "TEST 2.n"
    test_case "-inform pem -outform der -inform -noout"
    echo "TEST 2.o"
    test_case "-outform pem -inform der -outform -noout"
    echo "TEST 2.p"
    test_case "-outform -noout"
    echo "TEST 2.q"
    test_case "-inform pem -outform pem -noout"
}

function run3() {
    echo "TEST3: VALID INPUT FILES"
    echo "TEST 3.a"
    #convert ca-cert.der to tmp.pem and compare to ca-cert.pem for valid transform
    cert_test_case "-inform der -in testing-certs/ca-cert.der -outform pem -out tmp.pem" \
                   testing-certs/ca-cert.pem tmp.pem
    echo "TEST 3.b"
    cert_test_case "-inform pem -outform der -in testing-certs/ca-cert.pem -out tmp.der" \
                    testing-certs/ca-cert.der tmp.der
    echo "TEST 3.c"
    test_case "-inform der -in ca-cert.pem -outform der -out out.txt"
    echo "TEST 3.d"
    test_case "-inform pem -in ca-cert.pem -outform pem -out out.txt"
}

run1
run2
run3

rm out.txt
rm tmp.pem
rm tmp.der
