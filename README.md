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
I am currently working on the lightmapping workflow. While this isn't strictly speaking a core part of LPortal, many users will want to use baked lighting with the system because it will offer the best performance, particularly on mobile.

The current Godot standard workflow for lightmapping needs a bit of work, and I wanted to improve it for LPortal. The Godot standard workflow is based around creating a lightmap per object, and the uv mapping (via xatlas) can produce dodgy results sometimes. Instead I've tried to create a more streamlined and optimal lightmapping workflow for use with LPortal.

For rendering efficiency on older hardware I decided to have several (or all) objects share the same lightmap. I have at long last managed to get this working. The system currently merges together all the static objects in each room to form one mesh. The user will have two options:

1) UV map the merged mesh within Godot using the inbuilt xatlas system, and lightmap using Godot's BakedLightmap node. This should be a fast workflow, suitable for low budget games / testing.
2) For higher production quality, LPortal can export the merged mesh as an .obj file, where it can be loaded into blender or your modelling package of choice, uv unwrapped, and lightmapped.

This allows far more extensive and accurate lighting, and ambient occlusion etc than would be possible within Godot. The uv mapped mesh can be re-exported from blender as an .obj ready for reloading by LPortal.

In both workflows, LPortal will disassemble the lightmapped UV mapped mesh back to the original geometry and objects with their original materials. This was not easy to get working! :)

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
