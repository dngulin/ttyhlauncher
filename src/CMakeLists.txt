add_executable(ttyhlauncher WIN32 MACOSX_BUNDLE
  "main.cpp"
  "launcherwindow.cpp"
  "skinuploaddialog.cpp"
  "settingsdialog.cpp"
  "updatedialog.cpp"
  "feedbackdialog.cpp"
  "aboutdialog.cpp"
  "oldsettings.cpp"
  "oldlogger.cpp"
  "util.cpp"
  "licensedialog.cpp"
  "jsonparser.cpp"
  "libraryinfo.cpp"
  "fileinfo.cpp"
  "gamerunner.cpp"
  "filefetcher.cpp"
  "datafetcher.cpp"
  "hashchecker.cpp"
  "logview.cpp"
  "selfupdatedialog.cpp"
  "storesettingsdialog.cpp"
  "storemanagedialog.cpp"
  "storeinstalldialog.cpp"
  "installinfo.cpp"
  "fileinstaller.cpp"
  "json/downloadinfo.cpp"
  "json/assetdownloadinfo.cpp"
  "json/checkinfo.cpp"
  "json/assetsindex.cpp"
  "json/libraryrule.cpp"
  "json/libraryinfo.cpp"
  "json/versionindex.cpp"
  "json/prefixinfo.cpp"
  "json/prefixesindex.cpp"
  "json/prefixversionsindex.cpp"
  "json/dataindex.cpp"
  "logs/logger.cpp"
  "logs/namedlogger.cpp"
)
add_dependencies(ttyhlauncher update_qm)

set_target_properties(ttyhlauncher PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(ttyhlauncher PRIVATE
  ${CONFIG_INCLUDE_DIR} ${QUAZIP_INCLUDE_DIRS}
)

target_link_libraries(ttyhlauncher
  Qt5::Gui Qt5::Widgets Qt5::Network ${QUAZIP_LIBRARIES}
)

if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
  target_compile_options(ttyhlauncher PRIVATE "-Wall" "-Wpedantic")
endif()

if(UNIX AND NOT APPLE)
  install(TARGETS ttyhlauncher RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
