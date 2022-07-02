# Changelog
All notable changes to this project will be documented in this file.

## [Unreleased]

## [1.5.5] - 2022-02-19

### API changes list

### Changed

- Move EventLoop::runAfter to a template.

- Remove an assertion when removing channels.

- Prevent TcpClient::removeConnection call on deleted TcpClient instance.

- Wait for loop to exit in EventLoop destructor.

- Add r-reference version of set-callback methods to TcpConnectionImpl.

### Fixed

- Fix a bug when closing connections on Windows/MacOS.

- Fix logger causes if statement mismatch.

## [1.5.4] - 2021-12-10

### API changes list

### Changed

- Correctly handle the error of the getaddrinfo function

### Fixed

- Fix the error when sending partial files

## [1.5.3] - 2021-11-28

### API changes list

- TcpClientImpl support SSL client certificate

### Changed

- Allow RVO in fromDbStringLocal

### Fixed

- Make sure resolvers are added when C-Ares is manually disabled

## [1.5.2] - 2021-10-17

### API changes list

### Changed

- Disable setting SSL Configs when using LibreSSL

- cmake: Use GNUInstallDirs to figure out install dirs

- support HaikuOS

- Improve Error handling for certificates/private keys

- Make c-ares support optional when building

- Use locale.h

- Assert fd >= 0 in updateChannel()

- Add Clang support for -Wall -Wextra -Werror; fix -Wunused-parameter

### Fixed

- Fix a small memory leak

- Fix errors in log macros

- Fix a race condition when TcpClient destroyed before connected

- Fix the error of calling removeAndResetChannel twice

- Fix a bug when EAGAIN on reading sockets

- Fix compilation warnings

- Fix a potential race condition

## [1.5.1] - 2021-08-08

### API changes list

### Changed

- Fix warning C4244 in MSVC

- Disable strict compiler check on Windows with GCC

- Add support for paths containing unicode characters on Windows

- Add BUILD_DOC cmake option (doxygen)

- Use make_shared instead of shared_ptr(new)

- Detect and handle MinGW

### Fixed

- Fix the destructor of AresResolver

- Fix memory leak in NormalResolver

## [1.5.0] - 2021-06-18

### API changes list

- Enable multiple log files or streams.

- Add SSL_CONF_cmd support.

- Add runOnQuit to the EventLoop class.

### Changed

- Export the FixedBuffer.

- Added a try_compile block to detect if we need to link against atomic.

## [1.4.1] - 2021-05-15

### Changed

- Add github actions of Windows.

- Modify the way the log file is opened.

- Add version/soversion to shared library.

- Use double instead of long double as the type for timer durations.

### Fixed

- Fix a bug in the TcpConnectionImpl class.

- Fix constructing Date in a daylight saving timezone.

- GNU: -Wall -Wextra -Werror; fix related warnings.

- Add wincrypt.h include for Windows.

## [1.4.0] - 2021-04-09

### API changes list

- Add isUnspecified() to indicate if IP parsing failed.

- Add exports macro to allow Shared Library with hidden symbols by default.

### Changed

- Modify the AsyncFileLogger destructor.

### Fixed

- Recycle TimerID in the TimerQueue.

## [1.3.0] - 2021-03-05

### API changes list

- Add secondsSinceEpoch to trantor::Date.

- Rename the 'bzero' method of the FixedBuffer class to 'zeroBuffer'.

- Add SNI support to TcpClient.

- Add SSL certificate validation.

### Changed

- Change README.md.

## [1.2.0] - 2021-01-16

### API changes list

- Add LOG_IF and DLOG like glog lib.

### Changed

- Enable github actions.

- Add support for VS2019.

- Modify the LockFreeQueue.

### Fixed

- Fix MinGW error with inet_ntop and inet_pton.

- Fix a macro regression when using MSVC.

## [1.1.1] - 2020-12-12

### Changed

- Add Openbsd support.

## [1.1.0] - 2020-10-24

### Changed

- Disable TLS 1.0 and 1.1 by default.

- Use explicit lambda capture lists.

### Fixed

- Fix a bug in the Date::fromDbStringLocal() method.

## [1.0.0] - 2020-9-27

### API changes list

- Add the address() method to the TcpServer class.

- Change some internal methods from public to private in the Channel class.

### Changed

- Update the wepoll library.

- Add comments in public header files.

## [1.0.0-rc16] - 2020-8-15

### Fixed

- Fix a bug when sending big files on Windows.

### API changes list

- Add updateEvents() method to the Channel class.

## [1.0.0-rc15] - 2020-7-16

### Fixed

- Fix installation errors of shared library.

## [1.0.0-rc14] - 2020-6-14

### API changes list

- Add the moveToCurrentThread() method to EventLoop.

### Changed

- Optimized LockFreeQueue by Reducing Object Construction.

### Fixed

- Fix a bug when sending a file.

## [1.0.0-rc13] - 2020-5-23

### API changes list

- Make the Channel class as a part of the public API.

## [1.0.0-rc12] - 2020-5-22

### API changes list

- Add a method to show if the c-ares library is used

### Fixed

