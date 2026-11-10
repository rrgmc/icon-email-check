cmake_minimum_required(VERSION 3.21)

# Lets CMAKE_MSVC_DEBUG_INFORMATION_FORMAT below take effect. It must come
# before project().
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()
# install() DESTINATION paths such as ./bin are normalized.
if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()

# The release workflow passes the tag's version, for example -DIEC_VERSION=1.2.0.
set(IEC_VERSION "1.0.0" CACHE STRING "The app's version, as X.Y.Z")
if(NOT IEC_VERSION MATCHES "^[0-9]+\.[0-9]+\.[0-9]+$")
    message(FATAL_ERROR "IEC_VERSION is \"${IEC_VERSION}\", not X.Y.Z")
endif()

project(icon_email_check
    VERSION ${IEC_VERSION}
    DESCRIPTION "A tray icon that counts unread email"
    HOMEPAGE_URL "https://github.com/rrgmc/icon-email-check"
    LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ccache, where it is installed, shares compiling between worktrees and CI
# runs. It cannot cache MSVC's separate debug files, so objects hold their
# debug information.
find_program(IEC_CCACHE ccache)
if(IEC_CCACHE)
    message(STATUS "Compiling through ${IEC_CCACHE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${IEC_CCACHE}")
    set(CMAKE_OBJCXX_COMPILER_LAUNCHER "${IEC_CCACHE}")
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Network Svg Test)
qt_standard_project_setup()

# Qt 6.8's FindWrapOpenGL links the AGL framework, which Xcode 26 no longer
# ships. Nothing here uses AGL, so drop it.
if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
    get_target_property(_gl_libs WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
    list(FILTER _gl_libs EXCLUDE REGEX "AGL")
    set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES INTERFACE_LINK_LIBRARIES "${_gl_libs}")
endif()

option(IEC_WERROR "Treat compiler warnings as errors" OFF)

# The Google OAuth client that "Sign in with Google" uses, built into the app.
# CI takes them from the repository's secrets of the same names, through the
# environment. Only src/Settings.cpp is compiled with them. A desktop app's
# client ships inside the app, so this keeps them out of the repository, not
# out of the app. See README.md#gmail.
set(IEC_GOOGLE_CLIENT_ID "$ENV{IEC_GOOGLE_CLIENT_ID}" CACHE STRING "The built-in Google OAuth client ID")
set(IEC_GOOGLE_CLIENT_SECRET "$ENV{IEC_GOOGLE_CLIENT_SECRET}" CACHE STRING "The built-in Google OAuth client secret")
if(IEC_GOOGLE_CLIENT_ID AND NOT IEC_GOOGLE_CLIENT_SECRET)
    message(WARNING "IEC_GOOGLE_CLIENT_ID is set without IEC_GOOGLE_CLIENT_SECRET; Google refuses a desktop app's sign-in without the secret.")
endif()
# Whether the settings window has the "Google sign-in" section, where a user
# sets an OAuth client of their own. Off by default when a client is built in.
if(IEC_GOOGLE_CLIENT_ID)
    set(_google_settings_default OFF)
else()
    set(_google_settings_default ON)
endif()
option(IEC_GOOGLE_CLIENT_SETTINGS "Show the Google sign-in section in the settings window" ${_google_settings_default})
if(IEC_GOOGLE_CLIENT_ID)
    message(STATUS "Google sign-in: built-in client; settings section ${IEC_GOOGLE_CLIENT_SETTINGS}")
else()
    message(STATUS "Google sign-in: no built-in client; settings section ${IEC_GOOGLE_CLIENT_SETTINGS}")
endif()

# macOS keeps a list of the apps that may read each keychain entry. An ad hoc
# signature changes with every build, so macOS would ask again after each one.
# A certificate gives each build the same identity.
set(IEC_MACOS_SIGN_IDENTITY "$ENV{IEC_MACOS_SIGN_IDENTITY}" CACHE STRING
    "macOS: the certificate that signs the app bundle. Empty uses IEC_MACOS_DEFAULT_SIGN_IDENTITY when the keychain has it")
set(IEC_MACOS_DEFAULT_SIGN_IDENTITY "Apple Development: Rangel Reale (B6EDST9Q92)")
set(IEC_MACOS_SIGN_IDENTITY_USED "${IEC_MACOS_SIGN_IDENTITY}")
if(APPLE AND NOT IEC_MACOS_SIGN_IDENTITY_USED)
    execute_process(COMMAND security find-identity -v -p codesigning
        OUTPUT_VARIABLE _identities ERROR_QUIET)
    string(FIND "${_identities}" "\"${IEC_MACOS_DEFAULT_SIGN_IDENTITY}\"" _found)
    if(_found GREATER -1)
        set(IEC_MACOS_SIGN_IDENTITY_USED "${IEC_MACOS_DEFAULT_SIGN_IDENTITY}")
    endif()
endif()
if(APPLE)
    if(IEC_MACOS_SIGN_IDENTITY_USED)
        message(STATUS "Signing the app bundle with ${IEC_MACOS_SIGN_IDENTITY_USED}")
    else()
        message(STATUS "Signing the app bundle ad hoc")
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Dependencies)
include(Deploy)
include(Install)

function(iec_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(IEC_WERROR)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(IEC_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
