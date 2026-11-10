# Third-party libraries fetched at configure time, each pinned to a release.

include(FetchContent)

# Third-party code is built static, and its own tests are not built.
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)

# qtkeychain: passwords and tokens in the operating system's keychain.
set(BUILD_WITH_QT6 ON CACHE BOOL "" FORCE)
set(BUILD_TRANSLATIONS OFF CACHE BOOL "" FORCE)
set(BUILD_TEST_APPLICATION OFF CACHE BOOL "" FORCE)
FetchContent_Declare(qtkeychain
    URL https://github.com/frankosterfeld/qtkeychain/archive/refs/tags/0.15.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON)
FetchContent_MakeAvailable(qtkeychain)
