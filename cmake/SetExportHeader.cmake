include(GenerateExportHeader)
set(TRANTOR_EXPORT_HEADER ${CMAKE_CURRENT_BINARY_DIR}/exports/trantor/exports.h)
generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME ${TRANTOR_EXPORT_HEADER})
set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_NAME Trantor)
