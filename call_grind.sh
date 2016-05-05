#!/usr/bin/env bash

EXEC_PATH="./build/Debug/"
VALGRIND="valgrind --tool=callgrind --callgrind-out-file=callgrind.out"

$VALGRIND $EXEC_PATH/$@
kcachegrind callgrind.out &> /dev/null
