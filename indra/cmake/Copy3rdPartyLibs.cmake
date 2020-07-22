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
#       added from archive
        vivoxsdk.dll
        ortp.dll
        )

    #*******************************
    # Misc shared libs 

#    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
#    set(debug_files
#       alut.dll
#       openal32.dll
#       openjpegd.dll
#       libapr-1.dll
#       libaprutil-1.dll
#       libapriconv-1.dll
#       ssleay32.dll
#       libeay32.dll
#       glod.dll
#       libhunspell.dll
#       gstreamer dlls - not plugins
#       )


    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(release_files
 #       alut.dll
 #       openal32.dll
        openjpeg.dll
        libapr-1.dll
        libaprutil-1.dll
        libapriconv-1.dll
        ssleay32.dll
        libeay32.dll
        glod.dll
        libhunspell.dll

        # gstreamer dlls - not plugins
		# place holder


        )

    if (FMODEX)
      set(debug_files ${debug_files} fmodexL.dll)
      set(release_files ${release_files} fmodex.dll)
    endif (FMODEX)
    
    if (FMODSTUDIO)
        set(debug_files ${debug_files} fmodL.dll)
        set(release_files ${release_files} fmod.dll)
    endif (FMODSTUDIO)

    #*******************************
    # Copy MS C runtime dlls, required for packaging.
    # *TODO - Adapt this to support VC9
    if (MSVC80)
        list(APPEND LMSVC_VER 80)
        list(APPEND LMSVC_VERDOT 8.0)
    elseif (MSVC_VERSION EQUAL 1600) # VisualStudio 2010
        MESSAGE(STATUS "MSVC_VERSION ${MSVC_VERSION}")
    elseif (MSVC_VERSION EQUAL 1800) # VisualStudio 2013, which is (sigh) VS 12
        list(APPEND LMSVC_VER 120)
        list(APPEND LMSVC_VERDOT 12.0)
    else (MSVC80)
        MESSAGE(WARNING "New MSVC_VERSION ${MSVC_VERSION} of MSVC: adapt Copy3rdPartyLibs.cmake")
    endif (MSVC80)

    # try to copy VS2010 redist independently of system version
    # maint-7360 CP
    # list(APPEND LMSVC_VER 100)
    # list(APPEND LMSVC_VERDOT 10.0)
    
    list(LENGTH LMSVC_VER count)
    math(EXPR count "${count}-1")
    foreach(i RANGE ${count})
        list(GET LMSVC_VER ${i} MSVC_VER)
        list(GET LMSVC_VERDOT ${i} MSVC_VERDOT)
        MESSAGE(STATUS "Copying redist libs for VC ${MSVC_VERDOT}")
        FIND_PATH(debug_msvc_redist_path NAME msvcr${MSVC_VER}d.dll
            PATHS            
            [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\${MSVC_VERDOT}\\Setup\\VC;ProductDir]/redist/Debug_NonRedist/x86/Microsoft.VC${MSVC_VER}.DebugCRT
            [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/SysWOW64
            [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/System32
            ${MSVC_DEBUG_REDIST_PATH}
            NO_DEFAULT_PATH
            )

        if(EXISTS ${debug_msvc_redist_path})
            set(debug_msvc_files
                msvcr${MSVC_VER}d.dll
                msvcp${MSVC_VER}d.dll
                )

            copy_if_different(
                ${debug_msvc_redist_path}
                "${SHARED_LIB_STAGING_DIR_DEBUG}"
                out_targets
                ${debug_msvc_files}
                )
            set(third_party_targets ${third_party_targets} ${out_targets})

            unset(debug_msvc_redist_path CACHE)
        endif()

        if(ADDRESS_SIZE EQUAL 32)
            # this folder contains the 32bit DLLs.. (yes really!)
            set(registry_find_path "[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/SysWOW64")
        else(ADDRESS_SIZE EQUAL 32)
            # this folder contains the 64bit DLLs.. (yes really!)
            set(registry_find_path "[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/System32")
        endif(ADDRESS_SIZE EQUAL 32)

        FIND_PATH(release_msvc_redist_path NAME msvcr${MSVC_VER}.dll
            PATHS            
            ${registry_find_path}
            NO_DEFAULT_PATH
            )

        if(EXISTS ${release_msvc_redist_path})
            set(release_msvc_files
                msvcr${MSVC_VER}.dll
                msvcp${MSVC_VER}.dll
                )

            copy_if_different(
                ${release_msvc_redist_path}
                "${SHARED_LIB_STAGING_DIR_RELEASE}"
                out_targets
                ${release_msvc_files}
                )
            set(third_party_targets ${third_party_targets} ${out_targets})

            copy_if_different(
                ${release_msvc_redist_path}
                "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
                out_targets
                ${release_msvc_files}
                )
            set(third_party_targets ${third_party_targets} ${out_targets})

            unset(release_msvc_redist_path CACHE)
        endif()
    endforeach()


elseif(LINUX)
    # linux is weird, multiple side by side configurations aren't supported
    # and we don't seem to have any debug shared libs built yet anyways...
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}")


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
#          libopenal.so
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
#        libopenal.so
        libopenjpeg.so
        libuuid.so.16
        libuuid.so.16.0.22
        libfontconfig.so.1.8.0
        libfontconfig.so.1
# Remove OPenMP from build of viewer causes conflict starting at Viewer-Beta 3.3.3
#        libgomp.so.1
#        libgomp.so.1.0.0
       )
    if (OPENAL)
           set(release_files ${release_files} "libopenal.so")
    endif (OPENAL)
    endif(${ARCH} STREQUAL "x86_64")

    if (FMODSTUDIO)
      set(debug_files ${debug_files} "libfmodL.so")
      set(release_files ${release_files} "libfmod.so")
    endif (FMODSTUDIO)    

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



#copy_if_different(
#    ${debug_src_dir}
#    "${SHARED_LIB_STAGING_DIR_DEBUG}"
#    out_targets
#    ${debug_files}
#    )
#set(third_party_targets ${third_party_targets} ${out_targets})

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
