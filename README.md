# godot-lportal
Portal rendering module for Godot 3.2\
_Work in progress_

![plane_lines](images/plane_lines.png)

Although not yet released, I am trying to make sure that committed versions are runnable, so if you want to test out the system, simply follow the installation instructions. Note that the function definitions are still subject to change, but any rooms you build should be compatible with later versions.

Video of initial testing:\
https://www.youtube.com/watch?v=xF_3Fe2HRdk \
https://www.youtube.com/watch?v=NmlWkkhGoJA

_Feel free to leave suggestions / feature requests on the issue tracker, especially regarding ease of use._\
_If you are interested in integrating LPortal into your game, feel free to get in touch, e.g. in the issue tracker, and I will endeavour to help._

## Current status
The basic system is working, I am now currently working on shadow casters. In a simple system you would pre-bake your lighting with e.g. lightmaps, however with realtime lights, shadow casters outside the view of the camera can cast shadows onto the objects in view. Rather than rendering everything as a possible shadow caster, it makes sense to use the same room / portal system to cull out as many shadow casters as possible that could not contribute to the final image.

Note that I've had to move from showing and hiding objects, to using Godot object and camera layers to differentiate between which objects to render from the camera and which for shadows, at least for now. This is a little hacky, and might interfere with your own code if you are using godot layers - layer 1 (default layer) is unset for all objects, and 19 and 20 are currently used by the system to determine whether objects are shadow casters or visible (or both), respectively.

I've also uncovered a bug in Godot core shadow casting:
https://github.com/godotengine/godot/pull/32475

which I'm finding the best fix for to help with the showing / hiding of shadow casters. There is a workaround but I'm keen to get it fixed in core.

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
* Add debug graphical view of portal planes DONE
* Add debug graphical view of room bounds DONE
* Dealing with shadows from objects outside of view
* Bug fixing / testing ONGOING
* Closable portals
* PVS (primary and secondary)
* Investigate multiple passes (shadows, lights)

## Instructions
See [INSTRUCTIONS.md](INSTRUCTIONS.md)  and [TUTORIAL.md](TUTORIAL.md)

## Installation
You will need to compile Godot from source (for now). See:
http://docs.godotengine.org/en/3.0/development/compiling/index.html

Once the engine is compiling okay on your system, to add the module:
* Create a folder inside godot/modules called 'lportal'
* Clone / download this repository as a zip file and place the files in the lportal folder
* Compile the engine as normal, it should automatically pick up the lportal module
* Note that to export to other platforms you will also have to compile export templates for those platforms

You will know the installation was successful when you see a new Node type 'LRoomManager' in the Godot IDE.
