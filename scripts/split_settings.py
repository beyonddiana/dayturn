#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""\
@file split_settings.py
@author Armin Weatherwax
@date 2011-12-10
@brief sloppy script to split settings.xml into single (truncated)xml files.



$LicenseInfo:firstyear=2011&license=mit$

Copyright (c) 2011, Armin.Weatherwax@googlemail.com

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

import sys
import os.path


def add_indra_lib_path():
    root = os.path.realpath(__file__)
    # always insert the directory of the script in the search path
    dir = os.path.dirname(root)
    if dir not in sys.path:
        sys.path.insert(0, dir)

    # Now go look for indra/lib/python in the parent dies
    while root != os.path.sep:
        root = os.path.dirname(root)
        dir = os.path.join(root, 'indra', 'lib', 'python')
        if os.path.isdir(dir):
            if dir not in sys.path:
                sys.path.insert(0, dir)
            return root
    else:
        print >>sys.stderr, "This script is not inside a valid installation."
        sys.exit(1)

base_dir = add_indra_lib_path()
from indra.base import llsd
def usage(me):
    print "%s options:" % me
    print " -i"
    print "--infile <infile.xml> : settings file to split. Using \"settings.xml\" if left out."
    print " -o"
    print "--outdir <settings.d> : Directory to write the files to."
    print "                        If left out but an \".xml\" is specified using \"-i\""
    print "                        the outdir is derived from the infile, "
    print "                        e.g. for \"foo.xml\": \"foo.d/\". "
    print "                        Otherwise the outdir is \"settings.d/\"."
    print " -h"
    print "--help                : This help."
    sys.exit(0)

def split(infile, outdir):
    if len(infile) == 0:
       settings_file_name = "settings.xml"
    else:
       settings_file_name = infile
    if len(outdir) == 0:
       if infile.endswith(".xml"):
          dirname = infile.rpartition(".xml")[0]
          dirname = dirname.rpartition("/")[2]
          dirname = dirname + ".d"
       else:
          dirname="settings.d"
    else:
       dirname=outdir

    settings = llsd.parse(file(settings_file_name, 'rb').read())

    try:
        os.mkdir(dirname)
    except OSError, e:
        if e.errno != 17:
           raise

    for (k, m) in settings.iteritems():
       # format to our needs
       m = llsd.format_pretty_xml(m)
       m = m.partition("<map>")[2]
       m = m.rpartition("</map>")[0]
       out = "  <key>" + k + "</key>\n" + "  <map>" + m + "</map>"

       f = os.path.join(dirname, "%s.xml" % k)
       file(f, 'wb').write(out)

if __name__ == "__main__":
    args = sys.argv[1:]
    me = (sys.argv[0])
    if not args:
        print "No arguments, assuming settings.xml as infile, and settings.d/ ad outdir."
        print "%s -h for more options" % me
    r_infile = 0
    r_outdir = 0
    infile = ""
    outdir = ""
    for arg in args:
        if ('-h' == arg) or ("--help" == arg):
          usage(me)
        if r_infile == 1:
           if ('-i' == arg) or ("--infile" == arg) or ('-o' == arg) or ("--outdir" == arg):
              print "Wrong syntax. Try --help for help."
           else:
              infile = arg
              r_infile = 0
        if r_outdir  == 1:
           if ('-i' == arg) or ("--infile" == arg) or ('-o' == arg) or ("--outdir" == arg):
              print "Wrong syntax. Try --help for help."
           else:
              outdir = arg
              r_outdir = 0
        if ('-i' == arg) or ("--infile" == arg):
          r_infile = 1
        if ('-o' == arg) or ("--outdir" == arg):
          r_outdir = 1

    split(infile, outdir)
