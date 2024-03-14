# TRANTOR

[![Build Ubuntu gcc](https://github.com/an-tao/trantor/actions/workflows/ubuntu-gcc.yml/badge.svg)](https://github.com/an-tao/trantor/actions)
[![Build Macos clang](https://github.com/an-tao/trantor/actions/workflows/macos-clang.yml/badge.svg)](https://github.com/an-tao/trantor/actions)
[![Build RockyLinux gcc](https://github.com/an-tao/trantor/actions/workflows/rockylinux-gcc.yml/badge.svg)](https://github.com/an-tao/trantor/actions)
[![Build Windows msvc](https://github.com/an-tao/trantor/actions/workflows/windows-msvc.yml/badge.svg)](https://github.com/an-tao/trantor/actions)

## Overview
A non-blocking I/O cross-platform TCP network library, using C++14.  
Drawing on the design of Muduo Library

## Supported platforms
- Linux
- MacOS
- UNIX(BSD)
- Windows

## Feature highlights
- Non-blocking I/O
- cross-platform
- Thread pool
- Lock free design
- Support SSL
- Server and Client


## Build
```shell
git clone https://github.com/an-tao/trantor.git
cd trantor
cmake -B build -H.
cd build 
make -j
```
#### Building options

Trantor provides some building options, you can enable or disable them by setting the corresponding variables to `ON` or `OFF` in the cmake command line, cmake file etc...

| Option name | Description | Default value |
| :--- | :--- | :--- |
| BUILD_SHARED_LIBS | Build Trantor as a shared lib |  OFF | 
| BUILD_TESTING | Build tests |  OFF | 
| BUILD_DOC | Build Doxygen documentation |  OFF |
| TRANTOR_USE_SPDLOG | Allow using the spdlog logging library | ON | 
| TRANTOR_USE_C-ARES | Allow using C-ARES |  ON |
| TRANTOR_TLS_PROVIDER | TLS provider for trantor. Valid options are 'none', 'openssl', 'botan-3', 'auto'.   | auto  | 

## Licensing
Trantor - A non-blocking I/O based TCP network library, using C++14. 

Copyright (c) 2016-2021, Tao An.  All rights reserved.

https://github.com/an-tao/trantor

For more information see [License](License)

## Community
[Gitter](https://gitter.im/drogon-web/community)

## Documentation
[DocsForge](https://trantor.docsforge.com/)
