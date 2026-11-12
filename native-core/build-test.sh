#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p build
c++ -std=c++17 -Wall -Wextra -Werror -pedantic -Iinclude src/modcore.cpp tests/core_tests.cpp -o build/modcore_tests
./build/modcore_tests
