#!/bin/sh

function test() {
CHECK=$1
 [ $CHECK != 23 ] && [ $CHECK != 0 ] && echo "CHECK = $CHECK" && exit 5
echo "test passed"
echo ""
}

echo "TEST 1: VALID"
echo "TEST 1.a"
OUTPUT=`wolfssl -x509 -inform pem -outform pem`
RESULT=$?
test $RESULT
echo "TEST 1.b"
OUTPUT=`wolfssl -x509 -inform pem -outform der`
RESULT=$?
test $RESULT
echo "TEST 1.c"
OUTPUT=`wolfssl -x509 -inform der -outform pem`
RESULT=$?
test $RESULT
echo "TEST 1.d"
OUTPUT=`wolfssl -x509 -inform der -outform der`
RESULT=$?
test $RESULT
echo ""

echo "TEST 2: INVALID INPUT"
echo "TEST 2.a"
OUTPUT=`wolfssl -x509 -inform pem -inform der`
RESULT=$?
test $RESULT
echo "TEST 2.b"
OUTPUT=`wolfssl -x509 -outform pem -outform der`
RESULT=$?
test $RESULT
echo "TEST 2.c"
OUTPUT=`wolfssl -x509 -inform -inform`
RESULT=$?
test $RESULT
echo "TEST 2.d"
OUTPUT=`wolfssl -x509 -outform -outform`
RESULT=$?
test $RESULT
echo "TEST 2.e"
OUTPUT=`wolfssl -x509 -inform pem -inform der -inform`
RESULT=$?
test $RESULT
echo "TEST 2.f"
OUTPUT=`wolfssl -x509 -outform pem -outform der -outform`
RESULT=$?
test $RESULT
echo "TEST 2.g"
OUTPUT=`wolfssl -x509 -inform pem -outform der -inform`
RESULT=$?
test $RESULT
echo "TEST 2.h"
OUTPUT=`wolfssl -x509 -outform pem -inform der -outform`
RESULT=$?
test $RESULT
echo "TEST 2.i"
OUTPUT=`wolfssl -x509 -inform`
RESULT=$?
test $RESULT
echo "TEST 2.j"
OUTPUT=`wolfssl -x509 -outform`
RESULT=$?
test $RESULT



