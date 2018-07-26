#!/bin/sh -e

if [ $# -ne 1 ]; then
    echo "$0 program_path"
    exit 1
fi
PROGRAM=$1

TIMEOUT=10

expect -c "
    set timeout ${TIMEOUT}
    spawn \"${PROGRAM}\"
    expect -re {uart0 at (.*)} {
        send $expect_out(1,string)
        send $expect_out(0,string)
        send $expect_out(2,string)
    }
    expect \"$\"
    exit 0
"
exit 0