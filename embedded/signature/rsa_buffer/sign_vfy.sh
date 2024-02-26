#!/bin/sh

make clean

make sign
./sign "This is the message" > signature.h
make verify
./verify


