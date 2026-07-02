# libctru - CTR User Library

![Build Status](https://github.com/devkitPro/libctru/actions/workflows/build.yaml/badge.svg)

Library for writing user mode ARM11 code for the 3DS (CTR)

This library aims to provide the foundations necessary to write 3DS Homebrew, and straightforwardly access the different functionalities provided by the 3DS operating system.
It is not meant to provide higher level functions; to put things in perspective, the purpose of libctru would be to sit between the OS and a possible port of SDL rather than replace it.

*(Originally located at github.com/smealum/ctrulib)*

# Differences to Normal libctru

- The PICA200 GPU driver in gpu.h/gpu.c has been significantly optimized and is now configurable at build time.
- VRAM.h has an additional function to query free space per-bank.
- The following build flags were added to the Makefile:
  - `GPUCMD_ENABLE_BOUNDS_CHECKS (Default 1)`: If 1, the GPU driver will avoid exceeding the bounds of its buffer. If 0, this functionality is disabled. Disabling this can cause memory corruption if the buffer is not large enough! See Footnote 2.
  - `GPUCMD_ENABLE_ZERO_PADDING (Default 0)`: If 1, the GPU driver will pad odd-sized GPU commands with zeroes, mirroring stock libctru behavior. If 0, commands are still aligned, but padding data is not zeroed. This seems to be safe and improves performance. See Footnote 2.
  - `GPUCMD_INLINE_THRESH (Default 6)`: adjusts the auto-inline threshold for applicable GPU driver calls. Writes smaller or equal to this number are inlined to improve performance. Set to -1 to disable. See Footnote 1.
  - `ENABLE_LTO (Default 0)`: enables building libctru itself with LTO.

Footnote 1: This flag affects a header file with inline functions. Because these inline functions are compiled
wherever they are called and NOT necessarily just within libctru, this value must also be set by your project, whether that be in your makefile or defined in the file itself. Care should be taken to keep this flag synchronized across all separate compilations in your project.

Footnote 2: This flag affects both header files with inline functions and source files. In addition to following footnote 1, this file affects libctru itself.

# How to Modify Your Codebase for This Fork

No breaking changes have been made, so existing projects should build without issue.

To achieve the best performance, you should:
- Replace small GPUCMD_Add calls with their inline versions or try the auto versions.
- Find out how big your project's GPU command lists actually are, ensure that buffers are large enough, and disable GPUCMD bounds checks.
- Enable LTO at build time for a small speedup.

# Setup

libctru is just a library and needs a toolchain to function. devkitARM (created by [devkitPro](http://devkitpro.org)) is the officially supported ARM cross compiling toolchain, which provides the framework necessary to supply a usable POSIX-like environment, with working C and C++ standard libraries; as well as the tools required to compile homebrew in the 3DSX format, and assemble GPU shaders. The use of other ARM toolchains is severely discouraged.

The most recent version of devkitARM is always recommended. The [installers/setup scripts](https://devkitpro.org/wiki/devkitPro_pacman) supplied by devkitPro install a prebuilt copy of the latest stable version of libctru, which is recommended for general use. Please note that devkitPro has a policy of keeping legacy code to a minimum, so a library upgrade may result in older code failing to compile or behave properly. Developers are encouraged to keep their code working with the latest versions of the tools and libraries.

You may find instructions on how to install devkitARM on [the devkitPro Wiki](http://devkitpro.org/wiki/Getting_Started).

# Documentation

The documentation is automatically built upon release and can be found at the following url: https://devkitpro.github.io/libctru/

# License

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any
  damages arising from the use of this software.

  Permission is granted to anyone to use this software for any
  purpose, including commercial applications, and to alter it and
  redistribute it freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you
     must not claim that you wrote the original software. If you use
     this software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and
     must not be misrepresented as being the original software.
  3. This notice may not be removed or altered from any source
     distribution.
