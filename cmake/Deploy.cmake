# The bin folder: a copy of the app that runs on its own.

# iec_add_bin_folder(<target>)
#
# After each link of <target>, copies it to <build>/bin with the Qt libraries
# and plugins it needs, so the copy starts without Qt's bin folder on PATH.
#
# - Windows: windeployqt. The Visual C++ runtime is not copied.
# - macOS: the app bundle, made self-contained with macdeployqt. With a
#   certificate in IEC_MACOS_SIGN_IDENTITY_USED, both the built bundle and the
#   copy are signed with it, so the keychain knows each build as the same app.
# - Linux: only the executable, which finds Qt through its RPATH.
function(iec_add_bin_folder target)
    set(bin_dir "${CMAKE_BINARY_DIR}/bin")

    if(WIN32)
        if(NOT TARGET Qt6::windeployqt)
            message(WARNING "windeployqt was not found, so ${bin_dir} is not made.")
            return()
        endif()
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${bin_dir}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${target}>" "${bin_dir}"
            COMMAND Qt6::windeployqt --no-compiler-runtime --verbose 0
                "${bin_dir}/$<TARGET_FILE_NAME:${target}>"
            COMMENT "Deploying ${target} to ${bin_dir}"
            VERBATIM)
    elseif(APPLE)
        if(NOT TARGET Qt6::macdeployqt)
            message(WARNING "macdeployqt was not found, so ${bin_dir} is not made.")
            return()
        endif()
        set(bundle_dir "$<TARGET_FILE_DIR:${target}>/../..")
        set(bundle_name "$<TARGET_FILE_NAME:${target}>.app")
        set(sign_args)
        set(sign_built)
        if(IEC_MACOS_SIGN_IDENTITY_USED)
            set(sign_args "-codesign=${IEC_MACOS_SIGN_IDENTITY_USED}")
            set(sign_built COMMAND codesign --force --sign "${IEC_MACOS_SIGN_IDENTITY_USED}" "${bundle_dir}")
        endif()
        add_custom_command(TARGET ${target} POST_BUILD
            ${sign_built}
            COMMAND "${CMAKE_COMMAND}" -E rm -rf "${bin_dir}/${bundle_name}"
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "${bundle_dir}" "${bin_dir}/${bundle_name}"
            COMMAND Qt6::macdeployqt "${bin_dir}/${bundle_name}" -verbose=0 ${sign_args}
            COMMENT "Deploying ${target} to ${bin_dir}"
            VERBATIM)
    else()
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${bin_dir}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${target}>" "${bin_dir}"
            COMMENT "Copying ${target} to ${bin_dir}"
            VERBATIM)
    endif()
endfunction()
