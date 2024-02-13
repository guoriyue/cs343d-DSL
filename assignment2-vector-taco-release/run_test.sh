#!/bin/bash

# For use in JIT.

TEST_FILE=test.cpp
TEST_EXE=test.out

BIN_DIR=bin
INC_DIR=include

CXX=clang++
CXXFLAGS="-g -O3 -std=c++17 -Itests -I./$INC_DIR/"

KERNEL=$1
TEST=$2

cat $KERNEL > $TEST_FILE
cat $TEST >> $TEST_FILE

mkdir -p $BIN_DIR
rm -f $TEST_EXE
$CXX $CXXFLAGS $TEST_FILE -o $TEST_EXE
./$TEST_EXE
