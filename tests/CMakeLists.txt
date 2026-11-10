# Unit tests of the parts that need no network or desktop: the IMAP and MIME
# parsers, the Gmail API answers, the account settings and the tray badge.
function(iec_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE iec_core Qt6::Test)
    iec_set_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
    # Headless: no window system or menu bar is needed. On Windows, Qt sends
    # its messages to the debugger when stderr is not a console, as under
    # CTest, so a test that fails in Qt would print nothing.
    set(environment "QT_QPA_PLATFORM=offscreen;QT_FORCE_STDERR_LOGGING=1")
    # The offscreen platform on Windows looks for fonts in Qt's own lib/fonts,
    # which the downloaded Qt lacks; without fonts, text has no size.
    if(WIN32)
        file(TO_CMAKE_PATH "$ENV{SystemRoot}/Fonts" fonts)
        list(APPEND environment "QT_QPA_FONTDIR=${fonts}")
    endif()
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "${environment}")
endfunction()

iec_add_test(tst_imap)
iec_add_test(tst_mime)
iec_add_test(tst_gmail)
iec_add_test(tst_account)
iec_add_test(tst_traybadge)
iec_add_test(tst_imap_checker)
iec_add_test(tst_account_dialog)
