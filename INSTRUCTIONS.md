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
Rooms are simply spatials, whose children should be the mesh instances that are static (non-moving) within the room. The naming of the room spatial is crucial in order for it to be recognised. All rooms should start with 'room_'.
e.g.:

* room_kitchen
* room_hall
* room_bathroom1

You can use any character within the name (except '*', that is reserved, see the portal naming).

### Portals
In order to calculate the visibility between rooms (and the objects within rooms) you need to manually specify the location and shape of portals that should join the rooms. These should be thought of as doorways, or windows between rooms (and often cover exactly these features).

Portals are mesh instances, but have very strict requirements, and again naming conventions.

* Each portal should be a single-sided CONVEX polygon, on a single plane.
* The portal should be a child of the room it links from.
* Single-sided means it can only be seen from one direction. It should face out from the room it is a child of.
* You only need to create one portal per opening between rooms, rather than one in each room.

The mirror portal will be created automatically. This saves on editing in the modelling package. You can create the portal in either of the adjoining rooms.
* The name of all portals must start with 'portal_' followed by the name of the room (not including the 'room_' prefix) that the portal should link to. e.g. 'portal_kitchen', 'portal_hall', 'portal_bathroom1'

#### Portal naming
There is a special case for naming portals - because Godot cannot handle more than one node with the same name!

If you had several portals named 'portal_kitchen', Godot would not allow it, and would add characters to the name, and screw up the system. To get around this, and allow unique names for each portal, LPortal will ignore anything in the portal name after a '*' character. e.g.:

* portal_kitchen*0
* portal_kitchen*from_the_hall
* portal_kitchen*it_is_sunny_today

You get the idea, you can use whatever scheme you want to make the name unique. Something like using the room the portal comes from, and a number is probably sensible, e.g. 'portal_kitchen*hall0' but it is totally up to you.

### Conversion
Once you have built your rooms and portals and placed them as children in a structure as follows:

```
Root
  RoomManager
    room_kitchen
      MeshInstance (table?)
      MeshInstance (walls?)
      MeshInstance (floor, ceiling)
      MeshInstance (chair?)
      portal_hall
    room_hall
      MeshInstance (painting?)
      MeshInstance (floor, ceiling)
```
* Change the RoomManager node type from a spatial to an LRoomManager in the Godot IDE
* At startup / when you load the level, call LRoomManager.rooms_convert
