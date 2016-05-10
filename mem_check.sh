#!/usr/bin/env bash

EXEC_PATH="./build/Debug/"
VALGRIND="valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes"

$VALGRIND $EXEC_PATH/Sudoku -branching cbs 4