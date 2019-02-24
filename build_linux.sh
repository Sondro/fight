#! /bin/bash

compiler_flags="-I ../source/ -g -DBUILD_LINUX"
linker_flags="-lX11 -lm -lGL"
executable_name="fight_linux"

if [ ! -d "build" ]; then
  mkdir build
fi

pushd build
gcc $compiler_flags ../source/linux/linux_main.c $linker_flags -o $executable_name
popd
