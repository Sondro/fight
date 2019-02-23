@echo off

set compiler_flags=   -I../source/ -nologo
set linker_flags=     user32.lib
set executable_name=  fight_win32.exe

if not exist build mkdir build
pushd build
cl %compiler_flags% ../source/win32/win32_main.c /link %linker_flags% /out:%executable_name%
popd