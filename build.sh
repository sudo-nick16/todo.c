#!/bin/bash

clang -Wall -Wextra -g -ggdb -o todo todo.c -lpthread && ./todo
