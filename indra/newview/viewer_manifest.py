#!/usr/bin/env python
"""\
@file viewer_manifest.py
@author Ryan Williams
@brief Description of all installer viewer files, and methods for packaging
       them into installers for all supported platforms.

$LicenseInfo: firstyear=2006&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2006-2014, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""
import sys
import os.path
import shutil
import errno
import json
import re
import tarfile
import time
import random
import subprocess

viewer_dir = os.path.dirname(__file__)
# Add indra/lib/python to our path so we don't have to muck with PYTHONPATH.
# Put it FIRST because some of our build hosts have an ancient install of
# indra.util.llmanifest under their system Python!
sys.path.insert(0, os.path.join(viewer_dir, os.pardir, "lib", "python"))
from indra.util.llmanifest import LLManifest, main, path_ancestors, CHANNEL_VENDOR_BASE, RELEASE_CHANNEL, ManifestError, MissingError
try:
    from llbase import llsd
except ImportError:
    from indra.base import llsd
    
    
global home_path
from os.path import expanduser
home_path = expanduser("~")

class ViewerManifest(LLManifest):
    def is_packaging_viewer(self):
        # Some commands, files will only be included
        # if we are packaging the viewer on windows.
        # This manifest is also used to copy
        # files during the build (see copy_w_viewer_manifest
        # and copy_l_viewer_manifest targets)
        return 'package' in self.args['actions']
    
    def construct(self):
        super(ViewerManifest, self).construct()
        self.path(src="../../scripts/messages/message_template.msg", dst="app_settings/message_template.msg")
        self.path(src="../../etc/message.xml", dst="app_settings/message.xml")

        if self.is_packaging_viewer():
            with self.prefix(src="app_settings"):
                self.exclude("logcontrol.xml")
                self.exclude("logcontrol-dev.xml")
                self.path("*.ini")
                self.path("*.xml")
                self.path("*.db2")

                # include the entire shaders directory recursively
                self.path("shaders")
                # include the extracted list of contributors
                contributions_path = "../../doc/contributions.txt"
                contributor_names = self.extract_names(contributions_path)
                self.put_in_file(contributor_names, "contributors.txt", src=contributions_path)

                # ... and the entire windlight directory
                self.path("windlight")

                # ... and the entire image filters directory
                self.path("filters")
            
                # ... and the included spell checking dictionaries and certificates
                pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
                with self.prefix(src=pkgdir,dst=""):
                    self.path("dictionaries")
                    self.path("ca-bundle.crt")

                # include the extracted packages information (see BuildPackagesInfo.cmake)
                self.path(src=os.path.join(self.args['build'],"packages-info.txt"), dst="packages-info.txt")

                # CHOP-955: If we have "sourceid" or "viewer_channel" in the
                # build process environment, generate it into
                # settings_install.xml.
                settings_template = dict(
                    sourceid=dict(Comment='Identify referring agency to Linden web servers',
                                  Persist=1,
                                  Type='String',
                                  Value=''),
                    CmdLineGridChoice=dict(Comment='Default grid',
                                  Persist=0,
                                  Type='String',
                                  Value=''),
                    CmdLineChannel=dict(Comment='Command line specified channel name',
                                        Persist=0,
                                        Type='String',
                                        Value=''))
                settings_install = {}
                if 'sourceid' in self.args and self.args['sourceid']:
                    settings_install['sourceid'] = settings_template['sourceid'].copy()
                    settings_install['sourceid']['Value'] = self.args['sourceid']
                    print "Set sourceid in settings_install.xml to '%s'" % self.args['sourceid']

                if 'channel_suffix' in self.args and self.args['channel_suffix']:
                    settings_install['CmdLineChannel'] = settings_template['CmdLineChannel'].copy()
                    settings_install['CmdLineChannel']['Value'] = self.channel_with_pkg_suffix()
                    print "Set CmdLineChannel in settings_install.xml to '%s'" % self.channel_with_pkg_suffix()

                if 'grid' in self.args and self.args['grid']:
                    settings_install['CmdLineGridChoice'] = settings_template['CmdLineGridChoice'].copy()
                    settings_install['CmdLineGridChoice']['Value'] = self.grid()
                    print "Set CmdLineGridChoice in settings_install.xml to '%s'" % self.grid()

                # put_in_file(src=) need not be an actual pathname; it
                # only needs to be non-empty
                self.put_in_file(llsd.format_pretty_xml(settings_install),
                                 "settings_install.xml",
                                 src="environment")



            with self.prefix(src="character"):
                self.path("*.llm")
                self.path("*.xml")
                self.path("*.tga")

            # Include our fonts
            with self.prefix(src="fonts"):
                self.path("*.ttf")
                self.path("*.txt")

            # skins
            with self.prefix(src="skins"):
                    # include the entire textures directory recursively
                    with self.prefix(src="*/textures"):
                            self.path("*/*.jpg")
                            self.path("*/*.png")
                            self.path("*.tga")
                            self.path("*.j2c")
                            self.path("*.png")
                            self.path("textures.xml")
                    self.path("*/xui/*/*.xml")
                    self.path("*/xui/*/widgets/*.xml")
                    self.path("*/*.xml")

                    # The claim is that we never use local html files any
                    # longer. But rather than commenting out this block, let's
                    # rename every html subdirectory as html.old. That way, if
                    # we're wrong, a user actually does have the relevant
                    # files; s/he just needs to rename every html.old
                    # directory back to html to recover them.
                    with self.prefix(src="*/html", dst="*/html.old"):
                            self.path("*.png")
                            self.path("*/*/*.html")
                            #self.path("*/*/*.gif")


            #summary.json.  Standard with exception handling is fine.  If we can't open a new file for writing, we have worse problems
            summary_dict = {"Type":"viewer","Version":'.'.join(self.args['version']),"Channel":self.channel_with_pkg_suffix()}
            with open(os.path.join(os.pardir,'summary.json'), 'w') as summary_handle:
                json.dump(summary_dict,summary_handle)

            #we likely no longer need the test, since we will throw an exception above, but belt and suspenders and we get the
            #return code for free.
            if not self.path2basename(os.pardir, "summary.json"):
                print "No summary.json file"

    def grid(self):
        return self.args['grid']

    def channel(self):
        return self.args['channel']

    def channel_with_pkg_suffix(self):
        fullchannel=self.channel()
        if 'channel_suffix' in self.args and self.args['channel_suffix']:
            fullchannel+=' '+self.args['channel_suffix']
        return fullchannel

    def channel_variant(self):
        global CHANNEL_VENDOR_BASE
        return self.channel().replace(CHANNEL_VENDOR_BASE, "").strip()

    def channel_type(self): # returns 'release', 'beta', 'project', or 'test'
        global CHANNEL_VENDOR_BASE
        channel_qualifier=self.channel().replace(CHANNEL_VENDOR_BASE, "").lower().strip()
        if channel_qualifier.startswith('release'):
            channel_type='release'
        elif channel_qualifier.startswith('beta'):
            channel_type='beta'
        elif channel_qualifier.startswith('project'):
            channel_type='project'
        else:
            channel_type='test'
        return channel_type

    def channel_variant_app_suffix(self):
        # get any part of the compiled channel name after the CHANNEL_VENDOR_BASE
        suffix=self.channel_variant()
        # by ancient convention, we don't use Release in the app name
        if self.channel_type() == 'release':
            suffix=suffix.replace('Release', '').strip()
        # for the base release viewer, suffix will now be null - for any other, append what remains
        if len(suffix) > 0:
            suffix = "_"+ ("_".join(suffix.split()))
        # the additional_packages mechanism adds more to the installer name (but not to the app name itself)
        if 'channel_suffix' in self.args and self.args['channel_suffix']:
            suffix+='_'+("_".join(self.args['channel_suffix'].split()))
        return suffix

    def installer_base_name(self):
        global CHANNEL_VENDOR_BASE
        # a standard map of strings for replacing in the templates
        substitution_strings = {
            'channel_vendor_base' : '_'.join(CHANNEL_VENDOR_BASE.split()),
            'channel_variant_underscores':self.channel_variant_app_suffix(),
            'version_underscores' : '_'.join(self.args['version']),
            'arch':self.args['arch']
            }
        return "%(channel_vendor_base)s%(channel_variant_underscores)s_%(version_underscores)s_%(arch)s" % substitution_strings

    def app_name(self):
        global CHANNEL_VENDOR_BASE
        channel_type=self.channel_type()
        if channel_type == 'release':
            app_suffix='Viewer'
        else:
            app_suffix=self.channel_variant()
        return CHANNEL_VENDOR_BASE + ' ' + app_suffix
    def app_name_oneword(self):
        return ''.join(self.app_name().split())
    
    def icon_path(self):
        return "icons/" + self.channel_type()

    def extract_names(self,src):
        try:
            contrib_file = open(src,'r')
        except IOError:
            print "Failed to open '%s'" % src
            raise
        lines = contrib_file.readlines()
        contrib_file.close()

        # All lines up to and including the first blank line are the file header; skip them
        lines.reverse() # so that pop will pull from first to last line
        while not re.match("\s*$", lines.pop()) :
            pass # do nothing

        # A line that starts with a non-whitespace character is a name; all others describe contributions, so collect the names
        names = []
        for line in lines :
            if re.match("\S", line) :
                names.append(line.rstrip())
        # It's not fair to always put the same people at the head of the list
        random.shuffle(names)
        return ', '.join(names)

class Windows_i686_Manifest(ViewerManifest):
    def final_exe(self):
        return self.app_name_oneword()+".exe"
#    def final_exe(self):
#        if self.default_channel():
#            if self.default_grid():
#                return "Kokua.exe"
#            else:
#                return "Kokua.exe"
#        else:
#            return ''.join(self.channel().split()) + '.exe'

    def test_msvcrt_and_copy_action(self, src, dst):
        # This is used to test a dll manifest.
        # It is used as a temporary override during the construct method
        from test_win32_manifest import test_assembly_binding
        if src and (os.path.exists(src) or os.path.islink(src)):
            # ensure that destination path exists
            self.cmakedirs(os.path.dirname(dst))
            self.created_paths.append(dst)
            if not os.path.isdir(src):
                if(self.args['configuration'].lower() == 'debug'):
                    test_assembly_binding(src, "Microsoft.VC80.DebugCRT", "8.0.50727.4053")
                else:
                    test_assembly_binding(src, "Microsoft.VC80.CRT", "8.0.50727.4053")
                self.ccopy(src,dst)
            else:
                raise Exception("Directories are not supported by test_CRT_and_copy_action()")
        else:
            print "Doesn't exist:", src

    def test_for_no_msvcrt_manifest_and_copy_action(self, src, dst):
        # This is used to test that no manifest for the msvcrt exists.
        # It is used as a temporary override during the construct method
        from test_win32_manifest import test_assembly_binding
        from test_win32_manifest import NoManifestException, NoMatchingAssemblyException
        if src and (os.path.exists(src) or os.path.islink(src)):
            # ensure that destination path exists
            self.cmakedirs(os.path.dirname(dst))
            self.created_paths.append(dst)
            if not os.path.isdir(src):
                try:
                    if(self.args['configuration'].lower() == 'debug'):
                        test_assembly_binding(src, "Microsoft.VC80.DebugCRT", "")
                    else:
                        test_assembly_binding(src, "Microsoft.VC80.CRT", "")
                    raise Exception("Unknown condition")
                except NoManifestException as err:
                    pass
                except NoMatchingAssemblyException as err:
                    pass
                    
                self.ccopy(src,dst)
            else:
                raise Exception("Directories are not supported by test_CRT_and_copy_action()")
        else:
            print "Doesn't exist:", src
        
    def construct(self):
        super(Windows_i686_Manifest, self).construct()

        pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
        relpkgdir = os.path.join(pkgdir, "lib", "release")
        debpkgdir = os.path.join(pkgdir, "lib", "debug")

        if self.is_packaging_viewer():
            # Find dayturn-bin.exe in the 'configuration' dir, then rename it to the result of final_exe.
            self.path(src='%s/dayturn-bin.exe' % self.args['configuration'], dst=self.final_exe())

        # Plugin host application
        self.path2basename(os.path.join(os.pardir,
                                        'llplugin', 'slplugin', self.args['configuration']),
                           "slplugin.exe")
        
        # Get shared libs from the shared libs staging directory
        with self.prefix(src=os.path.join(os.pardir, 'sharedlibs', self.args['configuration']),
                       dst=""):

            # Mesh 3rd party libs needed for auto LOD and collada reading
            try:
                self.path("glod.dll")
            except RuntimeError as err:
                print err.message
                print "Skipping GLOD library (assumming linked statically)"

            # Get fmodex dll, continue if missing
            try:
                if self.args['configuration'].lower() == 'debug':
                    self.path("fmodexL.dll")
                else:
                    self.path("fmodex.dll")
            except:
                print "Skipping fmodex audio library(assuming other audio engine)"

            # For textures
            self.path("openjpeg.dll")

            # These need to be installed as a SxS assembly, currently a 'private' assembly.
            # See http://msdn.microsoft.com/en-us/library/ms235291(VS.80).aspx
            if self.args['configuration'].lower() == 'debug':
                 self.path("msvcr120d.dll")
                 self.path("msvcp120d.dll")
            else:
                 self.path("msvcr120.dll")
                 self.path("msvcp120.dll")

            # Vivox runtimes
#            self.path("wrap_oal.dll") no longer in archive
            self.path("SLVoice.exe")
            self.path("vivoxsdk.dll")
            self.path("ortp.dll")

				# Security
            self.path("ssleay32.dll")
            self.path("libeay32.dll")				

            # Hunspell
            self.path("libhunspell.dll")
            
	    #self.path(src="licenses-win32.txt", dst="licenses.txt")
	    #self.path("featuretable.txt")
	    

        # Media plugins - CEF
        with self.prefix(src='../media_plugins/cef/%s' % self.args['configuration'], dst="llplugin"):        
            self.path("media_plugin_cef.dll")

        # Media plugins - LibVLC
        with self.prefix(src='../media_plugins/libvlc/%s' % self.args['configuration'], dst="llplugin"):
            self.path("media_plugin_libvlc.dll")

        # CEF runtime files - debug
        if self.args['configuration'].lower() == 'debug':
            with self.prefix(src=os.path.join(os.pardir, 'packages', 'bin', 'debug'), dst="llplugin"):
                self.path("d3dcompiler_43.dll")
                self.path("d3dcompiler_47.dll")
                self.path("libcef.dll")
                self.path("libEGL.dll")
                self.path("libGLESv2.dll")
                self.path("llceflib_host.exe")
                self.path("natives_blob.bin")
                self.path("snapshot_blob.bin")
                self.path("widevinecdmadapter.dll")
                self.path("wow_helper.exe")
        else:
        # CEF runtime files - not debug (release, relwithdebinfo etc.)
            with self.prefix(src=os.path.join(os.pardir, 'packages', 'bin', 'release'), dst="llplugin"):
                self.path("d3dcompiler_43.dll")
                self.path("d3dcompiler_47.dll")
                self.path("libcef.dll")
                self.path("libEGL.dll")
                self.path("libGLESv2.dll")
                self.path("llceflib_host.exe")
                self.path("natives_blob.bin")
                self.path("snapshot_blob.bin")
                self.path("widevinecdmadapter.dll")
                self.path("wow_helper.exe")

        # MSVC DLLs needed for CEF and have to be in same directory as plugin
        with self.prefix(src=os.path.join(os.pardir, 'sharedlibs', 'Release'), dst="llplugin"):
            self.path("msvcp120.dll")
            self.path("msvcr120.dll")

        # CEF files common to all configurations
        with self.prefix(src=os.path.join(os.pardir, 'packages', 'resources'), dst="llplugin"):
            self.path("cef.pak")
            self.path("cef_100_percent.pak")
            self.path("cef_200_percent.pak")
            self.path("cef_extensions.pak")
            self.path("devtools_resources.pak")
            self.path("icudtl.dat")

        with self.prefix(src=os.path.join(os.pardir, 'packages', 'resources', 'locales'), dst=os.path.join('llplugin', 'locales')):
            self.path("am.pak")
            self.path("ar.pak")
            self.path("bg.pak")
            self.path("bn.pak")
            self.path("ca.pak")
            self.path("cs.pak")
            self.path("da.pak")
            self.path("de.pak")
            self.path("el.pak")
            self.path("en-GB.pak")
            self.path("en-US.pak")
            self.path("es-419.pak")
            self.path("es.pak")
            self.path("et.pak")
            self.path("fa.pak")
            self.path("fi.pak")
            self.path("fil.pak")
            self.path("fr.pak")
            self.path("gu.pak")
            self.path("he.pak")
            self.path("hi.pak")
            self.path("hr.pak")
            self.path("hu.pak")
            self.path("id.pak")
            self.path("it.pak")
            self.path("ja.pak")
            self.path("kn.pak")
            self.path("ko.pak")
            self.path("lt.pak")
            self.path("lv.pak")
            self.path("ml.pak")
            self.path("mr.pak")
            self.path("ms.pak")
            self.path("nb.pak")
            self.path("nl.pak")
            self.path("pl.pak")
            self.path("pt-BR.pak")
            self.path("pt-PT.pak")
            self.path("ro.pak")
            self.path("ru.pak")
            self.path("sk.pak")
            self.path("sl.pak")
            self.path("sr.pak")
            self.path("sv.pak")
            self.path("sw.pak")
            self.path("ta.pak")
            self.path("te.pak")
            self.path("th.pak")
            self.path("tr.pak")
            self.path("uk.pak")
            self.path("vi.pak")
            self.path("zh-CN.pak")
            self.path("zh-TW.pak")

        with self.prefix(src=os.path.join(os.pardir, 'packages', 'bin', 'release'), dst="llplugin"):
            self.path("libvlc.dll")
            self.path("libvlccore.dll")
            self.path("plugins/")

        # pull in the crash logger and updater from other projects
        # tag:"crash-logger" here as a cue to the exporter
        self.path(src='../win_crash_logger/%s/windows-crash-logger.exe' % self.args['configuration'],
                  dst="win_crash_logger.exe")

        if not self.is_packaging_viewer():
            self.package_file = "copied_deps"    

    def nsi_file_commands(self, install=True):
        def wpath(path):
            if path.endswith('/') or path.endswith(os.path.sep):
                path = path[:-1]
            path = path.replace('/', '\\')
            return path

        result = ""
        dest_files = [pair[1] for pair in self.file_list if pair[0] and os.path.isfile(pair[1])]
        # sort deepest hierarchy first
        dest_files.sort(lambda a,b: cmp(a.count(os.path.sep),b.count(os.path.sep)) or cmp(a,b))
        dest_files.reverse()
        out_path = None
        for pkg_file in dest_files:
            rel_file = os.path.normpath(pkg_file.replace(self.get_dst_prefix()+os.path.sep,''))
            installed_dir = wpath(os.path.join('$INSTDIR', os.path.dirname(rel_file)))
            pkg_file = wpath(os.path.normpath(pkg_file))
            if installed_dir != out_path:
                if install:
                    out_path = installed_dir
                    result += 'SetOutPath ' + out_path + '\n'
            if install:
                result += 'File ' + pkg_file + '\n'
            else:
                result += 'Delete ' + wpath(os.path.join('$INSTDIR', rel_file)) + '\n'

        # at the end of a delete, just rmdir all the directories
        if not install:
            deleted_file_dirs = [os.path.dirname(pair[1].replace(self.get_dst_prefix()+os.path.sep,'')) for pair in self.file_list]
            # find all ancestors so that we don't skip any dirs that happened to have no non-dir children
            deleted_dirs = []
            for d in deleted_file_dirs:
                deleted_dirs.extend(path_ancestors(d))
            # sort deepest hierarchy first
            deleted_dirs.sort(lambda a,b: cmp(a.count(os.path.sep),b.count(os.path.sep)) or cmp(a,b))
            deleted_dirs.reverse()
            prev = None
            for d in deleted_dirs:
                if d != prev:   # skip duplicates
                    result += 'RMDir ' + wpath(os.path.join('$INSTDIR', os.path.normpath(d))) + '\n'
                prev = d

        return result

    def package_finish(self):
        # a standard map of strings for replacing in the templates
        substitution_strings = {
            'version' : '.'.join(self.args['version']),
            'version_short' : '.'.join(self.args['version'][:-1]),
            'version_dashes' : '-'.join(self.args['version']),
            'final_exe' : self.final_exe(),
            'flags':'',
            'app_name':self.app_name(),
            'app_name_oneword':self.app_name_oneword()
            }

        installer_file = self.installer_base_name() + '_Setup.exe'
        substitution_strings['installer_file'] = installer_file
        
        version_vars = """
        !define INSTEXE  "%(final_exe)s"
        !define VERSION "%(version_short)s"
        !define VERSION_LONG "%(version)s"
        !define VERSION_DASHES "%(version_dashes)s"
        """ % substitution_strings
        
        if self.channel_type() == 'release':
            substitution_strings['caption'] = CHANNEL_VENDOR_BASE
        else:
            substitution_strings['caption'] = self.app_name() + ' ${VERSION}'

        inst_vars_template = """
            OutFile "%(installer_file)s"
            !define INSTNAME   "%(app_name_oneword)s"
            !define SHORTCUT   "%(app_name)s"
            !define URLNAME   "dayturn"
            Caption "%(caption)s"
            """
            
        if(self.args['arch'].lower() == 'x86_64'):
            engage_registry="SetRegView 64"
            program_files="!define MULTIUSER_USE_PROGRAMFILES64"
        else:
            engage_registry="SetRegView 32"
            program_files=""

        tempfile = "dayturn_setup_tmp.nsi"
        # the following replaces strings in the nsi template
        # it also does python-style % substitution
        self.replace_in("installers/windows/installer_template.nsi", tempfile, {
                "%%VERSION%%":version_vars,
                "%%SOURCE%%":self.get_src_prefix(),
                "%%INST_VARS%%":inst_vars_template % substitution_strings,
                "%%INSTALL_FILES%%":self.nsi_file_commands(True),
                "%%PROGRAMFILES%%":program_files,
                "%%ENGAGEREGISTRY%%":engage_registry,
                "%%DELETE_FILES%%":self.nsi_file_commands(False)})

        # Check two paths, one for Program Files, and one for Program Files (x86).
        # Yay 64bit windows.
        for ProgramFiles in 'ProgramFiles', 'ProgramFiles(x86)':
            NSIS_path = os.path.expandvars(r'${%s}\NSIS\makensis.exe' % ProgramFiles)
            if os.path.exists(NSIS_path):
                break
        installer_created=False
        nsis_attempts=3
        nsis_retry_wait=15
        for attempt in xrange(nsis_attempts):
            try:
                self.run_command([NSIS_path, '/V2', self.dst_path_of(tempfile)])
            except ManifestError as err:
                if attempt+1 < nsis_attempts:
                    print >> sys.stderr, "nsis failed, waiting %d seconds before retrying" % nsis_retry_wait
                    time.sleep(nsis_retry_wait)
                    nsis_retry_wait*=2
            else:
                # NSIS worked! Done!
                break
        else:
            print >> sys.stderr, "Maximum nsis attempts exceeded; giving up"
            raise

        # self.remove(self.dst_path_of(tempfile))
        # If we're on a build machine, sign the code using our Authenticode certificate. JC
        sign_py = os.path.expandvars("${SIGN}")
        if not sign_py or sign_py == "${SIGN}":
            sign_py = 'C:\\buildscripts\\code-signing\\sign.py'
        else:
            sign_py = sign_py.replace('\\', '\\\\\\\\')
        python = os.path.expandvars("${PYTHON}")
        if not python or python == "${PYTHON}":
            python = 'python'
        if os.path.exists(sign_py):
            self.run_command("%s %s %s" % (python, sign_py, self.dst_path_of(installer_file).replace('\\', '\\\\\\\\')))
        else:
            print "Skipping code signing,", sign_py, "does not exist"
        self.created_path(self.dst_path_of(installer_file))
        self.package_file = installer_file




################################################################

def symlinkf(src, dst):
    """
    Like ln -sf, but uses os.symlink() instead of running ln.
    """
    try:
        os.symlink(src, dst)
    except OSError as err:
        if err.errno != errno.EEXIST:
            raise
        # We could just blithely attempt to remove and recreate the target
        # file, but that strategy doesn't work so well if we don't have
        # permissions to remove it. Check to see if it's already the
        # symlink we want, which is the usual reason for EEXIST.
        elif os.path.islink(dst):
            if os.readlink(dst) == src:
                # the requested link already exists
                pass
            else:
                # dst is the wrong symlink; attempt to remove and recreate it
                os.remove(dst)
                os.symlink(src, dst)
        elif os.path.isdir(dst):
            print "Requested symlink (%s) exists but is a directory; replacing" % dst
            shutil.rmtree(dst)
            os.symlink(src, dst)
        elif os.path.exists(dst):
            print "Requested symlink (%s) exists but is a file; replacing" % dst
            os.remove(dst)
            os.symlink(src, dst)
        else:
            # see if the problem is that the parent directory does not exist
            # and try to explain what is missing
            (parent, tail) = os.path.split(dst)
            while not os.path.exists(parent):
                (parent, tail) = os.path.split(parent)
            if tail:
                raise Exception("Requested symlink (%s) cannot be created because %s does not exist"
                                % os.path.join(parent, tail))
            else:
                raise

if __name__ == "__main__":

    try:
        main()
    except (ManifestError, MissingError) as err:
        sys.exit("\nviewer_manifest.py failed: "+err.msg)
    except:
        raise
