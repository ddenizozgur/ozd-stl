@echo off

rem Build.bat              - debug build with cl, needs an x64 Developer Command Prompt
rem Build.bat run          - debug build, then runs it
rem Build.bat release      - optimized build (asserts off)
rem Build.bat clang        - builds with clang++

cd /d "%~dp0"

set name=Example

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

set compileFlags=-std=c++20
set "compileFlags=%compileFlags% -fno-exceptions"
set "compileFlags=%compileFlags% -fno-rtti"
set "compileFlags=%compileFlags% -msse4.2"         & rem floorf/ceilf/roundf become one roundss instruction, also enables popcnt
if %release%==0 set "compileFlags=%compileFlags% -fstack-protector-strong"
set "compileFlags=%compileFlags% -Wall"
set "compileFlags=%compileFlags% -Wshorten-64-to-32"
set "compileFlags=%compileFlags% -Wsign-compare"
set "compileFlags=%compileFlags% -Wno-parentheses" & rem allows if (int x = Fun(); x = Next())
set "compileFlags=%compileFlags% -Wundef"          & rem undefined macro in #if
set "compileFlags=%compileFlags% -Wshadow"
if %release%==1 (set "compileFlags=%compileFlags% -O3 -DNDEBUG") else (set "compileFlags=%compileFlags% -g -O1")

set linkFlags=
set "linkFlags=%linkFlags% -Xlinker /NODEFAULTLIB:libcpmt.lib" & rem no static C++ standard library
set "linkFlags=%linkFlags% -Xlinker /NODEFAULTLIB:msvcprt.lib" & rem no DLL C++ standard library

clang++ %compileFlags% main.cpp -o Bin\%name%.exe %linkFlags%
if errorlevel 1 exit /b 1
goto built

:cl
set compileFlags=/nologo /std:c++20
set "compileFlags=%compileFlags% /GR-"                    & rem no rtti
set "compileFlags=%compileFlags% /arch:SSE4.2"            & rem floorf/ceilf become one roundss instruction in optimized builds
set "compileFlags=%compileFlags% /utf-8"
set "compileFlags=%compileFlags% /Zc:preprocessor"        & rem macros expand like clang
if %release%==1 set "compileFlags=%compileFlags% /GS-"    & rem no stack cookies in release
set "compileFlags=%compileFlags% /W4"
set "compileFlags=%compileFlags% /wd4244"                 & rem allows narrowing
set "compileFlags=%compileFlags% /wd4305"                 & rem allows f32 f = 0.1
set "compileFlags=%compileFlags% /w34388"                 & rem signed/unsigned compare of different sizes
set "compileFlags=%compileFlags% /w34062"                 & rem enum value missing from switch
set "compileFlags=%compileFlags% /w34668"                 & rem undefined macro in #if
set "compileFlags=%compileFlags% /wd4100"                 & rem allows unused parameters
set "compileFlags=%compileFlags% /wd4706"                 & rem allows if (int x = Fun(); x = Next())
set "compileFlags=%compileFlags% /wd4201"                 & rem allows anonymous structs
set "compileFlags=%compileFlags% /wd4324"                 & rem allows padding from alignas
set "compileFlags=%compileFlags% /wd4245"                 & rem allows u32 x = -1
set "compileFlags=%compileFlags% /external:anglebrackets" & rem headers included with angle brackets are external
set "compileFlags=%compileFlags% /external:W0"            & rem no warnings from external headers
if %release%==1 (set "compileFlags=%compileFlags% /O2 /DNDEBUG") else (set "compileFlags=%compileFlags% /Zi /Od")

set linkFlags=
set "linkFlags=%linkFlags% /INCREMENTAL:NO"
set "linkFlags=%linkFlags% /NODEFAULTLIB:libcpmt.lib" & rem no static C++ standard library
set "linkFlags=%linkFlags% /NODEFAULTLIB:msvcprt.lib" & rem no DLL C++ standard library

cl %compileFlags% main.cpp /Fe:Bin\%name%.exe /Fo:Bin\ /Fd:Bin\ /link %linkFlags%
if errorlevel 1 exit /b 1

:built
if %run%==1 Bin\%name%.exe
