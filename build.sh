#!/bin/sh

# This is the custom build script for the viewer
#
# It must be run by the Linden Lab build farm shared buildscript because
# it relies on the environment that sets up, functions it provides, and
# the build result post-processing it does.
#
# The shared buildscript build.sh invokes this because it is named 'build.sh',
# which is the default custom build script name in buildscripts/hg/BuildParams
#
# PLEASE NOTE:
#
# * This script is interpreted on three platforms, including windows and cygwin
#   Cygwin can be tricky....
# * The special style in which python is invoked is intentional to permit
#   use of a native python install on windows - which requires paths in DOS form

build_dir_CYGWIN()
{
  echo build-vs
}

viewer_channel_suffix()
{
    local package_name="$1"
    local suffix_var="${package_name}_viewer_channel_suffix"
    local suffix=$(eval "echo \$${suffix_var}")
    if [ "$suffix"x = ""x ]
    then
        echo ""
    else
        echo "_$suffix"
    fi
}


pre_build()
{
  local variant="$1"
  begin_section "Configure $variant"
    [ -n "$master_message_template_checkout" ] \
    && [ -r "$master_message_template_checkout/message_template.msg" ] \
    && template_verifier_master_url="-DTEMPLATE_VERIFIER_MASTER_URL=file://$master_message_template_checkout/message_template.msg"

    "$autobuild" configure --quiet -c $variant -- \
     -DPACKAGE:BOOL=ON \
     -DRELEASE_CRASH_REPORTING:BOOL=OFF \
     -DVIEWER_CHANNEL:STRING="\"$viewer_channel\"" \
     -DGRID:STRING="\"$viewer_grid\"" \
     -DLL_TESTS:BOOL="$run_tests" \
     -DTEMPLATE_VERIFIER_OPTIONS:STRING="$template_verifier_options" $template_verifier_master_url
  end_section "Configure $variant"
}

package_llphysicsextensions_tpv()
{
  begin_section "PhysicsExtensions_TPV"
  tpv_status=0
  if [ "$variant" = "Release" ]
  then 
      llpetpvcfg=$build_dir/packages/llphysicsextensions/autobuild-tpv.xml
      "$autobuild" build --quiet --config-file $llpetpvcfg -c Tpv
      
      # capture the package file name for use in upload later...
      PKGTMP=`mktemp -t pgktpv.XXXXXX`
      trap "rm $PKGTMP* 2>/dev/null" 0
      "$autobuild" package --quiet --config-file $llpetpvcfg --results-file "$(native_path $PKGTMP)"
      tpv_status=$?
      if [ -r "${PKGTMP}" ]
      then
          cat "${PKGTMP}" >> "$build_log"
          eval $(cat "${PKGTMP}") # sets autobuild_package_{name,filename,md5}
          autobuild_package_filename="$(shell_path "${autobuild_package_filename}")"
          echo "${autobuild_package_filename}" > $build_dir/llphysicsextensions_package
      fi
  else
      record_event "Do not provide llphysicsextensions_tpv for $variant"
      llphysicsextensions_package=""
  fi
  end_section "PhysicsExtensions_TPV"
  return $tpv_status
}

