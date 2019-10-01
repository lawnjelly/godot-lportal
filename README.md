# godot-lportal
Portal rendering module for Godot 3.2\
_Work in progress_

![plane_lines](images/plane_lines.png)

Although not yet released, I am trying to make sure that committed versions are runnable, so if you want to test out the system, simply follow the installation instructions. Note that the function definitions are still subject to change, but any rooms you build should be compatible with later versions.

Video of initial testing:\
https://www.youtube.com/watch?v=xF_3Fe2HRdk \
https://www.youtube.com/watch?v=NmlWkkhGoJA

_Feel free to leave suggestions / feature requests on the issue tracker, especially regarding ease of use._

## Current status
The basic system is mostly working. I am now testing / polishing the interface and adding debugging. I will leave PVS and closable portals until after the first release.

I am now working on shadows. Shadows can be cast by objects that are not 'in view', thus with the naive culling system, in some circumstances shadows will pop into and out of view.

There are three possibilities here:
1) Draw everything in the shadow pass (use Godot default octree). This gets rid of artefacts but would be slow and negate a lot of the benefits of visibility culling.
2) Draw everything roughly around the visible objects. The current version does this now, with the main pass only rendering objects in view, but the shadow pass draws everything within the rooms in view. This is better than naive version, but still can suffer 'popping' of shadows at doorways.
3) More intelligent system to try and work out which shadow casters are relevant to the view.

Note that I've had to move from showing and hiding objects, to using Godot object and camera layers to differentiate between which objects to render from the camera and which for shadows, at least for now. This is a little hacky, and might interfere with your own code if you are using godot layers - layer 1 (default layer) is unset for all objects, and 19 and 20 are currently used by the system to toggle visibility internally.

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
