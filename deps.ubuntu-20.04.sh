#!/usr/bin/env bash

# This script is used to install dependencies on Ubuntu 20.04 or newer

# Installing packages might fail as the github image becomes outdated
sudo apt update
sudo apt install libgtest-dev libspdlog-dev libfmt-dev libc-ares-dev openssl libssl-dev -y
