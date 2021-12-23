@echo off


IF "%1"=="vs" (
    devenv build\debug\primevil.exe
) ELSE (
    pushd build\debug
    START /D ..\..\data primevil.exe 
    popd
)
