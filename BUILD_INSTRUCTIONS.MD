# ** Dayturn Windows Build Instructions **

This [repository](https://bitbucket.org/dayturn/dayturn-windows) is dedicated to build a Windows version of the Dayturn viewer for OpenSim grids and standalone installations.

At this time there also exist [repository](https://bitbucket.org/dayturn/dayturn) for a macOS version of the Dayturn viewer.

The Linux version is currently not developed, but people who want develop such a version are referred to the KokuaOS [repository](https://bitbucket.org/dayturn/kokua-opensim) as the starting point.  

 
There are two active bookmarks; dayturn and dayturn-mkrlv for developing versions without and with support for Marine Kelley's RLV.  


**Development system:**

Linden Lab's development system for their viewer is referenced in: https://wiki.secondlife.com/wiki/Visual_Studio_2013_Viewer_Builds

*NOTE* Their development system over time is diverging from our because they are geared to do their builds on a build farm that individual builders usually don't have access to. Because of this the repository will not build with their autobuild version 1.1 or higher.


**You will need these items before you begin:**

-  An installer for Windows 7 Pro 64-bit

-  A valid Windows Product key

-  An installer for Visual Studio 2013

-  A valid license for Visual Studio 2013

- Install Windows 7 Pro 64-bit using your own product key

- Keep running Windows Update (Start Menu -> All Programs -> Windows Update) until clicking on "Check for Updates" there tells you everything is up to date.

- Depending on the age of the install media you started with, this could take a really long time and many, many iterations.



***Microsoft Visual Studio 2013 Pro***

- Install VS 2013 Pro

- Note: If you don't own a copy of VS 2013 Pro, you might consider installing the Community Version from
  [https://visualstudio.microsoft.com/vs/older-downloads/ ] as the Express version does not have devenv.exe
  and therefore will not be able to build the viewer. 

- Run the installer as Administrator (right click, "Run as administrator")

- Uncheck all the "Optional features to install:" - they are not required

- Download and install VS2013 Service Packs and updates via Software Update

- is the most recent '''released''' version at time of writing (2015-01)

- Run the installer as Administrator (right click, "Run as administrator")

***DirectX SDK***

- Download and install [http://www.microsoft.com/en-us/download/details.aspx?id=6812 DirectX SDK (June 2010)]

- Run the installer as Administrator (right click, "Run as administrator")

- At the Installation Options screen, set everything except the DirectX Headers and Libs to "This feature will not be installed"

***Tortoise Hg***

- Download and install [http://tortoisehg.bitbucket.org/download/ TortoiseHg 3.2.3] (64-bit)

- Note: No option available to install as Administrator

- Use default options (path, components etc.)

***CMake***

- Download and install [http://www.cmake.org/download/ CMake 3.1 or higher] (32bit is only option)

- Run the installer as Administrator (right click, "Run as administrator")

- At the "Install options" screen, select "Add CMake to the system PATH for all users"

- For everything else, use the default options (path, etc.)

***Cygwin***

- Cygwin is not required for current builds, but it makes everything easier if you use it for your builds. If using the VS2013 IDE or developer shell you may have to tweak your build procedure accordingly.

- Download and install [http://cygwin.com/install.html Cygwin 64] (64-bit)

- Run the installer as Administrator (right click, "Run as administrator")

- Use default options (path, components etc.) *until* you get to the "Select Packages" screen

- Add additional packages:

- Devel/bison

- Devel/flex

- Devel/patch

- Use default options for everything else


***Python***

- Download and install [https://www.python.org/ftp/python/2.7.8/python-2.7.8.msi Python 2.7.8 (32bit)] *Do not install version 2.7.9 as it doesn't work for our setup currently*

- Note: No option available to install as Administrator

- Use default options (path, components etc.) *until* you get to the "Customize Python" screen

- Change "Add python.exe to Path" to "Will be installed on local hard drive"

***Intermediate check***

- Confirm things are installed properly so far|

- Open a Cygwin terminal and type:

-  bison --version

-  cmake --version

-  flex --version

-  hg --version

-  python --version

- If they all report sensible values and not "Command not found" errors, then you are in good shape}}

***Set up Autobuild and Python***

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

-   Use autobuild-1.0

-       Run pip install hg+http://bitbucket.org/lindenlab/autobuild-1.0#egg=autobuild

-   Autobuild will be installed.  Earlier versions of autobuild could be made to work by just putting the source files into your path correctly; this is no longer true - autobuild must be installed as described here.

- Update system PATH

-  Add Python Scripts folder to PATH environment variable via the Control Panel

-  Typically C:\Python27\Scripts


***NSIS (Unicode)***

-  You must install the Unicode version http://www.scratchpaper.com/ and not the one from the NSIS page

-  Not strictly required for developers (although it's useful)


***Check Paths in VS 2013 and CYGWIN***

- Open Developer Command Prompt for VS2013 C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\VsDevCmd.bat

- This command prompt is kinda hard to find on newer windows versions. Use File Explorer to locate VsDevCmd.bat then,

- right click and send shortcut to Desktop.

- In Developer Command Prompt for VS2013 or in CYGWIN

- Verify that these version request.

	-  cmake --version

	-  hg --version

	-  python --version

	-  autobuild --version


***Get source and build in CYGWIN***

-  Stable source code repositories

-   OpenSim https://bitbucket.org/dayturn/dayturn-windows

-   Use mercurial to clone the repository 

-   hg clone https://bitbucket.org/dayturn/dayturn-windows

-   This will place a folder with the source code typically in /Users/xxxx/dayturn-windows 

-   cd dayturn-windows 

-   Check that you have the two bookmarks with 		hg bookmark

-   Change to the dayturn bookmark with
	 	hg update -C --rev dayturn

-   Run
		autobuild configure -c ReleaseOS -- -DFMODEX:BOOL=TRUE -DLL_TESTS:BOOL=FALSE -DOPENAL:BOOL=FALSE -DPACKAGE=ON -DVIEWER_CHANNEL="\"Release\""

-   Run	
    	autobuild build -c ReleaseOS
 
-  The above will produce a command line build with an install program.

-  To test the build without running the full installer, in folder /Users/xxxx/dayturn-windows/build-vs/newview/Release find, right click on file dayturn-bin.exe and select Send to->Desktop (create shortcut)

-  Select Desktop and right click the dayturn-bin.exe shortcut and click Properties.

-  Edit Start in to "C:\Users\xxxx\dayturn-windows\indra\newview". This allows use of working tree skin and settings files.

-  Click on the dayturn.exe shortcut and the viewer should startup and run.

***Optional Visual Studio 2013 build***

-  After the autobuild configure step, start Visual Studio 2013 and open the solution from C:\Users\xxxx\dayturn-windows\build-vs\Dayturn.sln


***Disclaimer***

*This software is not provided nor supported by Linden Lab, the makers of Second Life.*







