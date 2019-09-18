# godot-lportal
Portal rendering module for Godot 3.2\
_Work in progress_

Although not yet released, I am trying to make sure that committed versions are runnable, so if you want to test out the system, simply follow the installation instructions. Note that the function definitions are still subject to change, but any rooms you build should be compatible with later versions.

Video of initial testing:\
https://www.youtube.com/watch?v=xF_3Fe2HRdk \
https://www.youtube.com/watch?v=NmlWkkhGoJA

_Feel free to leave suggestions / feature requests on the issue tracker, especially regarding ease of use._

## Current status
The system is mostly working. I am now testing / polishing the interface and adding a few features. I will make a first release before implementing PVS as PVS is an optional feature.

## Roadmap
* Auto conversion of named room spatials and portal mesh instances to LRoom and LPortal DONE
* Auto creation of mirror portals DONE
* Recursive determine visibility DONE
* Prevent memory allocations (use pools for plane vectors) DONE
* Add support for objects moving between rooms - cameras, players, physics etc - DONE
* Refactor code, moving LRooms and LPortals outside scene graph DONE
* Cleanup code, Optimize DONE
* Handle special cases (multiple portals views into room etc) DONE
* Optimize non-moving statics DONE
* Optional convex hull bound for rooms DONE
* Bug fixing / testing ONGOING
* Closable portals
* PVS (primary and secondary)
* Investigate multiple passes (shadows, lights)

## Instructions
See INSTRUCTIONS.md and TUTORIAL.md

## Installation
You will need to compile Godot from source (for now). See:
http://docs.godotengine.org/en/3.0/development/compiling/index.html

Once the engine is compiling okay on your system, to add the module:
* Create a folder inside godot/modules called 'lportal'
* Clone / download this repository as a zip file and place the files in the lportal folder
* Compile the engine as normal, it should automatically pick up the lportal module
* Note that to export to other platforms you will also have to compile export templates for those platforms

You will know the installation was successful when you see a new Node type 'LRoomManager' in the Godot IDE.
