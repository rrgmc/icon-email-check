# Unit tests of the parts that need no network or desktop: the IMAP and MIME
# parsers, the Gmail API answers, the account settings and the tray badge.
function(iec_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE iec_core Qt6::Test)
    iec_set_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
    # Headless: no window system or menu bar is needed.
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

iec_add_test(tst_imap)
iec_add_test(tst_mime)
iec_add_test(tst_gmail)
iec_add_test(tst_account)
iec_add_test(tst_traybadge)
iec_add_test(tst_imap_checker)
