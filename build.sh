#!/bin/bash

code="$PWD"
opts=-g
cd build > /dev/null
g++ $opts $code/build.bat -o generator.exe
cd $code > /dev/null
