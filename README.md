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
The last public release of GES was v4.2.4 on November 10, 2013. Since then we have 
been watching the state of SDK 2013 code base recently released by Valve. This code
represents our steps to transition from SDK 2007 to this SDK 2013 code base. It is
currently a work in progress and we appreciate your help in fixing bugs and testing
releases on various Operating Systems.

## Navigating the Code
The main code base for GES is located in the following directories:

- /game/client/ges
- /game/server/ges
- /game/shared/ges

TODO: Write more specific code navigation readme

### What happened to the SDK??
GES does not agree with the way Valve setup their SDK repository. There are hundreds
of duplicated files and a ridiculous directory structure. As a result, we only used
the "multiplayer" part of the SDK and collapsed some directories to be easier to navigate.

## Third-Party Libraries
GES relies on several third-party libraries during compilation. We have an integrated
Python 2.6 interpreter to run our Gameplay Scenarios and AI. To drive this, we use
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
