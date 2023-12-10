#!/bin/sh

clang-format --version

find trantor -name *.h -o -name *.cc -exec dos2unix {} \;
find trantor -name *.h -o -name *.cc|xargs clang-format -i -style=file

cmake-format --version
find . -name CMakeLists.txt|xargs cmake-format -i