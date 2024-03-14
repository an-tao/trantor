#!/bin/bash

# This script is used to install dependencies on MacOS 12.0 or newer
brew update
# TODO: botan v3, botan@3.3.0 build failed, if newer version work, remove @3.2.0
brew install googletest spdlog c-ares openssl@3  botan@3.2.0