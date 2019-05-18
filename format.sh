#!/bin/sh

clang-format --version
find trantor -name *.h -o -name *.cc|xargs clang-format -i -style=file
