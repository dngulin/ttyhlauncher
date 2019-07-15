if(UNIX AND NOT APPLE)
  install(FILES "unix/ttyhlauncher.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
  )
  install(FILES "unix/ttyhlauncher.svg"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
  )
elseif(WIN32)
  set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/favicon.rc")

  file(WRITE ${RC_FILE}
    "IDI_ICON1 ICON \"${CMAKE_CURRENT_SOURCE_DIR}/favicon.ico\""
  )
  target_sources(ttyhlauncher PUBLIC ${RC_FILE})
elseif(APPLE)
  set(MACOSX_BUNDLE_ICON_FILE "favicon.icns")
  set(ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/favicon.icns")

  set_source_files_properties(${ICON_FILE} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
  )
  target_sources(ttyhlauncher PUBLIC ${ICON_FILE})
endif()
