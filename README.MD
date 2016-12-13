## ** Kokua OpenSim ** ##
This repository is a baseline of the combined Linden Lab 4.0.2 SecondLife Viewer and Kokua 4.0.2 releases with select commits from the following Linden code base with the aim of providing a starting point for development of an OpenSim specific version of Kokua or derived viewers.

The reason for this is that the http changes in the Linden 4.0.3 release are interoperable with OpenSim core in Standalone mode or in Grid mode without assets served over http. Assets over http is still largely experimental in OpenSim and currently requires both more hardware and configuration skills than many small grids are willing or able to invest in. 

A pure-play OpenSim viewer will also over time evolve to a simpler code base removing functionality not supported by OpenSim, removing similar functionality with different implementations across the two environments, and adding better support for OpenSim specific functionality such as Hypergrid Teleports, variable size regions, NPCs, the OSSL scripting language and different physics engines. 

It will also remove the code for SecondLife specific functionality such as Pathfinding, a unified marketplace, experience keys, avatar complexity and information that is not directly relevant to OpenSim grids. 

There are two active branches KokuaNT and Kokua-MKRLV. 


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




Development system is UBUNTU 14.04 LTS (testing). The compiler is gcc-4.8. If starting with a new Stretch you will need to install
Library archives for the most part are built with gcc-4.6.4.  except Boost gcc-4.7 

3.19.0-47-generic #53~14.04.1-Ubuntu SMP Mon Jan 18 16:09:14 UTC 2016 x86_64 x86_64 x86_64 GNU/Linux


Preprations to build:

--------------

sudo apt-get update

sudo apt-get upgrade
libX11-dev libxinerama-dev libxml2-dev libxrender-dev libpulse-dev libalut-dev 

sudo apt-get install --install-recommends bison bzip2 cmake curl flex g++-4.8 m4 mercurial python2.7 python2.7-dev python-pip

sudo apt-get install --install-recommends pulseaudio
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


Windows

Reference: https://wiki.secondlife.com/wiki/Visual_Studio_2013_Viewer_Builds

You will need these items before you begin:

-  An installer for Windows 7 Pro 64bit

-  A valid Windows Product key

-  An installer for Visual Studio 2013

-  A valid license for Visual Studio 2013

- Install Windows 7 Pro 64bit using your own product key

- Keep running Windows Update (Start Menu -> All Programs -> Windows Update) until clicking on "Check for Updates" there tells you everything is up to date.

- Depending on the age of the install media you started with, this could take a really long time and many, many iterations.

==Microsoft Visual Studio 2013 Pro==

- Install VS 2013 Pro

- Note: If you don't own a copy of VS 2013 Pro, you might consider installing the 

