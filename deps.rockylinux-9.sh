#!/usr/bin/env bash

# This script is used to install dependencies on RockyLinux 9 or newer
dnf install epel-release -y
dnf update
dnf install gtest-devel spdlog-devel fmt-devel c-ares-devel openssl-devel -y
