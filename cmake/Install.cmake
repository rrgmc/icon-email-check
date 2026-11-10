# The install and the packages.
#
# `cmake --install <build> --component App --prefix <folder>` makes a
# self-contained app: Qt's deploy script copies the Qt libraries and plugins
# next to it. `cpack` in the build folder then makes the package: an NSIS setup
# on Windows, a disk image on macOS and a .deb package on Linux. The release
# workflow makes the macOS disk image with tools/package-macos.sh instead,
# after it signs the installed bundle.
#
# Every rule here is in the component App, and the package holds only that
# component. qtkeychain's own install rules, for its library and headers, stay
# out of it.

set(IEC_LINUX_DIR opt/icon-email-check)

# iec_add_install(<target>)
function(iec_add_install target)
    if(WIN32 OR APPLE)
        set(app_dir .)
    else()
        # The app with its own Qt in /opt/icon-email-check, and the files the
        # desktop needs under /usr. A Qt of the system cannot then clash with
        # the one the app was built against.
        set(app_dir "${IEC_LINUX_DIR}")
    endif()

    install(TARGETS ${target}
        BUNDLE DESTINATION . COMPONENT App
        RUNTIME DESTINATION "${app_dir}/bin" COMPONENT App)
    install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" DESTINATION "${app_dir}" COMPONENT App)

    # The Visual C++ runtime comes from InstallRequiredSystemLibraries below.
    # Qt Widgets on Direct3D needs neither the shader compilers nor the
    # software OpenGL.
    set(deploy_options)
    if(WIN32)
        set(deploy_options --no-compiler-runtime --no-system-d3d-compiler
            --no-system-dxc-compiler --no-opengl-sw)
    endif()
    qt_generate_deploy_app_script(
        TARGET ${target}
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
        DEPLOY_TOOL_OPTIONS ${deploy_options})
    # Qt's deploy script copies into the install prefix. On Linux the app's
    # folder is below it, so the prefix points there while the script runs.
    # CPack sets DESTDIR for the .deb, so it goes into the prefix for that time.
    if(NOT app_dir STREQUAL ".")
        install(CODE "
            set(iec_install_prefix \"\${CMAKE_INSTALL_PREFIX}\")
            set(iec_destdir \"\$ENV{DESTDIR}\")
            set(CMAKE_INSTALL_PREFIX \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${app_dir}\")
            set(ENV{DESTDIR} \"\")
        " COMPONENT App)
    endif()
    install(SCRIPT "${deploy_script}" COMPONENT App)
    if(NOT app_dir STREQUAL ".")
        install(CODE "
            set(ENV{DESTDIR} \"\${iec_destdir}\")
            set(CMAKE_INSTALL_PREFIX \"\${iec_install_prefix}\")
        " COMPONENT App)
    endif()

    if(WIN32)
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION bin)
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT App)
        include(InstallRequiredSystemLibraries)
    elseif(NOT APPLE)
        install(FILES "${PROJECT_SOURCE_DIR}/resources/linux/icon-email-check.desktop"
            DESTINATION usr/share/applications COMPONENT App)
        install(FILES "${PROJECT_SOURCE_DIR}/resources/icons/app.svg"
            DESTINATION usr/share/icons/hicolor/scalable/apps RENAME icon-email-check.svg COMPONENT App)
        install(CODE "
            file(MAKE_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/usr/bin\")
            file(CREATE_LINK ../../${IEC_LINUX_DIR}/bin/${target}
                \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/usr/bin/icon-email-check\" SYMBOLIC)
        " COMPONENT App)
    endif()

    # macdeployqt leaves the bundle's binaries unsigned, and macOS on Apple
    # silicon does not start unsigned code. This comes last, since a file added
    # after it would break the signature. The release workflow signs the
    # installed bundle again with the Developer ID.
    if(APPLE)
        set(IEC_MACOS_DISTRIBUTION_IDENTITY "" CACHE STRING
            "macOS: the certificate that signs the installed app, with the hardened runtime. Empty signs ad hoc")
        set(identity "${IEC_MACOS_DISTRIBUTION_IDENTITY}")
        if(NOT identity)
            set(identity -)
        endif()
        install(CODE "
            execute_process(
                COMMAND sh \"${PROJECT_SOURCE_DIR}/tools/sign-macos.sh\"
                    \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${target}.app\" \"${identity}\"
                COMMAND_ERROR_IS_FATAL ANY)
        " COMPONENT App)
    endif()

    iec_add_package(${target})
endfunction()

# The package, made by `cpack` in the build folder.
function(iec_add_package target)
    set(CPACK_PACKAGE_NAME icon-email-check)
    set(CPACK_PACKAGE_VENDOR rrgmc)
    set(CPACK_PACKAGE_CONTACT "rrgmc <https://github.com/rrgmc>")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Icon Email Check")
    set(CPACK_PACKAGE_EXECUTABLES ${target} "Icon Email Check")
    set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
    # The component App only, in one package.
    set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};App;/")

    if(WIN32)
        set(CPACK_GENERATOR NSIS)
        set(CPACK_PACKAGE_FILE_NAME "icon-email-check-${PROJECT_VERSION}-win64")
        set(CPACK_NSIS_PACKAGE_NAME "Icon Email Check")
        set(CPACK_NSIS_DISPLAY_NAME "Icon Email Check")
        set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\${target}.exe")
        # One key for every version, so a new setup finds and removes the old
        # version, and the setup's script can read it.
        set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "Icon Email Check")
        set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
        set(CPACK_NSIS_URL_INFO_ABOUT "${PROJECT_HOMEPAGE_URL}")
        iec_add_nsis_finish_options(${target})
    elseif(APPLE)
        set(CPACK_GENERATOR DragNDrop)
        set(CPACK_PACKAGE_FILE_NAME "icon-email-check-${PROJECT_VERSION}-macos")
        set(CPACK_DMG_VOLUME_NAME "Icon Email Check")
    else()
        set(CPACK_GENERATOR DEB)
        set(CPACK_PACKAGING_INSTALL_PREFIX /)
        # CPack stages the package in a folder of this name. The .deb itself
        # gets Debian's name, icon-email-check_<version>_amd64.deb.
        set(CPACK_PACKAGE_FILE_NAME "icon-email-check-${PROJECT_VERSION}-linux")
        set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
        set(CPACK_DEBIAN_PACKAGE_SECTION mail)
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
        # dpkg-shlibdeps finds the bundled Qt libraries here, and leaves them
        # out of the dependencies.
        set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS
            "${CMAKE_BINARY_DIR}/_CPack_Packages/Linux/DEB/${CPACK_PACKAGE_FILE_NAME}/${IEC_LINUX_DIR}/lib")
        # Qt loads OpenSSL at run time for TLS, so dpkg-shlibdeps cannot see it.
        # libsecret holds the passwords, through qtkeychain.
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3t64 | libssl3")
        set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "gnome-keyring | kwalletmanager")
    endif()

    include(CPack)
endfunction()

# The setup's icon, and two checkboxes on its last page: "Run Icon Email
# Check", and "Start Icon Email Check when I sign in". CPack has no hook for
# finish page options, so they go in with the icon code, which comes right
# before the pages; CPack then ignores CPACK_NSIS_MUI_ICON. The sign-in value
# goes in the per-user Run key, as the app's own setting writes it in
# src/platform/LoginItem.cpp; --hidden starts the app in the tray.
#
# The sign-in checkbox starts ticked on a first install, since a tray app is
# meant to run all the time, and keeps the installed version's choice on an
# upgrade. The old version's uninstaller removes the Run value before the new
# files go in, so IecReadStartAtSignIn reads it first, in .onInit.
#
# CPack's template uses Modern UI 1, which writes the last page into
# ioSpecial.ini before it calls the page's PRE function, so IecFinishPre sets
# the sign-in checkbox, Field 5, in that file.
# A macro, so the variables reach include(CPack) in the caller's scope.
macro(iec_add_nsis_finish_options target)
    file(TO_NATIVE_PATH "${PROJECT_SOURCE_DIR}/resources/app/app.ico" _iec_ico)
    set(_iec_nsis_code [=[
!define MUI_ICON "@ICO@"
!define MUI_UNICON "@ICO@"
!define MUI_FINISHPAGE_RUN ""
!define MUI_FINISHPAGE_RUN_TEXT "Run Icon Email Check"
!define MUI_FINISHPAGE_RUN_FUNCTION IecRun
!define MUI_FINISHPAGE_SHOWREADME ""
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Start Icon Email Check when I sign in"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION IecAutostart
Var IecStartAtSignIn
; Ticked on a first install, and on an upgrade of a version that started at sign-in.
Function IecReadStartAtSignIn
  Push $0
  StrCpy $IecStartAtSignIn 1
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Icon Email Check" "UninstallString"
  StrCmp $0 "" done
  StrCpy $IecStartAtSignIn 0
  ClearErrors
  ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "IconEmailCheck"
  IfErrors done
  StrCpy $IecStartAtSignIn 1
done:
  Pop $0
FunctionEnd
Function IecFinishPre
  !insertmacro INSTALLOPTIONS_WRITE "ioSpecial.ini" "Field 5" "State" $IecStartAtSignIn
FunctionEnd
Function IecAutostart
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "IconEmailCheck" '"$INSTDIR\bin\@EXE@.exe" --hidden'
FunctionEnd
; The setup runs elevated; Explorer starts the app as the signed-in user instead.
Function IecRun
  Exec '"$WINDIR\explorer.exe" "$INSTDIR\bin\@EXE@.exe"'
FunctionEnd
]=])
    string(REPLACE "@ICO@" "${_iec_ico}" _iec_nsis_code "${_iec_nsis_code}")
    string(REPLACE "@EXE@" "${target}" CPACK_NSIS_INSTALLER_MUI_ICON_CODE "${_iec_nsis_code}")
    # The Run value may point at an old folder, so it goes before the files.
    # The finish page writes it again; a silent install has no finish page, so
    # it writes the value here.
    set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS [=[
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "IconEmailCheck"
  ${If} ${Silent}
  ${AndIf} $IecStartAtSignIn == 1
    Call IecAutostart
  ${EndIf}
]=])
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS [=[
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "IconEmailCheck"
]=])
    # CPack's NSIS template has no hook in .onInit or on the finish page, so
    # CPack gets a copy that calls IecReadStartAtSignIn and IecFinishPre.
    file(READ "${CMAKE_ROOT}/Modules/Internal/CPack/NSIS.template.in" _iec_template)
    set(_iec_oninit "Function .onInit\n")
    set(_iec_finish "!insertmacro MUI_PAGE_FINISH\n")
    foreach(_anchor IN ITEMS _iec_oninit _iec_finish)
        string(FIND "${_iec_template}" "${${_anchor}}" _pos)
        if(_pos EQUAL -1)
            message(FATAL_ERROR "CPack's NSIS template has no \"${${_anchor}}\"; update cmake/Install.cmake.")
        endif()
    endforeach()
    string(REPLACE "${_iec_oninit}" "${_iec_oninit}  Call IecReadStartAtSignIn\n" _iec_template "${_iec_template}")
    string(REPLACE "${_iec_finish}" "!define MUI_PAGE_CUSTOMFUNCTION_PRE IecFinishPre\n  ${_iec_finish}"
        _iec_template "${_iec_template}")
    file(WRITE "${PROJECT_BINARY_DIR}/cpack/NSIS.template.in" "${_iec_template}")
    list(PREPEND CMAKE_MODULE_PATH "${PROJECT_BINARY_DIR}/cpack")
    # Pass the NSIS code above through unchanged; without this CPack strips its quotes.
    set(CPACK_VERBATIM_VARIABLES ON)
endmacro()