- [http://www.visualstudio.com/en-us/news/vs2013-community-vs.aspx Community Version]

- Run the installer as Administrator (right click, "Run as administrator")

- Uncheck all the "Optional features to install:" - they are not required

- Download and install VS2013 Service Packs and updates

- [http://www.visualstudio.com/en-us/downloads/download-visual-studio-vs#DownloadFamilies_5 Update 4 ] 

- is the most recent '''released''' version at time of writing (2015-01)

- Run the installer as Administrator (right click, "Run as administrator")

==DirectX SDK==

- Download and install [http://www.microsoft.com/en-us/download/details.aspx?id=6812 DirectX SDK (June 2010)]

- Run the installer as Administrator (right click, "Run as administrator")

- At the Installation Options screen, set everything except the DirectX Headers and Libs to "This feature will not be installed"

==Tortoise Hg==

- Download and install [http://tortoisehg.bitbucket.org/download/ TortoiseHg 3.2.3] (64bit)

- Note: No option available to install as Administrator

- Use default options (path, components etc.)

==CMake==

- Download and install [http://www.cmake.org/download/ CMake 3.1.0] (32bit is only option)

- Run the installer as Administrator (right click, "Run as administrator")

- At the "Install options" screen, select "Add CMake to the system PATH for all users"

- For everything else, use the default options (path, etc.)

==Cygwin==

- Cygwin is not required for current builds. If you want to build older version of Kokua you may want to install it. Or, if you need the unix/linux tools.

- Download and install [http://cygwin.com/install.html Cygwin 64] (64bit)

- Run the installer as Administrator (right click, "Run as administrator")

- Use default options (path, components etc.) *until* you get to the "Select Packages" screen

- Add additional packages:

- Devel/bison

- Devel/flex

- Devel/patch

- Use default options for everything else

==Python==
- Download and install [https://www.python.org/ftp/python/2.7.8/python-2.7.8.msi Python 2.7.8 (32bit)] Do not install version 2.7.9

- it doesn't work for our setup currently

- Note: No option available to install as Administrator

- Use default options (path, components etc.) *until* you get to the "Customize Python" screen

- Change "Add python.exe to Path" to "Will be installed on local hard drive"

==Intermediate check==

- Confirm things are installed properly so far|

- Open a Cygwin terminal and type:

-  bison --version

-  cmake --version

-  flex --version

-  hg --version

-  python --version

- If they all report sensible values and not "Command not found" errors, then you are in good shape}}

==Set up Autobuild and Python==

-  This section only works inside the Windows Command Prompt. 

- Bootstrap pip 

-   Download (Save As) https://bootstrap.pypa.io/get-pip.py get-pip.py and copy to a temp folder

-   Open Windows Command Prompt

-   Switch to that temp folder and execute it <code>python get-pip.py</code>

-   Pip will be installed

- Bootstrap easy_install 

-   Download (Save As) https://bootstrap.pypa.io/ez_setup.py ez_setup.py and copy to a temp folder

-   Remain in Windows Command Prompt

-   Switch to that temp folder and execute it python ez_setup.py

-   easy_install will be installed

- Install Autobuild

-   Remain in Windows Command Prompt

-   Change to the Python Scripts folder that was just created

-   Typically cd \Python27\Scripts

-   Run pip install hg+http://bitbucket.org/lindenlab/autobuild-1.0#egg=autobuild

-   Autobuild will be installed.  Earlier versions of autobuild could be made to work by just putting the source files

-  into your path correctly; this is no longer true - autobuild must be installed as described here.

- Update system PATH

-  Add Python Scripts folder to PATH environment variable via the Control Panel

-  Typically C:\Python27\Scripts

==NSIS (Unicode)==

-  You must install the Unicode version http://www.scratchpaper.com/ and not the one from the NSIS page

-  Not strictly required for developers (although it's useful)

==Check Paths in VS 2013==

Open Developer Command Prompt for VS2013 C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\VsDevCmd.bat

This command prompt is kinda hard to find on newer windows versions. Use File Explorer to locate VsDevCmd.bat then,

right click and send shortcut to Desktop.

In Developer Command Prompt for VS2013

Verify that these version request.

-  cmake --version

-  hg --version

-  python --version

-  autobuild --version

==Get source and compile==

-  Stable source code repositories

-   OpenSim https://bitbucket.org/kokua/kokua-os

-   SecondLife https://bitbucket.org/kokua/kokua-sl

-  Development source code repositories

-   OpenSim https://bitbucket.org/nickyp/kokua-os

-   SecondLife https://bitbucket.org/nickyp/kokua-sl

-   Use mercurial to clone one of the above respostories to your local machine. For example: 

-   hg clone https://bitbucket.org/kokua/kokua-os

-   This will place a folder with the source code typically in /Users/xxxx/kokua-os

-   cd kokua-os 

-   hg update -rKokua-MKRLV

-   autobuild configure -c ReleaseOS -- -DCMAKE_VERBOSE_MAKEFILE:BOOL=FALSE -DLL_TESTS:BOOL=OFF \
    -DPACKAGE:BOOL=FALSE  -DOPENAL:BOOL=FALSE -DFMODEX:BOOL=ON
    
    autobuild build -c ReleaseOS
 
-  The above will produce a command line build without an install program.

-  In folder /Users/xxxx/kokua-os/build-vc120/newview/Release find an right click on file kokuaos-bin.exe and select Send to->Desktop (create shortcut)

-  Select Desktop and right click the kokuaos-bin.exe shortcut and click Properties.

-  Edit Start in to "C:\Users\xxxx\kokua-os\indra\newview". This allows use of working tree skin and settings files.

-  Click on the kokuaos-bin.exe shortcut and the viewer should startup and run.

-  Optionally, Visual Studio 2013 program may be used to build the viewer.

-  After the autobuild configure step, start Visual Studio 2013 and open the solution from C:\Users\xxxx\Kokua-OS\build-vc120\KokuaOS.sln

-  Follow the steps below to confirm or set the Start up project and Debugging working directory...

-  In the Solution Exployer click on kokua-bin, then from the main menu Project select Set as startup project.    

-  From main menu Project open kokuaos-bin Properties, then in kokuaos-bin Properties window chose Debugging, then Working directory.

-  At the right side a down arrow expanders. From there use Browse or Edit and set to /Users/xxxx/kokua-os/build-vc120/newview/Release

-  And Apply to write the property page to memory.

-  Under main menu DEBUG is a right pointing green arrow with text 'Local Windows Debugger' click to start building...

-  Once the build competes the viewer should be at the start page waiting to logon.
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