macro(set_crypto)
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
    PRIVATE_HEADERS
    trantor/utils/crypto/md5.h
    trantor/utils/crypto/sha1.h
    trantor/utils/crypto/sha256.h
    trantor/utils/crypto/sha3.h
    trantor/utils/crypto/blake2.h
  )

endmacro()

macro(set_openssl)
  message(STATUS "Trantor using SSL library: openssl")

  target_compile_definitions(${PROJECT_NAME} PRIVATE TRANTOR_TLS_PROVIDER="OpenSSL")
  target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENSSL)

  target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

  if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Crypt32 Secur32)
  endif()

  list(
    APPEND
    TRANTOR_SOURCES
    trantor/net/inner/tlsprovider/OpenSSLProvider.cc
    trantor/utils/crypto/openssl.cc
  )

endmacro()

macro(set_botan)
  message(STATUS "Trantor using SSL library: botan")
  target_compile_definitions(${PROJECT_NAME} PRIVATE TRANTOR_TLS_PROVIDER="Botan")
  target_compile_definitions(${PROJECT_NAME} PRIVATE USE_BOTAN)

  # conan target is different, always lower case
  if(TARGET botan::botan)
    target_link_libraries(${PROJECT_NAME} PRIVATE botan::botan)
    target_compile_features(botan::botan INTERFACE cxx_std_20)
  else()
    target_link_libraries(${PROJECT_NAME} PRIVATE Botan::Botan)
  endif()

  list(
    APPEND
    TRANTOR_SOURCES
    trantor/net/inner/tlsprovider/BotanTLSProvider.cc
    trantor/utils/crypto/botan.cc
  )

endmacro()

# ######################################################################################################################
# Set TLS provider
message(STATUS "Setting TLS: ${TRANTOR_USE_TLS}  TRANTOR_TLS_PROVIDER: ${TRANTOR_TLS_PROVIDER}")

# Checking valid TLS providers
set(VALID_TLS_PROVIDERS
    "none"
    "openssl"
    "botan-3"
    "auto"
)
if(NOT
   "${TRANTOR_TLS_PROVIDER}"
   IN_LIST
   VALID_TLS_PROVIDERS
)
  message(FATAL_ERROR "Invalid TLS provider: ${TRANTOR_TLS_PROVIDER}\n"
                      "Valid TLS providers are: ${VALID_TLS_PROVIDERS}"
  )
endif()

if(TRANTOR_TLS_PROVIDER STREQUAL "openssl") # openssl
  find_package(OpenSSL REQUIRED)
  set_openssl()

elseif(TRANTOR_TLS_PROVIDER STREQUAL "botan-3") # botan 3
  find_package(Botan 3 REQUIRED)

  set_botan()

elseif(TRANTOR_TLS_PROVIDER STREQUAL "none") # none
  set_crypto()

elseif(TRANTOR_TLS_PROVIDER STREQUAL "auto") # auto
  find_package(OpenSSL)

  if(OPENSSL_FOUND)
    set_openssl()
  else()
    find_package(Botan 3)

    if(BOTAN_FOUND)
      set_botan()
    else()
      set_crypto()
    endif() # botan 3

  endif() # openssl

else() # none
  set_crypto()
endif()
