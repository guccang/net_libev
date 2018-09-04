#! /bin/bash

protoc --cpp_out=./pb net.proto
echo success
