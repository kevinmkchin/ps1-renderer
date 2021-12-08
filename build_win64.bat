@echo off



set SOURCE=..\src\main.cpp
set PREPROCDEFINES = -DINTERNAL_BUILD=1 -DSLOW_BUILD=1

rem Additional Include Directories - preceded by /I
set ADDINCLUDES=/I"..\lib\GLEW\include" /I"..\lib\SDL\include" /I"..\lib\ASSIMP\include" /I"..\lib\stb" /I"..\lib\noclip"
rem Additional Library/Dependency Directories - preceded by /LIBPATH:
set ADDLIBPATHS=/LIBPATH:"..\lib\GLEW\lib\Release\x64" /LIBPATH:"..\lib\SDL\lib\x64" /LIBPATH:"..\lib\ASSIMP\lib"
rem Additional Dependencies; may be from above ADDLIBPATHS
set ADDDEPENDENCIES=Shell32.lib opengl32.lib glew32.lib SDL2.lib SDL2main.lib assimp-vc142-mt.lib

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