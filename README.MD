## ** Kokua OpenSim ** ##
This repository is a baseline of the combined Linden Lab 4.0.2 SecondLife Viewer and Kokua 4.0.2 releases with select commits from the following Linden code base with the aim of providing a starting point for development of an OpenSim specific version of Kokua or derived viewers.

The reason for this is that the http changes in the Linden 4.0.3 release are interoperable with OpenSim core in Standalone mode or in Grid mode without assets served over http. Assets over http is still largely experimental in OpenSim and currently requires both more hardware and configuration skills than many small grids are willing or able to invest in. 

A pure-play OpenSim viewer will also over time evolve to a simpler code base removing functionality not supported by OpenSim, removing similar functionality with different implementations across the two environments, and adding better support for OpenSim specific functionality such as Hypergrid Teleports, variable size regions, NPCs, the OSSL scripting language and different physics engines. 

It will also remove the code for SecondLife specific functionality such as Pathfinding, a unified marketplace, experience keys, avatar complexity and information that is not directly relevant to OpenSim grids. 



### Contributing Code to Kokua OpenSim ###
----------------------------------

Contributing one patch with less than 3 files touched can be submitted as a diff or patch file. Please open a ticket and attach the file to a ticket.

Tickets are found on SourceForge at https://sourceforge.net/p/team-purple/kokua/tickets/

Contributions by frequent contributors or Kokua Team members requires a formalized work flow as outlined below.

Kokua repositories use mercurial bookmarks instead of branches. Reference mercurial wiki https://www.mercurial-scm.org/wiki/Bookmarks

Work Flow:

On bitbucket fork kokua-dev to your Bitbucket account. The repository name will default to kokua-dev, but can be named as desired.

Open the repository and notice bookmark icons that are named Kokua_MKRLV and KokuaNT; these represent our current working default branches

that are assigned to bookmarks. If the bookmarks where not there two (2) default branches would be present. The bookmarks tie the bookmark names

to each default branch mercurial hash. 

Using mercurial command hg clone place you forked kokua-dev repository on your local file system. Example: 

hg clone https://bitucket.org/nickyp/kokua-dev

An important step is to pull and update Bookmarks from your respository. This allows the bookmarks to advance to the tip with each commit.

A bookmark pull is hg pull -B KokuaNT https://bitucket.org/nickyp/kokua-dev --Notice that it is a capital B. Now repeat with Kokua-MKRLV bookmark.

If your bitbucket repository's bookmarks have fallen behind or do not show up then push the bookmark name. For example:

hg push -B KokuaNT https://bitucket.org/nickyp/kokua-dev notice the capital B again. Capital B is for bookmark exchange only. No changes are made to

the working tree.

Next use hg update to activate the bookmarks. "hg update KokuaNT" and then "hg update Kokua-MKRLV". Your working tree is now at Kokua-MKRLV.

If you want to work on KokuaNT just "hg update KokuaNT". In a terminal enter "hg bookmarks" and terminal response will be a list of all bookmarks

with an * next to the active bookmark.
 
Use of the tortoisehg program will allow a graphical view of bookmarks and provides bookmark management. In tortioisehg right click on a commit and

and select Bookmarks... a popup will show bookmark management options. 

Any collorative repository system will be frought with frusations and what seems as lost time resolving merge conflicts if the users

of the system do not check for incoming upstream changes before beginning work. Any upstream incoming changes should be pulled and

merged before beginning work. Use of mercurial queues MQ can be helpful by working in a patch queue and by uninstalling patches and pulling

and merging upstream and then reinstall from the patch queue. This insures that your changes are allways on top of the upstream repository.

Once sure that no upstream incoming are pending and you ready for merge into upstream push to your bitbucket repo and submit a pull request.



Linux 64 Bit

------------



Development system:

-------------



3.19.0-47-generic #53~14.04.1-Ubuntu SMP Mon Jan 18 16:09:14 UTC 2016 x86_64 x86_64 x86_64 GNU/Linux

Development system is UBUNTU 14.04 LTS (testing). The compiler is gcc-4.8. If starting with a new Stretch you will need to install
Library archives for the most part are built with gcc-4.6.4.  except Boost gcc-4.7 

3.19.0-47-generic #53~14.04.1-Ubuntu SMP Mon Jan 18 16:09:14 UTC 2016 x86_64 x86_64 x86_64 GNU/Linux

--------------

Preprations to build:

--------------

sudo apt-get update

sudo apt-get upgrade
libX11-dev libxinerama-dev libxml2-dev libxrender-dev libpulse-dev libalut-dev 

