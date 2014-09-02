#!/bin/bash

SCRIPTSRC=`readlink -f "$0" || echo "$0"`
RUN_PATH=`dirname "${SCRIPTSRC}" || echo .`

install_prefix=${RUN_PATH}/..

function install_desktop_entry()
{
    local installation_prefix="$1"
    local desktop_entries_dir="$2"

    local desktop_entry="\
[Desktop Entry]\n\
Name=Kokua\n\
Comment=Client for the On-line Virtual Worlds\n\
Exec=${installation_prefix}/kokua\n\
Icon=${installation_prefix}/kokua_icon.png\n\
Terminal=false\n\
Type=Application\n\
Categories=Application;Network;\n\
StartupNotify=true\n\
StartupWMClass=do-not-directly-run-kokua-bin\n\
X-Desktop-File-Install-Version=3.0"

    echo " - Installing menu entries in ${desktop_entries_dir}"
    mkdir -vp "${desktop_entries_dir}"
    echo -e $desktop_entry > "${desktop_entries_dir}/kokua-viewer.desktop" || "Failed to install application menu!"
}

if [ "$UID" == "0" ]; then
    # system-wide
    install_desktop_entry "$install_prefix" /usr/local/share/applications
else
    # user-specific
    install_desktop_entry "$install_prefix" "$HOME/.local/share/applications"
fi