- Fix a bug in SSL mode (#85)

- Use SOCKET type in windows for x86-windows compilation

- Use env to find bash in build.sh script to support FreeBSD

## [1.0.0-rc11] - 2020-4-27

### API changes list

- Add fromDbStringLocal() method to the Date class

### Fixed

- Fix a race condition of TimingWheel class

- Fix localhost resolving on windows

## [1.0.0-rc10] - 2020-3-28

### API changes list

- Add the send(const void *, size_t) method to the TcpConnection class

- Add the send(const MsgBufferPtr &) method to TcpConnection class

- Add stop() method to the TcpServer class

### Changed

- Compile wepoll directly into trantor (Windows)

- Add CI for Windows

- Make CMake install files relocatable

- Modify the Resolver class

## [1.0.0-rc9] - 2020-2-17

### API changes list

- Add support for a delayed SSL handshake

- Change a method name of EventLoopThreadPool(getLoopNum() -> size())

### Changed

- Port Trantor to Windows

- Use SSL_CTX_use_certificate_chain_file instead of SSL_CTX_use_certificate_file()

## [1.0.0-rc8] - 2019-11-30

### API changes list

- Add the isSSLConnection() method to the TcpConnection class

### Changed

- Use the std::chrono::steady_clock for timers


## [1.0.0-rc7] - 2019-11-21

### Changed

- Modify some code styles

## [1.0.0-rc6] - 2019-10-4

### API changes list

- Add index() interface to the EventLoop class.

### Changed

- Fix some compilation warnings.

- Modify the CMakeLists.txt

## [1.0.0-rc5] - 2019-08-24

### API changes list

- Remove the resolve method from the InetAddress class.

### Added

- Add the Resolver class that provides high-performance DNS functionality(with c-ares library)
- Add some unit tests.
  
## [1.0.0-rc4] - 2019-08-08

### API changes list

- None

### Changed

- Add TrantorConfig.cmake so that users can use trantor with the `find_package(Trantor)` command.

### Fixed

- Fix an SSL error (occurs when sending large data via SSL).

## [1.0.0-rc3] - 2019-07-30

### API changes list

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

[Unreleased]: https://github.com/an-tao/trantor/compare/v1.5.5...HEAD

[1.5.5]: https://github.com/an-tao/trantor/compare/v1.5.4...v1.5.5

[1.5.4]: https://github.com/an-tao/trantor/compare/v1.5.3...v1.5.4

[1.5.3]: https://github.com/an-tao/trantor/compare/v1.5.2...v1.5.3

[1.5.2]: https://github.com/an-tao/trantor/compare/v1.5.1...v1.5.2

[1.5.1]: https://github.com/an-tao/trantor/compare/v1.5.0...v1.5.1

[1.5.0]: https://github.com/an-tao/trantor/compare/v1.4.1...v1.5.0

[1.4.1]: https://github.com/an-tao/trantor/compare/v1.4.0...v1.4.1

[1.4.0]: https://github.com/an-tao/trantor/compare/v1.3.0...v1.4.0

[1.3.0]: https://github.com/an-tao/trantor/compare/v1.2.0...v1.3.0

[1.2.0]: https://github.com/an-tao/trantor/compare/v1.1.1...v1.2.0

[1.1.1]: https://github.com/an-tao/trantor/compare/v1.1.0...v1.1.1

[1.1.0]: https://github.com/an-tao/trantor/compare/v1.0.0...v1.1.0

[1.0.0]: https://github.com/an-tao/trantor/compare/v1.0.0-rc16...v1.0.0

[1.0.0-rc16]: https://github.com/an-tao/trantor/compare/v1.0.0-rc15...v1.0.0-rc16

[1.0.0-rc15]: https://github.com/an-tao/trantor/compare/v1.0.0-rc14...v1.0.0-rc15

[1.0.0-rc14]: https://github.com/an-tao/trantor/compare/v1.0.0-rc13...v1.0.0-rc14

[1.0.0-rc13]: https://github.com/an-tao/trantor/compare/v1.0.0-rc12...v1.0.0-rc13

[1.0.0-rc12]: https://github.com/an-tao/trantor/compare/v1.0.0-rc11...v1.0.0-rc12

[1.0.0-rc11]: https://github.com/an-tao/trantor/compare/v1.0.0-rc10...v1.0.0-rc11

[1.0.0-rc10]: https://github.com/an-tao/trantor/compare/v1.0.0-rc9...v1.0.0-rc10

[1.0.0-rc9]: https://github.com/an-tao/trantor/compare/v1.0.0-rc8...v1.0.0-rc9

[1.0.0-rc8]: https://github.com/an-tao/trantor/compare/v1.0.0-rc7...v1.0.0-rc8

[1.0.0-rc7]: https://github.com/an-tao/trantor/compare/v1.0.0-rc6...v1.0.0-rc7

[1.0.0-rc6]: https://github.com/an-tao/trantor/compare/v1.0.0-rc5...v1.0.0-rc6

[1.0.0-rc5]: https://github.com/an-tao/trantor/compare/v1.0.0-rc4...v1.0.0-rc5

[1.0.0-rc4]: https://github.com/an-tao/trantor/compare/v1.0.0-rc3...v1.0.0-rc4

[1.0.0-rc3]: https://github.com/an-tao/trantor/compare/v1.0.0-rc2...v1.0.0-rc3

[1.0.0-rc2]: https://github.com/an-tao/trantor/compare/v1.0.0-rc1...v1.0.0-rc2

[1.0.0-rc1]: https://github.com/an-tao/trantor/releases/tag/v1.0.0-rc1
