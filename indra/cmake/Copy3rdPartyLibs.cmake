# -*- cmake -*-

# The copy_win_libs folder contains file lists and a script used to
# copy dlls, exes and such needed to run the Kokua.sln from within
# VisualStudio.

include(CMakeCopyIfDifferent)
include(Linking)

###################################################################
# set up platform specific lists of files that need to be copied
###################################################################
if(WINDOWS)
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}/Debug")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}/Release")

    #*******************************
    # VIVOX - *NOTE: no debug version
    set(vivox_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(vivox_files
        SLVoice.exe
        ca-bundle.crt
#       added from archive
        vivoxsdk.dll
        ortp.dll
        libsndfile-1.dll
        vivoxoal.dll
        vivoxplatform.dll
        zlib1.dll
        )

    #*******************************
    # Misc shared libs 

    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
    set(debug_files
 #       alut.dll
 #       openal32.dll
        openjpegd.dll
        libapr-1.dll
        libaprutil-1.dll
        libapriconv-1.dll

        # gstreamer dlls - not plugins
        avcodec-gpl-52.dll
        avdevice-gpl-52.dll
        avfilter-gpl-1.dll
        avformat-gpl-52.dll
        avutil-gpl-50.dll
        iconv.dll
        liba52-0.dll
        libbz2.dll
        libcelt-0.dll
        libdca-0.dll
        libexpat-1.dll
        libfaad-2.dll
        libFLAC-8.dll
        libgcrypt-11.dll
        libgio-2.0-0.dll
        libglib-2.0-0.dll
        libgmodule-2.0-0.dll
        libgnutls-26.dll
        libgobject-2.0-0.dll
        libgpg-error-0.dll
        libgstapp-0.10.dll
        libgstaudio-0.10.dll
        libgstbase-0.10.dll
        libgstcontroller-0.10.dll
        libgstdataprotocol-0.10.dll
        libgstfft-0.10.dll
        libgstinterfaces-0.10.dll
        libgstnet-0.10.dll
        libgstnetbuffer-0.10.dll
        libgstpbutils-0.10.dll
        libgstphotography-0.10.dll
        libgstreamer-0.10.dll
        libgstriff-0.10.dll
        libgstrtp-0.10.dll
        libgstrtsp-0.10.dll
        libgstsdp-0.10.dll
        libgstsignalprocessor-0.10.dll
        libgsttag-0.10.dll
        libgstvideo-0.10.dll
        libgthread-2.0-0.dll
        libmms-0.dll
        libmpeg2-0.dll
        libneon-27.dll
        libogg-0.dll
        liboil-0.3-0.dll
        libsoup-2.4-1.dll
        libtasn1-3.dll
        libtheora-0.dll
        libtheoradec-1.dll
        libvorbis-0.dll
        libvorbisenc-2.dll
        libvorbisfile-3.dll
        libwavpack-1.dll
        libx264-67.dll
        SDL.dll
        xvidcore.dll
        z.dll

        ssleay32.dll
        libeay32.dll
        glod.dll
        libhunspell.dll
        )


    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(release_files
 #       alut.dll
 #       openal32.dll
        openjpeg.dll
        libapr-1.dll
        libaprutil-1.dll
        libapriconv-1.dll

        # gstreamer dlls - not plugins
        avcodec-gpl-52.dll
        avdevice-gpl-52.dll
        avfilter-gpl-1.dll
        avformat-gpl-52.dll
        avutil-gpl-50.dll
        iconv.dll
        liba52-0.dll
        libbz2.dll
        libcelt-0.dll
        libdca-0.dll
        libexpat-1.dll
        libfaad-2.dll
        libFLAC-8.dll
        libgcrypt-11.dll
        libgio-2.0-0.dll
        libglib-2.0-0.dll
        libgmodule-2.0-0.dll
        libgnutls-26.dll
        libgobject-2.0-0.dll
        libgpg-error-0.dll
        libgstapp-0.10.dll
        libgstaudio-0.10.dll
        libgstbase-0.10.dll
        libgstcontroller-0.10.dll
        libgstdataprotocol-0.10.dll
        libgstfft-0.10.dll
        libgstinterfaces-0.10.dll
        libgstnet-0.10.dll
        libgstnetbuffer-0.10.dll
        libgstpbutils-0.10.dll
        libgstphotography-0.10.dll
        libgstreamer-0.10.dll
        libgstriff-0.10.dll
        libgstrtp-0.10.dll
        libgstrtsp-0.10.dll
        libgstsdp-0.10.dll
        libgstsignalprocessor-0.10.dll
        libgsttag-0.10.dll
        libgstvideo-0.10.dll
        libgthread-2.0-0.dll
        libmms-0.dll
        libmpeg2-0.dll
        libneon-27.dll
        libogg-0.dll
        liboil-0.3-0.dll
        libsoup-2.4-1.dll
        libtasn1-3.dll
        libtheora-0.dll
        libtheoradec-1.dll
        libvorbis-0.dll
        libvorbisenc-2.dll
        libvorbisfile-3.dll
        libwavpack-1.dll
        libx264-67.dll
        SDL.dll
        xvidcore.dll
        z.dll

        ssleay32.dll
        libeay32.dll
        glod.dll
        libhunspell.dll
        )

    if(USE_TCMALLOC)

      set(debug_files ${debug_files} libtcmalloc_minimal-debug.dll)
      set(release_files ${release_files} libtcmalloc_minimal.dll)
    endif(USE_TCMALLOC)

    if (FMODEX)
      set(debug_files ${debug_files} fmodexL.dll)
      set(release_files ${release_files} fmodex.dll)
    endif (FMODEX)

#*******************************
# Copy MS C runtime dlls, required for packaging.
# *TODO - Adapt this to support VC9
if (MSVC80)
    FIND_PATH(debug_msvc8_redist_path msvcr80d.dll
        PATHS
        ${MSVC_DEBUG_REDIST_PATH}
         [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\8.0\\Setup\\VC;ProductDir]/redist/Debug_NonRedist/x86/Microsoft.VC80.DebugCRT
        NO_DEFAULT_PATH
        NO_DEFAULT_PATH
        )

    if(EXISTS ${debug_msvc8_redist_path})
        set(debug_msvc8_files
            msvcr80d.dll
            msvcp80d.dll
            Microsoft.VC80.DebugCRT.manifest
            )

        copy_if_different(
            ${debug_msvc8_redist_path}
            "${SHARED_LIB_STAGING_DIR_DEBUG}"
            out_targets
            ${debug_msvc8_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})

    endif (EXISTS ${debug_msvc8_redist_path})

    FIND_PATH(release_msvc8_redist_path msvcr80.dll
        PATHS
        ${MSVC_REDIST_PATH}
         [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\8.0\\Setup\\VC;ProductDir]/redist/x86/Microsoft.VC80.CRT
        NO_DEFAULT_PATH
        NO_DEFAULT_PATH
        )

    if(EXISTS ${release_msvc8_redist_path})
        set(release_msvc8_files
            msvcr80.dll
            msvcp80.dll
            Microsoft.VC80.CRT.manifest
            )

        copy_if_different(
            ${release_msvc8_redist_path}
            "${SHARED_LIB_STAGING_DIR_RELEASE}"
            out_targets
            ${release_msvc8_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})

        copy_if_different(
            ${release_msvc8_redist_path}
            "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
            out_targets
            ${release_msvc8_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})
          
    endif (EXISTS ${release_msvc8_redist_path})
elseif (MSVC_VERSION EQUAL 1600) # VisualStudio 2010
    FIND_PATH(debug_msvc10_redist_path msvcr100d.dll
        PATHS
        ${MSVC_DEBUG_REDIST_PATH}
         [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0\\Setup\\VC;ProductDir]/redist/Debug_NonRedist/x86/Microsoft.VC100.DebugCRT
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/SysWOW64
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/System32
        NO_DEFAULT_PATH
        )

    if(EXISTS ${debug_msvc10_redist_path})
        set(debug_msvc10_files
            msvcr100d.dll
            msvcp100d.dll
            )

        copy_if_different(
            ${debug_msvc10_redist_path}
            "${SHARED_LIB_STAGING_DIR_DEBUG}"
            out_targets
            ${debug_msvc10_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})

    endif ()

    FIND_PATH(release_msvc10_redist_path msvcr100.dll
        PATHS
        ${MSVC_REDIST_PATH}
         [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0\\Setup\\VC;ProductDir]/redist/x86/Microsoft.VC100.CRT
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/SysWOW64
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/System32
        NO_DEFAULT_PATH
        )

    if(EXISTS ${release_msvc10_redist_path})
        set(release_msvc10_files
            msvcr100.dll
            msvcp100.dll
            )

        copy_if_different(
            ${release_msvc10_redist_path}
            "${SHARED_LIB_STAGING_DIR_RELEASE}"
            out_targets
            ${release_msvc10_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})

        copy_if_different(
            ${release_msvc10_redist_path}
            "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
            out_targets
            ${release_msvc10_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})
          
    endif ()
endif (MSVC80)

elseif(DARWIN)
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}/Debug/Resources")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/Resources")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}/Release/Resources")

    set(vivox_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(vivox_files
        SLVoice
        ca-bundle.crt
        libsndfile.dylib
        libvivoxoal.dylib
        libortp.dylib
        libvivoxplatform.dylib
        libvivoxsdk.dylib
       )
    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
    set(debug_files
       )
    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(release_files
#        libalut.0.dylib
#        libopenal.1.dylib
        libapr-1.0.dylib
        libapr-1.dylib
        libaprutil-1.0.dylib
        libaprutil-1.dylib
        libexception_handler.dylib
        libexpat.1.5.2.dylib
        libexpat.dylib
        libGLOD.dylib
#        libopenal.1.dylib
        libhunspell-1.3.0.dylib
        libndofdev.dylib
       )

    if (FMODEX)
      set(debug_files ${debug_files} libfmodexL.dylib)
      set(release_files ${release_files} libfmodex.dylib)
    endif (FMODEX)

elseif(LINUX)
    # linux is weird, multiple side by side configurations aren't supported
    # and we don't seem to have any debug shared libs built yet anyways...
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}")


        # ca-bundle.crt   #No cert for linux.  It is actually still 3.2SDK.
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS

    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
    set(debug_files
       )
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS

    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")

    # *FIX - figure out what to do with duplicate libalut.so here -brad

    if(${ARCH} STREQUAL "x86_64")
      set(vivox_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
      set(vivox_files
          libortp.so
          libvivoxsdk.so
          SLVoice
        )
      set(release_files
          libapr-1.so.0
          libaprutil-1.so.0
          libatk-1.0.so.0 
#          libbreakpad_client.so.0
#          libcares.so.2
#          libcrypto.so
#          libcrypto.so.1.0.0
#          libcollada14dom.so
          libdb-5.1.so
          libexpat.so
          libexpat.so.1
#          libgmock_main.so
#          libgmock.so.0
          libgmodule-2.0.so.0 
          libgobject-2.0.so 
#          libgtest_main.so
#          libgtest.so.0
#          libminizip.so
          libopenal.so
          libopenjpeg.so
          libopenjpeg.so.1.4.0
#           libstacktrace.so
#           libtcmalloc.so
#          libssl.so
#          libssl.so.1.0.0
# Remove OPenMP from build of viewer causes conflict starting at Viewer-Beta 3.3.3
#          libgomp.so.1
#          libgomp.so.1.0.0
#          libpcre.so.3
#          libpng15.so.15
#          libpng15.so.15.10.0
         )
    else(${ARCH} STREQUAL "x86_64")
      set(vivox_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
      set(vivox_files
          libsndfile.so.1
          libortp.so
          libvivoxoal.so.1
          libvivoxplatform.so
          libvivoxsdk.so
          SLVoice
          )
    set(release_files
        libapr-1.so.0
        libaprutil-1.so.0
        libatk-1.0.so
        libdb-5.1.so
        libexpat.so
        libexpat.so.1
        libfreetype.so.6.6.2
        libGLOD.so
        libgmodule-2.0.so
        libgobject-2.0.so
        libhunspell-1.3.so.0.0.0
        libopenal.so
        libopenjpeg.so
        libuuid.so.16
        libuuid.so.16.0.22
        libfontconfig.so.1.8.0
        libfontconfig.so.1
# Remove OPenMP from build of viewer causes conflict starting at Viewer-Beta 3.3.3
#        libgomp.so.1
#        libgomp.so.1.0.0
       )
    endif(${ARCH} STREQUAL "x86_64")

    if (USE_TCMALLOC)
      set(release_files ${release_files} "libtcmalloc_minimal.so")
    endif (USE_TCMALLOC)

    if (FMODEX)
      set(debug_files ${debug_files} "libfmodexL.so")
      set(release_files ${release_files} "libfmodex.so")
    endif (FMODEX)

else(WINDOWS)
    message(STATUS "WARNING: unrecognized platform for staging 3rd party libs, skipping...")
    set(vivox_src_dir "${CMAKE_SOURCE_DIR}/newview/vivox-runtime/i686-linux")#voice is always i686
    set(vivox_files "")
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(debug_src_dir "${CMAKE_SOURCE_DIR}/../libraries/${ARCH}-linux/lib/debug")
    set(debug_files "")
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(release_src_dir "${CMAKE_SOURCE_DIR}/../libraries/${ARCH}-linux/lib/release")
    set(release_files "")

    set(debug_llkdu_src "")
    set(debug_llkdu_dst "")
    set(release_llkdu_src "")
    set(release_llkdu_dst "")
    set(relwithdebinfo_llkdu_dst "")
endif(WINDOWS)


################################################################
# Done building the file lists, now set up the copy commands.
################################################################
copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR_DEBUG}"
    out_targets 
    ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELEASE}"
    out_targets
    ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
    out_targets
    ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})



copy_if_different(
    ${debug_src_dir}
    "${SHARED_LIB_STAGING_DIR_DEBUG}"
    out_targets
    ${debug_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${release_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELEASE}"
    out_targets
    ${release_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${release_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
    out_targets
    ${release_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

if(NOT USESYSTEMLIBS)
  add_custom_target(
      stage_third_party_libs ALL
      DEPENDS ${third_party_targets}
      )
endif(NOT USESYSTEMLIBS)
