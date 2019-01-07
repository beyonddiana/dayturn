This repository is used to develop a *Windows only* codebase for an OpenSim compatible viewer. 

The repository is a refinement of a fork of the Kokua viewer which was made at the point in 2015 just before Linden Labs removed support for Curl from the viewer, and added the http coroutines. 

That fork is called Dayturn and is the base to develop a generic viewer for connecting to OpenSim grids and standalone instances leaving out SecondLife specific functionality. 
Dayturn for Windows exists in https://bitbucket.org/dayturn/dayturn-windows

Because of the complexity of having a shared codebase for Linux, macOS and Windows, the code in this repository has been stripped of 
all Linux and macOS specifics and is only suitable for a Windows version of the viewer. 

Linden Lab has essentially stopped all development of a Linux viewer, and there is an increasing degree if incompatibility of Apple's direction for macOS and Windows, where Apple in 2018 deprecated support for OpenGL. It is therefore better to maintain a pure-play Windows code base for the Windows version of the viewer. 

This version of the viewer does not have the so-called SecondLife Jelly Dolls. 

Support for the Linden Lab version 2.0 rigging and skeleton, Bento, was added in 2018.

This version of the viewer has support for Marine Kelly's RLV.

Now that the support for "legacy" assets has been removed from SecondLife, the code for the SecondLife unified marketplace and pathfinding will be removed, in addition to numerous other SecondLife tie-ins and functionality that don't apply to OpenSim users or grids.

Currently the macOS repository are many hundred commits ahead of this repository with major additions such as support for Linden Lab's animated mesh, AniMesh. It also can produce a 64-bit version of the viewer. It can be expected that the two repositories will converge for functional support over relatively short time. 
