#!/usr/bin/env bash

EXEC_PATH="./build/Debug/"
VALGRIND="valgrind --tool=memcheck"

$VALGRIND $EXEC_PATH/Sudoku -branching cbs 89