cmake_minimum_required(VERSION 3.21)

set(IEC_VERSION "0.1.0" CACHE STRING "The app's version, as X.Y.Z")
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

find_program(IEC_CCACHE ccache)
if(IEC_CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${IEC_CCACHE}")
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

# The Google OAuth client that "Sign in with Google" uses. Each user can also
# set their own in the app's settings. See README.md#gmail.
set(IEC_GOOGLE_CLIENT_ID "$ENV{IEC_GOOGLE_CLIENT_ID}" CACHE STRING "Default Google OAuth client ID")
set(IEC_GOOGLE_CLIENT_SECRET "$ENV{IEC_GOOGLE_CLIENT_SECRET}" CACHE STRING "Default Google OAuth client secret")

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

function(iec_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
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
