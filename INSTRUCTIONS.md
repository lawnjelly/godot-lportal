# LPortal Instructions

## About
One of the most requested features for Godot 3 has been occlusion culling (in some form). It is on the roadmap to be added to the core of the engine in Godot 4.0, but until that is available, I figured that I could come up with an interrim solution that would allow the production of some of the games that need this.

There are several approaches to occlusion culling, but one of the oldest and tried and tested techniques is the use of rooms (or 'cells') and portals. There was some support for portal rendering in Godot 2 but I have implemented this new system from scratch. Portal rendering was particularly attractive as an interrim solution partly because it is easier to integrate with the existing engine, and partly because I have done it before(!).

There is a simple explanation here of the concept:

https://www.youtube.com/watch?v=8xgb-ZcZV9s

LPortal is a portal rendering system for Godot 3.2. It may compile with 3.1, but this is untested / supported.

## Goals
* Easy to use
* Easy to iterate levels (re-exporting full levels from e.g. blender after editing)
* Fast (good algorithms, no dynamic allocation and cache friendly)

## The details
The first thing you may notice is that LPortal doesn't make much use of special features within the editor, and simply relies on careful naming of spatials (or Empties within Blender). There is one new node type, 'LRoomManager', which behaves pretty much as any spatial however it has some added functions and runs an entire visibility system under the hood.

The reason for not creating bespoke editing within Godot is twofold:
1) This is a non-official plugin to the engine, without modifying core, or creating a specific importer
2) Using the node names to denote rooms and portals allows the whole level to be edited (and more importantly re-edited) within a modelling package such as blender.*

* _Note that although the whole game level can be created in the modelling package, this is optional, you can also create it in the Godot editor._

### The room manager
Within your scene graph, you will be building all your rooms as children of the same spatial, which you can convert to an LRoomManager in the Godot IDE. To avoid problems it is best to make sure you only have rooms as children of the room manager.

### Rooms


