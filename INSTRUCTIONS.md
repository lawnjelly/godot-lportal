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

### DOBs (Dynamic objects)
While most of the world in the room portal is assumed to be static (non-moving), you will inevitably want some objects to move around in the scene. These objects that can move _between_ rooms are special, the visibility system has to keep track of which room they are in in order to render them correctly.

Players, boxes, props, physics objects are all examples of candidates for DOBs, as well as the most crucial example, the camera itself. Unless the visibility system knows which room the camera is in, the system will not know what to render! We will use the camera as an example as you will need to register this as a DOB.

Somewhat counter-intuitively, all DOBs should be maintained in your game __OUTSIDE__ the LRoomManager subtree. I.e., __NOT__ in a room. This is because the relationship between the DOB and the room is a soft one, the DOB can move between rooms. This is also handy because it makes it easier to use things like pools for your dynamic objects, because you don't have to worry about them being in different rooms in different places in the scene graph.

All the following functions should be called on the LRoomManager node.

* Call `dob_register(cam, 0.0)` to register a DOB to be handled

The number 0.0 is the radius of the DOB in the visibility system. All DOBs are managed as spheres. For a camera the radius can be zero because it will never be visible (however it DOES require to be a DOB so that the system can keep track of which room it is in).

If a DOB is being culled (popping out of view) when it should not, it is normally because the bounding radius needs to be larger. On the other hand, too large a radius will make the DOB render when it is not necessary, so it is a good idea to tweak this value. It is in Godot world units, so you may be able to simply measure you object in the IDE. 

* Each frame, call `dob_update(cam)` to keep that DOB updated in the system

If the DOB is not moving, or you want to deactivate it to save processing, simply don't call update again until you want to reactivate it.

* When you have finished with a DOB you should call `dob_unregister(cam)` to remove the soft link from the system. This is more important when you are creating and deleting DOBs (say with multiple game levels). If you are using a DOB throughout (say a camera) which gets deleted at the end of the app, it is not necessary to call unregister.

* If you are suddenly moving a DOB a large distance (rather than into a neighbouring room), you should call `dob_teleport(cam)`. This uses a different system to estimate the new room that the DOB is in.

### Conversion
Build your rooms and portals and placed them as children in a scene graph structure something like this:

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
  MyDOBs
    Camera
    Player
    Monster
    Box
  AnythingElse
```
* Change the RoomManager node type from a spatial to an LRoomManager in the Godot IDE

* At startup / when you load the level, call `rooms_convert()`

This will provide some output to indicate the building of the optimized internal visibility structure.

* Set which camera you want LPortal to use by calling `rooms_set_camera(cam)`

* Register the camera as a DOB (see above section on DOBs) and update each frame.

* If you load a new game level, simply call `rooms_convert()` again (which clears the old data), and repeat each step.

### Notes
* The most involved step is building your original rooms and portals, and getting the names right. Watch the debug output in the console when converting the rooms, it will let you know or give indications where it is having trouble converting.

#### Getting maximum performance
LPortal is very efficient, and the culling process itself is only likely to be a bottleneck if you are creating too high a density of portals. On each frame, every portal the camera 'sees' through creates a new set of clipping planes for each edge of the portal, which can be a lot to check when you are seeing an object through a door, through a window, through another window etc. So bear this in mind when building levels.

Portal polygons with fewer edges are also faster to cull against. So usually an axis aligned quad will make a lot of sense, even when covering an irregular opening (say a cave).

Level design is thus a balancing act between creating a higher density of rooms / portals (with greater occlusion culling accuracy), and a greater number of clipping planes. In practice there is also the issue of drawcalls, often hardware is limited by how many objects it can draw performantly in a frame - often it is faster to draw a bunch of small objects together than to cull them.
