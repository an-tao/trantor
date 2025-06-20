message(STATUS "Trantor using SSL library: None")
target_compile_definitions(${PROJECT_NAME} PRIVATE TRANTOR_TLS_PROVIDER="None")

list(
  APPEND
  TRANTOR_SOURCES
  trantor/utils/crypto/md5.cc
  trantor/utils/crypto/sha1.cc
  trantor/utils/crypto/sha256.cc
  trantor/utils/crypto/sha3.cc
  trantor/utils/crypto/blake2.cc
)
list(
  APPEND
  private_headers
  trantor/utils/crypto/md5.h
  trantor/utils/crypto/sha1.h
  trantor/utils/crypto/sha256.h
  trantor/utils/crypto/sha3.h
  trantor/utils/crypto/blake2.h
)
