@echo off



set SOURCE=..\src\main_win32.cpp
set PREPROCDEFINES = -DINTERNAL_BUILD=1 -DSLOW_BUILD=1

rem Additional Include Directories - preceded by /I
set ADDINCLUDES=/I"..\lib\GLEW\include" /I"..\lib\SDL\include" /I"..\lib\stb"
rem Additional Library/Dependency Directories - preceded by /LIBPATH:
set ADDLIBPATHS=/LIBPATH:"..\lib\GLEW\lib\Release\Win32" /LIBPATH:"..\lib\SDL\lib\x86"
rem Additional Dependencies; may be from above ADDLIBPATHS
set ADDDEPENDENCIES=Shell32.lib opengl32.lib glew32.lib SDL2.lib SDL2main.lib

rem Binary Output Directory
set OUTPUTDIR="debug"
rem Output Executable Name
set OUTPUTEXE="ps1.exe"

rem Linker Options
set LINKEROPTIONS=/SUBSYSTEM:CONSOLE %ADDLIBPATHS% %ADDDEPENDENCIES%

if NOT exist build mkdir build
pushd build
if NOT exist %OUTPUTDIR% mkdir %OUTPUTDIR%
cl /Zi /EHsc /W3 /Fe%OUTPUTDIR%%OUTPUTEXE% %ADDINCLUDES% %SOURCE% /link %LINKEROPTIONS%
popd