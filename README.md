# ozd-stl

A work-in-progress base layer for C++20. Windows only for now.

## Requirements

- **Windows**
  - `Base/`: Windows 8
  - `Win32Stuff/`: Windows 8, `MagicRingBuffer.hpp` needs Windows 10 1803
  - `DX11Stuff/`: Windows 10 1803
- **Visual Studio Build Tools** with the *Desktop development with C++* workload: [download](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
- Or [portable-msvc.py](https://gist.github.com/mmozeiko/7f3162ec2988e81e56d5c4e22cde9977). Run `setup_x64.bat` from its output folder before building.
- Optional **Clang**: install it from the [LLVM releases](https://github.com/llvm/llvm-project/releases) (`LLVM-<version>-win64.exe`) or with:
  ```
  winget install LLVM.LLVM
  ```

## Usage

Copy the folders you need side by side into your project, include the headers and compile as C++20. Headers include each other with relative paths and link their own libraries with `#pragma comment(lib)`, so no include paths or linker settings are needed.

## Build

### Visual Studio Build Tools
From an *x64 Native Tools* or *x86_x64 Cross Tools Command Prompt for VS*:
```
cd zExample
Build.bat release run
```
or `Build.bat clang release run`

```
cmake -B Build
cmake --build Build --config Release
```
The executables can then be found in `.\zExample\Bin\` and `.\DX11Example\Bin\`

## Credits

Some of the bitmap fonts in `zExample/R_Fonts.hpp` are from VileR's [VGA text mode fonts](https://github.com/viler-int10h/vga-text-mode-fonts) collection.
