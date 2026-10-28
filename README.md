# ozd-stl

A work-in-progress base layer for C++17. Windows only for now.

## Requirements

- **Clang**: install it from the [LLVM releases](https://github.com/llvm/llvm-project/releases) (`LLVM-<version>-win64.exe`) or with:
  ```
  winget install LLVM.LLVM
  ```
- **Visual Studio Build Tools** with the *Desktop development with C++* workload, for the Windows SDK and C runtime: [download](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
- Or [portable-msvc.py](https://gist.github.com/mmozeiko/7f3162ec2988e81e56d5c4e22cde9977). Run `setup_x64.bat` from its output folder before building.

## Build

```
cd Example
Build.bat release run
```

## Credits

Some of the bitmap fonts in `Example/R_Fonts.hpp` are from VileR's [VGA text mode fonts](https://github.com/viler-int10h/vga-text-mode-fonts) collection.
