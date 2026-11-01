@echo off

rem Build.bat              - debug build with cl, needs an x64 Developer Command Prompt (x64 or x86_x64)
rem Build.bat run          - debug build, then runs it
rem Build.bat release      - optimized build (asserts off)
rem Build.bat clang        - builds with clang++

cd /d "%~dp0"

set release=0
set run=0
set compiler=cl
for %%a in (%*) do (
  if "%%a"=="release" set release=1
  if "%%a"=="run" set run=1
  if "%%a"=="clang" set compiler=clang
)

if not exist Bin mkdir Bin

if %compiler%==cl goto cl

set mode=-g -O1
if %release%==1 set mode=-O3 -DNDEBUG
set compileFlags=-std=c++20 -Wall -Wextra -Wundef -Wshorten-64-to-32 -fno-exceptions -fno-rtti -msse4.1 -I. %mode%
set linkFlags=-Xlinker /NODEFAULTLIB:libcpmt.lib -Xlinker /NODEFAULTLIB:msvcprt.lib

clang++ %compileFlags% main.cpp -o Bin\Example.exe %linkFlags%
if errorlevel 1 exit /b 1
goto built

:cl
set mode=/Zi /Od
if %release%==1 set mode=/O2 /DNDEBUG
set compileFlags=/nologo /std:c++20 /W4 /wd4201 /Zc:preprocessor /utf-8 /GR- /EHs-c- /D_HAS_EXCEPTIONS=0 /I. %mode%
set linkFlags=/INCREMENTAL:NO /NODEFAULTLIB:libcpmt.lib /NODEFAULTLIB:msvcprt.lib

cl %compileFlags% main.cpp /Fe:Bin\Example.exe /Fo:Bin\ /Fd:Bin\ /link %linkFlags%
if errorlevel 1 exit /b 1

:built
if %run%==1 Bin\Example.exe
