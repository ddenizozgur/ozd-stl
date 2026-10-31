@echo off

rem Build.bat              - debug build
rem Build.bat run          - debug build, then runs it
rem Build.bat release      - optimized build (-O3, asserts off)

cd /d "%~dp0"

set mode=-g -O1
set run=0
for %%a in (%*) do (
  if "%%a"=="release" set mode=-O3 -DNDEBUG
  if "%%a"=="run" set run=1
)

set compileFlags=-std=c++20 -Wall -Wextra -Wundef -Wshorten-64-to-32 -fno-exceptions -fno-rtti -msse4.1 -I. %mode%
set linkFlags=-Xlinker /NODEFAULTLIB:libcpmt.lib -Xlinker /NODEFAULTLIB:msvcprt.lib

if not exist Bin mkdir Bin

clang++ %compileFlags% main.cpp -o Bin\DX11Example.exe %linkFlags%
if errorlevel 1 exit /b 1

if %run%==1 Bin\DX11Example.exe
