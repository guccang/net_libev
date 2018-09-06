#! /bin/bash

echo make clean
make clean

echo client
make -f ./makefile TARGET=client SRC=./src_client

echo server
make -f ./makefile TARGET=ev1 SRC=./src
