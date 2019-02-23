#! /bin/bash

compiler_flags=   -I../source/
linker_flags=     -lX11
executable_name=  fight_linux

if [ ! -d "build" ]; then
  mkdir build
fi

pushd build
gcc %compiler_flags% ../source/linux/linux_main.c %linker_flags% -o %executable_name%
popd
