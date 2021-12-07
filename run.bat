@echo off


IF "%1"=="vs" (
    devenv build\debug\ps1.exe
) ELSE (
    pushd build\debug
    START /D ..\..\data ps1.exe 
    popd
)