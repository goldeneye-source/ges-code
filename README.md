# GoldenEye: Source
## About
A total conversion modification of Half-Life 2. We are a fan made artistic 
recreation, released for free, with a primary goal in mind;  To bring the 
memories and experiences from the original GoldenEye-64 back to life using 
the Source SDK 2013 Technology. As one of the premier and consistently released 
HL2 mods, GES has been seen out and about, reviewed by the best, and played by 
hundreds and thousands of PC gamers for Eight long years and running.

Visit our [website](http://www.geshl2.com) and [forums](http://forums.geshl2.com)!

## Current State
The last major public release of GES was v5.0 on August 12, 2016 using the 2007 Source SDK. 

## Navigating the Code
The main code base for GES is located in the following directories:

- /game/client/ges
- /game/server/ges
- /game/shared/ges

TODO: Write more specific code navigation readme

## How to build
### Prerequisites
You must have the Boost libraries (v1.49  or higher) installed on your system path such that CMAKE can find them.
You can download Boost [here](https://sourceforge.net/projects/boost/files/boost/) or through your distro's package manager.

You must also have CMAKE v2.8.12 or higher installed on your system, available [here](https://cmake.org/download/) or through
your distro's package manager.

### Linux Build Process
1. Checkout the repository using git
2. Open a terminal window in the root of the repository you just checked out
3. Initialize the submodules: ```git submodule init --recursive```
4. Build with CMake: ```mkdir build && cd build```
5. ```cmake -DCMAKE_INSTALL_PREFIX=[path_to_gesource] ..```
6. ```make install```

### Windows Build Process
1. Checkout the repository using git and initialize the submodules (recommend TortoiseGit)
2. Create a build directory in the repository's root
3. Open CMAKE GUI and configure the project for the visual studio version you are using (prefer 2015)
4. Open Visual Studio using the generated solution files
5. Build and install as usual

## Third-Party Libraries
GES relies on several third-party libraries during compilation. We have an integrated
Python 3.4 interpreter to run our Gameplay Scenarios and AI. To drive all this, we use
the Boost Python library. To play our music, we utilize FMOD since Valve's internal
sound functions do not operate across level transitions.

## Contributing
We will accept pull requests and bug reports from anyone. All we ask at this state
is to limit your requests to bug fixes and compilation fixes. New capability and major
code revisions will not be accepted until further notice.

## Code License
GoldenEye: Source original code (files starting with 'ge_') is licensed under the
GNU GPL v3. The full license text can be found in LICENSE_GES.

It's important to know that this code is supplied WITH ABSOLUTELY NO WARRANTY WHATSOEVER.
We are not responsible for any damages resulting from the use of this code. Please see the
license for more information.

TODO: Replace file headers to comply with GNU GPL v3
