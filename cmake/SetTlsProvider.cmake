# Set TLS provider
message(STATUS "Setting TLS: ${TRANTOR_USE_TLS} TRANTOR_TLS_PROVIDER: ${TRANTOR_TLS_PROVIDER}")

# Checking valid TLS providers
set(VALID_TLS_PROVIDERS
    "auto"
    "openssl"
    "botan-3"
    "none"
)
if(NOT
   "${TRANTOR_TLS_PROVIDER}"
   IN_LIST
   VALID_TLS_PROVIDERS
)
  message(FATAL_ERROR "⛔Invalid TLS provider: ${TRANTOR_TLS_PROVIDER}, Valid TLS providers are: ${VALID_TLS_PROVIDERS}")
endif()

if(TRANTOR_TLS_PROVIDER STREQUAL "openssl")
  include(cmake/SetOpenssl.cmake)
elseif(TRANTOR_TLS_PROVIDER STREQUAL "botan-3")
  set(Botan_VERSION_MAJOR 3)
  include(cmake/SetBotan.cmake)
elseif(TRANTOR_TLS_PROVIDER STREQUAL "none")
  include(cmake/SetCrypto.cmake)
elseif(TRANTOR_TLS_PROVIDER STREQUAL "auto")

  foreach(item VALID_TLS_PROVIDERS)
    if(${item} STREQUAL "auto")
      continue()
    endif()

    if(${item} STREQUAL "openssl")
      find_package(OpenSSL)
      if(OPENSSL_FOUND)
        include(cmake/SetOpenssl.cmake)
      else()
        continue()
      endif()
    endif()

    if(${item} STREQUAL "botan-3")
      find_package(Botan 3)
      if(Botan_FOUND)
        include(cmake/SetBotan.cmake)
      else()
        continue()
      endif()
    endif()

    if(${item} STREQUAL "none")
      include(cmake/SetCrypto.cmake)
    endif()
  endforeach(item)

else() # none
  include(cmake/SetCrypto.cmake)
endif()