sudo apt-get install --install-recommends bison bzip2 cmake curl flex g++-4.8 m4 mercurial python2.7 python2.7-dev python-pip
gcc --version
gcc (Ubuntu 4.8.4-2ubuntu1~14.04) 4.8.4

sudo apt-get install --install-recommends libgl1-mesa-dev libglu1-mesa-dev libstdc++6 \
libX11-dev libxinerama-dev libxml2-dev libxrender-dev libpulse-dev libalut-dev 

Verify
gcc --version
gcc (Ubuntu 4.8.4-2ubuntu1~14.04) 4.8.4

Install autobuild into python

sudo pip install hg+https://bitbucket.org/lindenlab/autobuild-1.0#egg=autobuild

Install optional tools

sudo apt-get install --install-recommends tortoisehg kdiff3 mc

Optionally install gcc-version 4.6 which is needed to build library archives,

sudo apt-get install --install-recommends gcc-4.6 g++-4.6 cpp-4.6

If using ssh

mkdir ~/.ssh

copy you keys to this directory

cd ~/.ssh

sudo chmod 600 id_rsa

cd ~/

sudo chmod 700 .ssh

Voice libraries are already installed on 32 bit systems so, the steps can be skipped.

Voice 32 bit libraries are not needed to build the viewer but, are needed to test voice in the viewer.

sudo dpkg --add-architecture i386

sudo apt-get update
 
sudo apt-get install --install-recommends libasound2:i386 libasound2-plugins:i386 libasyncns0:i386 libattr1:i386 libc6:i386 libc6-i686:i386 libcap2:i386 libdbus-1-3:i386 libflac8:i386 libgcc1:i386 libice6:i386 libidn11:i386 libjson0:i386 libogg0:i386 libpulse0:i386 libsm6:i386 libsndfile1:i386 libstdc++6:i386 libvorbis0a:i386 libvorbisenc2:i386 libwrap0:i386 libuuid1:i386 libx11-6:i386 libx11-xcb1:i386 libxau6:i386 libxcb1:i386 libxdmcp6:i386 libxext6:i386 libxi6:i386 libxtst6:i386 zlib1g:i386 

ubuntu-14.04.3-desktop-i386.iso
 
Following applies to building 32 bit kokua by using a 32 bit virtual machine(vm). If building on 32 bit hardware -- follow 64 bit instructions from above.

Install a 32 bit vm from iso -- as of January 20, 2016 
ubuntu-14.04.3-desktop-i386.iso

Open your vm and follow instructions for 64 bit from above. 

Below is a sample ~/.hgrc (mercurial.ini) file. This uses tortoisehg or command line mercurial, kdiff3 as a merge tool and gedit as a visual editor.
The visual editor may be changed based on personal perference.


[tortoisehg]

defaultwidget = mq

opentabsaftercurrent = True

authorcolor = True

longsummary = True

[extensions]

hgext.bookmarks =

hgext.extdiff =

hgext.convert =

color =

fetch =

hgext.mq=

transplant=

eol =


[extdiff]

cmd.kdiff3 = 

[merge-tools]

kdiff3.priority=1

kdiff3.args=--L1 base --L2 local --L3 other $base $local $other -o $output

kdiff3.fixeol=True

kdiff3.gui=True

[tortoisehg]

authorcolor = True

vdiff = kdiff3

editor = gedit

[diff-tools]

kdiff3.diffargs=--L1 '$plabel1' --L2 '$clabel' $parent $child

[ui]

username = Nicky Perian <nickyperian@yahoo.com

ssh = ssh -C

[patch]

eol = auto

[hooks]

 #pretxncommit = python:~/hg-tools/coding_policy.py:hook
As an options add this to you bash history file ~/.bashrc

 #pretxnchangegroup = python:~/hg-tools/coding_policy.py:hook

----------------------
As an options add this to you bash history file ~/.bashrc

Otherwise enter into the terminal before autobuild configure step

Skip the export for 32 bit builds.

export AUTOBUILD_PLATFORM_OVERRIDE='linux64'

  KokuaNT can be built with opensource or properity audio engine. The opensource solution uses openal for sounds and gstreamer for streaming music and audio visual files like mp4's. Use of the properity FMOD Ex library for sounds and streaming audio is supported but, the FMOD Ex library must be provided separately.

-  hg clone https://NickyP@bitbucket.org/NickyP/kokuant

  KokuaNT can be built with opensource or properity audio engine. The opensource solution uses openal for sounds and gstreamer for streaming music and audio visual files like mp4's. Use of the properity FMOD Ex library for sounds and streaming audio is supported but, the FMOD Ex library must be provided separately.

-  Configure for an openal and gstreamer build:

