#!/bin/sh

clang-format --version
echo "cmake-format version: $(cmake-format --version)"

find trantor -name *.h -o -name *.cc -exec dos2unix {} \;

find trantor -name *.h -o -name *.cc|xargs clang-format -i -style=file \
&& find . -maxdepth 1 -name CMakeLists.txt|xargs cmake-format -i \
&& find trantor -name CMakeLists.txt|xargs cmake-format -i \
&& find cmake -name *.cmake -o -name *.cmake.in|xargs cmake-format -i \
&& find cmake_modules -name *.cmake -o -name *.cmake.in|xargs cmake-format -i