target_sources(ttyhlauncher PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc")

if(UNIX AND NOT APPLE)
  install(FILES "unix/ttyhlauncher.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
  )
  install(FILES "unix/ttyhlauncher.svg"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
  )
elseif(WIN32)
  set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/icon.rc")

  file(WRITE ${RC_FILE}
    "IDI_ICON1 ICON \"${CMAKE_CURRENT_SOURCE_DIR}/icon.ico\""
  )
  target_sources(ttyhlauncher PUBLIC ${RC_FILE})
elseif(APPLE)
  set(MACOSX_BUNDLE_ICON_FILE "icon.icns")
  set(ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/icon.icns")

  set_source_files_properties(${ICON_FILE} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
  )
  target_sources(ttyhlauncher PUBLIC ${ICON_FILE})

  set(MACOSX_BUNDLE_BUNDLE_NAME "ttyhlauncher")
  set(MACOSX_BUNDLE_EXECUTABLE_NAME "ttyhlauncher")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")

  set_target_properties(ttyhlauncher PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
  )
endif()
