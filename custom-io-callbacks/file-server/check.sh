#!/bin/sh

# Start the server first: the client's first write must not race an absent reader.
./start-server $1 &
SERVER_PID=$!

cd ../file-client || exit 1
./start-client $1
CLIENT_RESULT=$?
cd ../file-server || exit 1

# Bound the wait. A failed client leaves the server blocked reading a fifo that
# will never deliver, and a bare `wait` would hang until the CI step times out.
i=0
while kill -0 "${SERVER_PID}" 2>/dev/null && [ "$i" -lt 30 ]; do
    sleep 1
    i=$((i + 1))
done

if kill -0 "${SERVER_PID}" 2>/dev/null; then
    echo "server still running after ${i}s; killing it"
    kill "${SERVER_PID}" 2>/dev/null
    SERVER_RESULT=1
else
    wait "${SERVER_PID}"
    SERVER_RESULT=$?
fi

echo "server exited ${SERVER_RESULT}"
echo "client exited ${CLIENT_RESULT}"

[ ${CLIENT_RESULT} -eq 0 ] && [ ${SERVER_RESULT} -eq 0 ]
