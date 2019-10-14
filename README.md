# godot-lportal
Portal rendering module for Godot 3.2\
_Work in progress_

![plane_lines](images/plane_lines.png)

Although not yet released, I am trying to make sure that committed versions are runnable (especially stable branch), so if you want to test out the system, simply follow the installation instructions. Note that the function definitions are still subject to change, but any rooms you build should be compatible with later versions.

Video of initial testing:\
https://www.youtube.com/watch?v=xF_3Fe2HRdk \
https://www.youtube.com/watch?v=NmlWkkhGoJA

_Feel free to leave suggestions / feature requests on the issue tracker, especially regarding ease of use._

## Current status
I am currently diverted from the main work on LPortal, trying to get some kind of lightmapping working in Godot for a demo. It turns out the current lightmap workflow is quite broken. I've spent several days trying to work around bugs in the version of xatlas godot is using, but with no luck, so I'm now going to resort to exporting a mesh from godot, unwrapping in blender, then reimporting back into godot.

This means a slightly more complex workflow, but it does potentially allow baking lightmaps in blender which should lead to a nicer result. The downside is that I won't have a bake for dynamic shadowing as objects move through the level.

It may seem strange to be spending so long getting a demo working, but getting a usable workflow is imperative both for testing, and for tutorials.

The standard non-baked dynamic lighting workflow is mostly working (if not perfectly optimized), and that may be preferable for most people on desktop, but it is my intention to create a highly optimized system that will run very fast on mobiles as well, allowing fast action 1st person shooters etc.

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
* Dealing with shadows from objects outside of view PARTIALLY DONE
* Building baked lightmap workflow ONGOING
* Bug fixing / testing ONGOING
* Shadow caster optimization
* Closable portals
* PVS (primary and secondary)

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
