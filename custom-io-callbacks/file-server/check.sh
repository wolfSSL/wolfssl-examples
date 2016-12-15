#!/bin/sh
./start-server $1 &
PID1=$!
RESULT1=$?

cd ../file-client
./start-client $1
PID2=$!
RESULT2=$?

echo "RESULT1 = ${RESULT1}"
echo ""
echo "RESULT2 = ${RESULT2}"
echo ""
kill ${PID1}
kill ${PID2}