Following assumes a clean build tree.

cd kokuant

Update the source tree to KokuaNT. This is a build without RLV or if you want RLV it would be hg update Kokua-MKRLV

hg update KokuaNT


autobuild configure -c ReleaseOS -- -DLL_TESTS:BOOL=OFF -DFMODEX:BOOL=OFF -DOPENAL:BOOL=ON -DPACKAGE:BOOL=ON 2>&1 |tee configure.log

- Build the viewer

-autobuild build -c ReleaseOS 2>&1 |tee build.log

- Configuration and building takes about 1.5 hours on a 2 core machine.

- Test the build

cd build-linux-x86_64/newview/packaged

Install the viewer with

sudo ./install.sh follow the defaults

This places the viewer in /opt/kokua-install and places a Kokua menu entry under Applications->Internet

sudo is the perferred method as the chrome-sandbox a part of Chrome Embedded Framework requies root permissions.

To be published

Windows

To be published

--------

Mac

--------

The current development environment is XCode 7.3 running on OS X 10.11.5 where the viewer is built 32-bit with OS X SDK 10.11 and Deployment Target of OS X 10.9.

We are still building for i386 as there are dependencies in the LL code to old Carbon framework that prevents successful 64-bit build. A 32-bit build is not such a big issue on OS X as the application can use the full 32-bit address space of 4 GB which should be sufficient unless you run in an OpenSim grid with very large VAR regions.

You can both build from the command line in terminal or build the Xcode project that is generated during configuration. 

NOTE: On Upgrading XCode to a new version you probably should delete the Derived Data folder that is found in Development/XCode as it may contain links to old system library locations preventing your build from linking.

	-- 

Linden’s version of autobuild requires a different version of Python than the system installed so the best way to get it installed is to first install MacPorts from https://www.macports.org with the latest current release (2.3.4).

With MacPorts installed, in terminal install the following ports:

• sudo port install python27

• sudo port install py27-pip

• sudo port install cmake

When prompted by the installer run python_select to use the version you just installed. It will be installed in /opt/local/bin

If you have beta versions of Xcode installed then you also need to run code-select to make sure you use 7.3 for your build

--
Xcode should have created the directory ~/Library/Developer during installation. If not create it (or use the location of your choice) and shortcut it to the Finder sidebar. 
In terminal cd to the above directory and type the command:

hg clone https://bitbucket.org/lindenlab/autobuild

Pip Install auto build python dependencies by typing:

sudo pip install ‘hg+https://bitbucket.org/oz_linden/autobuild-metadata#egg=autobuild'

If everything goes well it should be installed in /opt/local/Library/Frameworks/Python.framework/Versions/2.7/bin/autobuild

To make life easier edit your .bash_profile and add the lines

alias autobuild="/opt/local/Library/Frameworks/Python.framework/Versions/2.7/bin/autobuild"

export AUTOBUILD=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/bin/autobuild

Then source your .bash_profile

--

To verify your build environment the best way forward is most likely to download the LL source by following the instructions on http://wiki.secondlife.com/wiki/Compiling_the_viewer_(Mac_OS_X_XCode_6.1)

You should be able to both use the Xcode project (easiest to verify) and the command line build. 

NOTE: Regardless of which configuration option you use on the command line the Xcode project will have the build mode set to Debug. To change this go to Product > Scheme > Edit Scheme (with ALL_BUILD selected) and change the Build Configuration to RelWithDebInfo or Release respectively

BUILD NOTE: When building in Xcode at some point the build will fail because it cannot find packages-info.txt. At this point just restart the build and it will continue from there.

The root cause of this is that it tries to run autobuild by spawning a shell from inside autobuild, but Xcode will not allow any other version than the system python to be called so autobuild will fail - it does not even find it.  For anything but a (final) release build this is not significant. This build has to be done from the command line. 

--

To build Kokua first download the Kokua source code with the following command in terminal:

hg clone https://NickyP@bitbucket.org/NickyP/kokuant 

You can configure the build with:

autobuild configure -c RelWithDebInfoOS — -DKDU:BOOL=FALSE -DFMODEX:BOOL=TRUE -DLL_TESTS:BOOL=FALSE -DOPENAL:BOOL=FALSE

or 

autobuild configure -c ReleaseOS — -DKDU:BOOL=FALSE -DFMODEX:BOOL=TRUE -DLL_TESTS:BOOL=FALSE -DOPENAL:BOOL=FALSE

When you have made sure your configuration is working (and compiles in Xcode) you can also compile on the command line by substituting configure with build in the two commands above. 

Disclaimer

-----

-      This software is not provided nor supported by Linden Lab, the makers of Second Life.