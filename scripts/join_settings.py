#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""\
@file   join_settings.py
@date 2010-08-21
@brief  Generate settings.xml from (truncated) xml files.

$LicenseInfo:firstyear=2010&license=mit$

Copyright (c) 2010, Armin.Weatherwax@googlemail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
$/LicenseInfo$
"""

import os, sys

SCRIPT_DIR = os.path.abspath( os.path.dirname( sys.argv[0] ) )
ROOT_DIR   = os.path.normpath( os.path.join(SCRIPT_DIR, "..") )
APP_SETTINGS_DIR = os.path.normpath( os.path.join(ROOT_DIR, "indra","newview","app_settings") )

def s_join(settings):
    if len(settings) == 0:
      settings = ["settings"] #["settings","settings_per_account"]

    for setting in settings:
        settings_dir = os.path.join(APP_SETTINGS_DIR, "%s.d" % setting)
        print 'generating %s.xml from %s' % (setting, settings_dir)
        files = []
        try:
            files = os.listdir(settings_dir)
        except:
            print "dir not found: %s" % settings_dir

        files = sorted([ file for file in files if file.endswith('.xml') ])

        outfile =  '<?xml version="1.0" ?>\n<llsd>\n<map>'
        outfile = outfile + "\n\n  <!-- Note for client developers: This file is auto-generated -->"
        outfile = outfile + "\n  <!-- it will be overwritten each time the client is compiled -->"
        outfile = outfile + "\n  <!-- please put your changes in %s.d/ -->\n" % setting
        for file in files:
            f=open(os.path.join(settings_dir,file), 'r')
            data = str(f.read())
            f.close()
            outfile = outfile + "\n" + data

        outfile =  outfile + '\n</map>\n</llsd>\n'    
        f = open(os.path.join(APP_SETTINGS_DIR, '%s.xml' % setting ), "w")
        f.write(str(outfile))
        f.close()


if __name__ == "__main__":
    args = sys.argv[1:]
    s_join(args)
