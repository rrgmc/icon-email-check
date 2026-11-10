# The app's code, except main.cpp, is a static library, so the tests link it.
qt_add_library(iec_core STATIC
    App.cpp App.h
    Settings.cpp Settings.h
    SingleInstance.cpp SingleInstance.h
    accounts/Account.cpp accounts/Account.h
    accounts/SecretStore.cpp accounts/SecretStore.h
    mail/CheckResult.h
    mail/GmailApi.cpp mail/GmailApi.h
    mail/GmailChecker.cpp mail/GmailChecker.h
    mail/GoogleSignIn.cpp mail/GoogleSignIn.h
    mail/ImapChecker.cpp mail/ImapChecker.h
    mail/ImapProtocol.cpp mail/ImapProtocol.h
    mail/MailChecker.h
    mail/MailMonitor.cpp mail/MailMonitor.h
    mail/MimeHeaders.cpp mail/MimeHeaders.h
    mail/Network.cpp mail/Network.h
    platform/ActiveSpace.cpp platform/ActiveSpace.h
    platform/Foreground.cpp platform/Foreground.h
    platform/LoginItem.cpp platform/LoginItem.h
    platform/Notifier.cpp platform/Notifier.h
    platform/Tray.cpp platform/Tray.h
    platform/TrayBadge.cpp platform/TrayBadge.h
    ui/AccountDialog.cpp ui/AccountDialog.h
    ui/MainWindow.cpp ui/MainWindow.h
)
target_include_directories(iec_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(iec_core PUBLIC Qt6::Widgets Qt6::Network Qt6::Svg qt6keychain)
target_compile_definitions(iec_core PUBLIC
    IEC_VERSION="${PROJECT_VERSION}"
    IEC_GOOGLE_CLIENT_ID="${IEC_GOOGLE_CLIENT_ID}"
    IEC_GOOGLE_CLIENT_SECRET="${IEC_GOOGLE_CLIENT_SECRET}")
iec_set_warnings(iec_core)

# macOS code in Objective-C++, built with ARC.
if(APPLE)
    set(mac_sources
        platform/ActiveSpaceMac.mm platform/LoginItemMac.mm platform/NotifierMac.mm platform/TrayMac.mm)
    target_sources(iec_core PRIVATE ${mac_sources})
    set_source_files_properties(${mac_sources} PROPERTIES COMPILE_OPTIONS -fobjc-arc)
    target_link_libraries(iec_core PRIVATE
        "-framework AppKit" "-framework ServiceManagement" "-framework UserNotifications")
endif()

qt_add_resources(iec_core icons
    PREFIX /icons
    BASE "${PROJECT_SOURCE_DIR}/resources/icons"
    FILES
        "${PROJECT_SOURCE_DIR}/resources/icons/app.svg"
        "${PROJECT_SOURCE_DIR}/resources/icons/tray.svg"
        "${PROJECT_SOURCE_DIR}/resources/icons/tray-error.svg"
        "${PROJECT_SOURCE_DIR}/resources/icons/tray-mono.svg"
        "${PROJECT_SOURCE_DIR}/resources/icons/tray-mono-unread.svg"
        "${PROJECT_SOURCE_DIR}/resources/icons/tray-mono-error.svg")

qt_add_executable(IconEmailCheck WIN32 MACOSX_BUNDLE main.cpp)
if(WIN32)
    set(IEC_ICO_PATH "${PROJECT_SOURCE_DIR}/resources/app/app.ico")
    configure_file("${PROJECT_SOURCE_DIR}/resources/app/app.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/app.rc" @ONLY)
    target_sources(IconEmailCheck PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/app.rc")
elseif(APPLE)
    # The bundle's icon and identifier. The Finder and keychain prompts show
    # the icon, and the keychain and Login Items know the app by its identifier.
    set(bundle_icon "${PROJECT_SOURCE_DIR}/resources/app/app.icns")
    set_source_files_properties("${bundle_icon}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(IconEmailCheck PRIVATE "${bundle_icon}")
    set_target_properties(IconEmailCheck PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/cmake/Info.plist.in"
        MACOSX_BUNDLE_ICON_FILE app.icns
        MACOSX_BUNDLE_GUI_IDENTIFIER com.rrgmc.iconemailcheck
        MACOSX_BUNDLE_BUNDLE_NAME "Icon Email Check"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2026 rrgmc")
endif()
target_link_libraries(IconEmailCheck PRIVATE iec_core)
iec_set_warnings(IconEmailCheck)
iec_add_bin_folder(IconEmailCheck)

# Installs the app: the bundle on macOS; the executable, a .desktop file and
# the icon on Linux; the executable on Windows. Qt's deploy script copies the
# Qt libraries next to it on macOS and Windows.
include(GNUInstallDirs)
install(TARGETS IconEmailCheck
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(UNIX AND NOT APPLE)
    install(FILES "${PROJECT_SOURCE_DIR}/resources/app/icon-email-check.desktop"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES "${PROJECT_SOURCE_DIR}/resources/icons/app.svg"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME icon-email-check.svg)
else()
    qt_generate_deploy_app_script(TARGET IconEmailCheck OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
    install(SCRIPT "${deploy_script}")
endif()
