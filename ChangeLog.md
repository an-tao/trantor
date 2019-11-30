# Changelog
All notable changes to this project will be documented in this file.

## [Unreleased]

## [1.0.0-rc8] - 2019-11-30

### API change list

- Add the isSSLConnection() method to the TcpConnection class

### Changed

- Use the std::chrono::steady_clock for timers


## [1.0.0-rc7] - 2019-11-21

### Changed

- Modify some code styles

## [1.0.0-rc6] - 2019-10-4

### API change list

- Add index() interface to the EventLoop class.

### Changed

- Fix some compilation warnings.

- Modify the CMakeLists.txt

## [1.0.0-rc5] - 2019-08-24

### API change list

- Remove the resolve method from the InetAddress class.

### Added

- Add the Resolver class that provides high-performance DNS functionality(with c-ares library)
- Add some unit tests.
  
## [1.0.0-rc4] - 2019-08-08

### API change list

- None

### Changed

- Add TrantorConfig.cmake so that users can use trantor with the `find_package(Trantor)` command.

### Fixed

- Fix an SSL error (occurs when sending large data via SSL).

## [1.0.0-rc3] - 2019-07-30

### API change list

- TcpConnection::setContext, TcpConnection::getContext, etc.
- Remove the config.h from public API.

### Changed

- Modify the CMakeLists.txt.
- Modify some log output.
- Remove some unnecessary `std::dynamic_pointer_cast` calls.

## [1.0.0-rc2] - 2019-07-11

### Added

- Add bytes statistics methods to the TcpConnection class.
- Add the setIoLoopThreadPool method to the TcpServer class.

### Changed

- Ignore SIGPIPE signal when using the TcpClient class.
- Enable TCP_NODELAY by default (for higher performance).

## [1.0.0-rc1] - 2019-06-11

[Unreleased]: https://github.com/an-tao/trantor/compare/v1.0.0-rc8...HEAD

[1.0.0-rc8]: https://github.com/an-tao/trantor/compare/v1.0.0-rc7...v1.0.0-rc8

[1.0.0-rc7]: https://github.com/an-tao/trantor/compare/v1.0.0-rc6...v1.0.0-rc7

[1.0.0-rc6]: https://github.com/an-tao/trantor/compare/v1.0.0-rc5...v1.0.0-rc6

[1.0.0-rc5]: https://github.com/an-tao/trantor/compare/v1.0.0-rc4...v1.0.0-rc5

[1.0.0-rc4]: https://github.com/an-tao/trantor/compare/v1.0.0-rc3...v1.0.0-rc4

[1.0.0-rc3]: https://github.com/an-tao/trantor/compare/v1.0.0-rc2...v1.0.0-rc3

[1.0.0-rc2]: https://github.com/an-tao/trantor/compare/v1.0.0-rc1...v1.0.0-rc2

[1.0.0-rc1]: https://github.com/an-tao/trantor/releases/tag/v1.0.0-rc1
