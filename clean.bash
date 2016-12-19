#!/bin/bash

rm -rvf *~ \#*\#;
make clean;
rm -rvf CMakeFiles/ cmake_install.cmake CMakeCache.txt Makefile;
rm -rvf build/*

exit 0;
