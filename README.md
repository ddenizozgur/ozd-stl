# ozd-stl

A work-in-progress base layer for C++20. Windows only for now.

## Requirements

- **Visual Studio Build Tools** with the *Desktop development with C++* workload, for cl, the Windows SDK and C runtime: [download](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
- Or [portable-msvc.py](https://gist.github.com/mmozeiko/7f3162ec2988e81e56d5c4e22cde9977). Run `setup_x64.bat` from its output folder before building.
- Optional **Clang**: install it from the [LLVM releases](https://github.com/llvm/llvm-project/releases) (`LLVM-<version>-win64.exe`) or with:
  ```
  winget install LLVM.LLVM
  ```

## Build

### Visual Studio Build Tools + Clang
```
cd zExample
Build.bat release run
```

### Visual Studio Build Tools
Using Visual Studio 2022 Developer Command Prompt or Visual Studio 2022 Developer PowerShell
```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
The executables can then be found in `.\zExample\Bin\` and `.\DX11Example\Bin\`

## Credits

Some of the bitmap fonts in `zExample/R_Fonts.hpp` are from VileR's [VGA text mode fonts](https://github.com/viler-int10h/vga-text-mode-fonts) collection.
