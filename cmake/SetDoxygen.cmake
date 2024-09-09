message(STATUS "Setting Doxygen documentation: ${BUILD_DOC}")

if(BUILD_DOC)
  if(WIN32)
    set(DOXYGEN_PREDEFINED _WIN32)
  endif(WIN32)

  find_package(Doxygen OPTIONAL_COMPONENTS dot dia)
  if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_BRIEF "${PROJECT_DESCRIPTION}")
    set(DOXYGEN_OUTPUT_DIRECTORY docs/${PROJECT_NAME})
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    set(DOXYGEN_STRIP_FROM_INC_PATH ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/exports)

    doxygen_add_docs(
      doc_${PROJECT_NAME}
      README.md
      ChangeLog.md
      ${TRANTOR_SOURCES}
      ${TRANTOR_EXPORT_HEADER}
      ${public_net_headers}
      ${public_utils_headers}
      ${private_headers}
      COMMENT "Generate documentation"
    )
    add_dependencies(${PROJECT_NAME} doc_${PROJECT_NAME})
  endif()

endif(BUILD_DOC)