build()
{
  local variant="$1"
  if $build_viewer
  then
    "$autobuild" build --no-configure -c $variant
    build_ok=$?

    # Run build extensions
    if [ $build_ok -eq 0 -a -d ${build_dir}/packages/build-extensions ]
    then
        for extension in ${build_dir}/packages/build-extensions/*.sh
        do
            begin_section "Extension $extension"
            . $extension
            end_section "Extension $extension"
        done
    fi

    # *TODO: Make this a build extension.
    package_llphysicsextensions_tpv
    tpvlib_build_ok=$?
    if [ $build_ok -eq 0 -a $tpvlib_build_ok -eq 0 ]
    then
      echo true >"$build_dir"/build_ok
    else
      echo false >"$build_dir"/build_ok
    fi
  fi
}

################################################################
# Start of the actual script
################################################################

# Check to see if we were invoked from the master buildscripts wrapper, if not, fail
if [ "x${BUILDSCRIPTS_SUPPORT_FUNCTIONS}" = x ]
then
    echo "This script relies on being run by the master Linden Lab buildscripts" 1>&2
    exit 1
fi

# Check to see if we're skipping the platform
if ! eval '$build_'"$arch"
then
    record_event "building on architecture $arch is disabled"
    pass
fi

# ensure AUTOBUILD is in native path form for child processes
AUTOBUILD="$(native_path "$AUTOBUILD")"
# set "$autobuild" to cygwin path form for use locally in this script
autobuild="$(shell_path "$AUTOBUILD")"
if [ ! -x "$autobuild" ]
then
  record_failure "AUTOBUILD not executable: '$autobuild'"
  exit 1
fi

# load autobuild provided shell functions and variables
eval "$("$autobuild" --quiet source_environment)"

# dump environment variables for debugging
begin_section "Environment"
env|sort
end_section "Environment"

# Now run the build
succeeded=true
build_processes=
last_built_variant=
for variant in $variants
do
  eval '$build_'"$variant" || continue
  eval '$build_'"$arch"_"$variant" || continue

  # Only the last built arch is available for upload
  last_built_variant="$variant"

  build_dir=`build_dir_$arch $variant`
  build_dir_stubs="$build_dir/win_setup/$variant"

  begin_section "Initialize $variant Build Directory"
  rm -rf "$build_dir"
  mkdir -p "$build_dir/tmp"
  end_section "Initialize $variant Build Directory"

  if pre_build "$variant" "$build_dir"
  then
      begin_section "Build $variant"
      build "$variant" "$build_dir"
      if `cat "$build_dir/build_ok"`
      then
          case "$variant" in
            Release)
              if [ -r "$build_dir/autobuild-package.xml" ]
              then
                  begin_section "Autobuild metadata"
                  upload_item docs "$build_dir/autobuild-package.xml" text/xml
                  if [ "$arch" != "Linux" ]
                  then
                      record_dependencies_graph # defined in buildscripts/hg/bin/build.sh
                  else
                      record_event "TBD - no dependency graph for linux (probable python version dependency)" 1>&2
                  fi
                  end_section "Autobuild metadata"
              else
                  record_event "no autobuild metadata at '$build_dir/autobuild-package.xml'"
              fi
              ;;
          esac

      else
          record_failure "Build of \"$variant\" failed."
      fi
      end_section "Build $variant"
  else
      record_event "configure for $variant failed: build skipped"
  fi

  if ! $succeeded 
  then
      record_event "remaining variants skipped due to $variant failure"
      break
  fi
done

# check status and upload results to S3
if $succeeded
then
  if $build_viewer
  then
    begin_section Upload Installer
    # Upload installer
    package=$(installer_$arch)
    if [ x"$package" = x ] || test -d "$package"
    then
      record_event "??? mystery event $package // $build_coverity"
      succeeded=$build_coverity
    else
      # Upload base package.
      upload_item installer "$package" binary/octet-stream
      upload_item quicklink "$package" binary/octet-stream
      [ -f $build_dir/summary.json ] && upload_item installer $build_dir/summary.json text/plain

      # Upload additional packages.
      for package_id in $additional_packages
      do
        package=$(installer_$arch "$package_id")
        if [ x"$package" != x ]
        then
          upload_item installer "$package" binary/octet-stream
          upload_item quicklink "$package" binary/octet-stream
        else
          record_failure "Failed to find additional package for '$package_id'."
        fi
      done

      case "$last_built_variant" in
      Release)
      # nat 2016-12-22: without RELEASE_CRASH_REPORTING, we have no symbol file.
          if [ "${RELEASE_CRASH_REPORTING:-}" != "OFF" ]
          then
              # Upload crash reporter file
              # These names must match the set of VIEWER_SYMBOL_FILE in indra/newview/CMakeLists.txt
              case "$arch" in
                  CYGWIN)
                      symbolfile="$build_dir/newview/Release/viewer-symbols-windows-${AUTOBUILD_ADDRSIZE}.tar.bz2"
                      ;;
                  Darwin)
                      symbolfile="$build_dir/newview/Release/viewer-symbols-darwin-${AUTOBUILD_ADDRSIZE}.tar.bz2"
                      ;;
                  Linux)
                      symbolfile="$build_dir/newview/Release/viewer-symbols-linux-${AUTOBUILD_ADDRSIZE}.tar.bz2"
                      ;;
              esac
              python_cmd "$helpers/codeticket.py" addoutput "Symbolfile" "$symbolfile" \
                  || fatal "Upload of symbolfile failed"
          fi

        # Upload the actual dependencies used
        if [ -r "$build_dir/packages/installed-packages.xml" ]
        then
            upload_item installer "$build_dir/packages/installed-packages.xml" text/xml
        fi

        # Upload the llphysicsextensions_tpv package, if one was produced
        # *TODO: Make this an upload-extension
        if [ -r "$build_dir/llphysicsextensions_package" ]
        then
            llphysicsextensions_package=$(cat $build_dir/llphysicsextensions_package)
            upload_item private_artifact "$llphysicsextensions_package" binary/octet-stream
        fi
        ;;
      *)
        ;;
      esac

      # Run upload extensions
      if [ -d ${build_dir}/packages/upload-extensions ]; then
          for extension in ${build_dir}/packages/upload-extensions/*.sh; do
              begin_section "Upload Extension $extension"
              . $extension
              end_section "Upload Extension $extension"
          done
      fi
    fi
    end_section Upload Installer
  else
    record_event "skipping upload of installer"
  fi

  
else
    record_event "skipping upload of installer due to failed build"
fi

# The branch independent build.sh script invoking this script will finish processing
$succeeded || exit 1
