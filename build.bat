@echo off
if not exist build mkdir build
pushd build
cl ../source/win32/win32_main.c /link user32.lib /out:fight_win32.exe
popd