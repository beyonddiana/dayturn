#!/bin/bash

# Send a URL of the form secondlife://... to Second Life.
#

URL="$1"

if [ -z "$URL" ]; then
    echo Usage: $0 hop://...
    exit
fi

RUN_PATH=`dirname "$0" || echo .`
cd "${RUN_PATH}"

if [ `pidof do-not-directly-run-kokua-bin` ]; then
    exec dbus-send --type=method_call --dest=com.kokuaviewer.ViewerAppAPIService /com/kokuaviewer/ViewerAppAPI com.kokuaviewer.ViewerAppAPI.GoSLURL string:"$1"
else
	exec ./kokua -url \'"${URL}"\'
fi
