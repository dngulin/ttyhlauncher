file(STRINGS "LINGUAS" LINGUAS REGEX "^[^#]")

string(REGEX REPLACE "([^;]+)"
  "${CMAKE_CURRENT_SOURCE_DIR}/ttyhlauncher_\\1.ts"
  TS_FILES "${LINGUAS}"
)

get_target_property(TS_FILES_SOURCE_DIR ttyhlauncher SOURCE_DIR)
get_target_property(TS_FILES_SOURCES ttyhlauncher SOURCES)

add_custom_target(update_ts
  COMMAND Qt5::lupdate "-noobsolete" ${TS_FILES_SOURCES} "-ts" ${TS_FILES}
  WORKING_DIRECTORY ${TS_FILES_SOURCE_DIR}
  BYPRODUCTS ${TS_FILES}
  VERBATIM
)

qt5_add_translation(QM_FILES ${TS_FILES})
add_custom_target(update_qm DEPENDS ${QM_FILES})

configure_file("translations.qrc.in" "translations.qrc" @ONLY)

target_sources(ttyhlauncher PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/translations.qrc")
