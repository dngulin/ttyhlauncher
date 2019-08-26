add_executable(ttyhlauncher WIN32 MACOSX_BUNDLE
  "main.cpp"
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
  "settings/settingsdata.cpp"
  "settings/settingsmanager.cpp"
  "versions/versionsmanager.cpp"
  "versions/fullversionid.cpp"
  "storage/filechecker.cpp"
  "storage/downloader.cpp"
  "profiles/profilesmanager.cpp"
  "profiles/profiledata.cpp"
  "profiles/filesindex.cpp"
  "profiles/profilerunner.cpp"
  "master/ttyhclient.cpp"
  "news/newsfeed.cpp"
  "utils/network.cpp"
  "utils/platform.cpp"
  "utils/downloadfilereply.cpp"
  "utils/zip.cpp"
  "utils/migrations.cpp"
  "ui/mainwindow.cpp"
  "ui/profiledialog.cpp"
  "ui/skindialog.cpp"
  "ui/aboutdialog.cpp"
  "launcher.cpp"
)
add_dependencies(ttyhlauncher update_qm)

target_sources(ttyhlauncher PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/ui/mainwindow.ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/ui/profiledialog.ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/ui/skindialog.ui"
  "${CMAKE_CURRENT_SOURCE_DIR}/ui/aboutdialog.ui"
)

set_target_properties(ttyhlauncher PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(ttyhlauncher PRIVATE ${CONFIG_INCLUDE_DIR})

target_link_libraries(ttyhlauncher
  Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Concurrent PkgConfig::Minizip
)

if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
  target_compile_options(ttyhlauncher PRIVATE "-Wall" "-Wpedantic")
endif()

if(UNIX AND NOT APPLE)
  install(TARGETS ttyhlauncher RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
