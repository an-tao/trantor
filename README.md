# TRANTOR

[![Build Status](https://travis-ci.org/an-tao/trantor.svg?branch=master)](https://travis-ci.org/an-tao/trantor)
[![Build status](https://ci.appveyor.com/api/projects/status/yn8xunsubn37pi1u/branch/master?svg=true)](https://ci.appveyor.com/project/an-tao/trantor/branch/master)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/an-tao/trantor.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/an-tao/trantor/context:cpp)


## Overview
A non-blocking I/O cross-platform TCP network library, using C++14.  
Drawing on the design of Muduo Library

## suported platforms
- Linux
- Mac Os
- Solaris
- Windows

## Feature highlights
- non-blocking I/O
- cross-platform
- Thread pool
- Lock free design
- Support SSL
- Thread pool
- Server and Client

## Dependence
cmake 3.5 or newer;    
- linux kernel 2.6.9 x86-64 or newer;
- Mac Os
- Solaris
- Windows

## Build
```c++
git clone https://github.com/an-tao/trantor.git
cd trantor
cmake -Bbuild -H.
cd build 
make -j9
```

## Licensing
Trantor - A non-blocking I/O based TCP network library, using C++14/17, 
Copyright (c) 2016-2020, Tao An.  All rights reserved.
https://github.com/an-tao/trantor
For more information see [License](License)

## Community
[Gitter](https://gitter.im/drogon-web/community)

## Documentation
Doxygen documented


