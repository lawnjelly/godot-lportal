# LPortal Instructions
LPortal is a portal rendering system for Godot 3.2. It may compile with 3.1, but this is untested / supported.

## About
One of the most requested features for Godot 3 has been occlusion culling (in some form). It is on the roadmap to be added to the core of the engine in Godot 4.0, but until that is available, I figured that I could come up with an interrim solution that would allow the production of some of the games that need this.

There are several approaches to occlusion culling, but one of the oldest and tried and tested techniques is the use of rooms (or 'cells') and portals. Portal rendering is particularly useful in indoor scenes, especially 1st / 3rd person games. It is not so useful in outdoor scenes. There was some support for portal rendering in Godot 2 but I have implemented this new system from scratch. Portal rendering was particularly attractive as an interrim solution partly because it is easier to integrate with the existing engine, and partly because I have done it before(!).

There is a simple explanation here of the concept:

https://www.youtube.com/watch?v=8xgb-ZcZV9s

## Goals
* Easy to use
* Easy to iterate levels (re-exporting full levels from e.g. blender after editing)
* Fast (good algorithms, no dynamic allocation and cache friendly)

As with most room portal systems, LPortal works better if rooms are convex. Most portal systems require you to specify the convex hull (bound) for each room. In LPortal this is optional. The convex hull is only required for calculating the 'start room' of a dynamic object when you either initially place it (dob_register), or teleport it (dob_teleport). As such, both of these functions have an alternative hint version where the user can specify the start room manually. This is possible in some simpler designs of games, and avoids the laborious need to model room bounds (convex hulls).

If you are exclusively using the hint commands, and not creating bounds, there is strictly speaking no longer a requirement for the rooms to be convex, they can be concave shapes. This allows considerable flexibility in level design. HOWEVER, the portals should be placed such that no portal is in front of the portal plane of another portal in the room. I.e. the portals themselves should form some kind of convex structure.

## The details
The first thing you may notice is that LPortal doesn't make much use of special features within the editor, and simply relies on careful naming of spatials (or Empties within Blender). There is one new node type, 'LRoomManager', which behaves pretty much as any spatial however it has some added functions and runs an entire visibility system under the hood.

The reason for not creating bespoke editing within Godot is twofold:
1) This is a non-official plugin to the engine, without modifying core, or creating a specific importer
2) Using the node names to denote rooms and portals allows the whole level to be edited (and more importantly re-edited) within a modelling package such as blender.*

*_Note that although the whole game level can be created in the modelling package, this is optional, you can also create it in the Godot editor._

### The room manager
Within your scene graph, you will be building all your rooms as children of the same spatial, which you can convert to an LRoomManager in the Godot IDE. To avoid problems it is best to make sure you only have rooms as children of the room manager.

### Rooms
Rooms are simply spatials, whose children should contain the mesh instances that are static (non-moving) within the room. The naming of the room spatial is crucial in order for it to be recognised. All rooms should start with 'room_'.
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

### Room Bounds
When you first add an object to a room at runtime, it has to decide which room it should be in.

You have three choices here:

1) Manually supply the convex hull (bound) for a room
2) Do not supply a bound, in which case LPortal will simply choose the nearest room centre to the object. _(this can choose the wrong room when rooms are different sizes, for instance a hallway by a large room, so it is not recommended)_
3) Use the dob_register_hint and teleport_hint versions, where the user specifies the start room

Approach (1) is the most powerful and flexible, but requires creating a bound, either in your modelling package or within Godot IDE.

The bound should be stored as a MeshInstance child of the room, with a specific naming convention, it should start with the prefix 'bound_'. You can put anything after the prefix (or leave as is).

Although the bound will be treated as a convex hull at runtime, you don't have to be perfect when creating it because the geometry will be run through the quickhull algorithm.

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

* DOBs should only move between rooms via the portals. In fact that is how their movement between rooms is defined. This is why a room's portals should form a convex space, never concave. In order to limit movement between rooms to the portals, you should use e.g. physics, or a navmesh.

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
* You can use hierarchies of nodes to place your static MeshInstances within a room, so it is easier to place groups e.g.:
```
room_office
  Desk (spatial)
    Table (mesh instance)
    Chair (mesh instance)
```

#### Debugging
A significant portion of LPortal is devoted to debugging, as without feedback it is difficult to diagnose problems that are occurring. The debugging occurs in 2 stages - the initial conversion, and at runtime, it will provide the visibility tree when you request debug output for a frame with rooms_log_frame().

A sample conversion is as follows:
```
running convert
Convert_Rooms
Convert_Room : room_myroom_x0y0
		Found VI : WallsAndFloor
		Found VI : Box
		Found VI : Box2
			myroom_x0y0 centre 0, 1, 0
Convert_Room : room_myroom_x1y0
		Found VI : WallsAndFloor
		Found VI : Box
		Found VI : Box2
			myroom_x1y0 centre 4, 1, 0
Convert_Room : room_myroom_x0y1
		Found VI : WallsAndFloor
		Found VI : Box
		Found VI : Box2
			myroom_x0y1 centre 0, 1, 4
Convert_Room : room_myroom_x1y1
		Found VI : WallsAndFloor
		Found VI : Box
		Found VI : Box2
			myroom_x1y1 centre 4, 1, 4
Convert_Portals pass 0

DETECT_PORTALS from room myroom_x0y0
	detected to myroom_x0y1
	detected to myroom_x1y0
	detected to myroom_x1y0
DETECT_PORTALS from room myroom_x1y0
	detected to myroom_x1y1
	detected to myroom_x2y0
	WARNING : portal to room myroom_x2y0, room not found
	detected to myroom_x2y0
	WARNING : portal to room myroom_x2y0, room not found
DETECT_PORTALS from room myroom_x0y1
	detected to myroom_x0y2
	WARNING : portal to room myroom_x0y2, room not found
	detected to myroom_x1y1
	detected to myroom_x1y1
DETECT_PORTALS from room myroom_x1y1
	detected to myroom_x1y2
	WARNING : portal to room myroom_x1y2, room not found
	detected to myroom_x2y1
	WARNING : portal to room myroom_x2y1, room not found
	detected to myroom_x2y1
	WARNING : portal to room myroom_x2y1, room not found
Convert_Portals pass 1

MAKE_PORTALS_TWOWAY from room myroom_x0y0
	contains 3 portals
	considering portal myroom_x0y1
		creating opposite portal
	considering portal myroom_x1y0
		creating opposite portal
	considering portal myroom_x1y0
		creating opposite portal
MAKE_PORTALS_TWOWAY from room myroom_x1y0
	contains 3 portals
	considering portal myroom_x1y1
		creating opposite portal
	considering portal myroom_x0y0
		is MIRROR, ignoring
	considering portal myroom_x0y0
		is MIRROR, ignoring
MAKE_PORTALS_TWOWAY from room myroom_x0y1
	contains 3 portals
	considering portal myroom_x1y1
		creating opposite portal
	considering portal myroom_x1y1
		creating opposite portal
	considering portal myroom_x0y0
		is MIRROR, ignoring
MAKE_PORTALS_TWOWAY from room myroom_x1y1
	contains 3 portals
	considering portal myroom_x1y0
		is MIRROR, ignoring
	considering portal myroom_x0y1
		is MIRROR, ignoring
	considering portal myroom_x0y1
		is MIRROR, ignoring
Convert_Portals pass 2

	CONVERT_BOUND : 'bound_ofroom' for room 'myroom_x0y0'
			contained 6 planes.
	CONVERT_BOUND : 'bound_ofroom' for room 'myroom_x1y0'
			contained 6 planes.
	CONVERT_BOUND : 'bound_ofroom' for room 'myroom_x0y1'
			contained 6 planes.
	CONVERT_BOUND : 'bound_ofroom' for room 'myroom_x1y1'
			contained 6 planes.
```
You can see that conversion takes place in several stages:

1) Finding the rooms and visual instances (meshes etc)
2) Detecting portal meshes
3) Making portals two way
4) Detecting room bounds

_(Note that if you are reading the log in the Godot IDE you may need to bump up the value of network/limits/debugger_stdout/max_chars_per_second in the project settings, to prevent getting an output overflow message)_

A sample visibility tree for a frame is as follows:
```
FRAME 172

ROOM 'myroom_x0y0' planes 6 portals 3
	DOB Camera2 culled
	PORTAL 0 (0) myroom_x0y1 normal 0, -0, 1
	
	ROOM 'myroom_x0y1' planes 10 portals 3
		DOB @Monster@4 culled
		PORTAL 0 (6) myroom_x1y1 normal 1, -0, -0
			Outside plane 0.996257, 0, -0.086442, 0
			CULLED (outside planes)
		PORTAL 1 (7) myroom_x1y1 normal 1, -0, -0
			Outside plane 0.996257, 0, -0.086442, 0
			CULLED (outside planes)
		PORTAL 2 (8) myroom_x0y0 normal 0, 0, -1
			CULLED (wrong direction)
	PORTAL 1 (1) myroom_x1y0 normal 1, -0, -0
		Outside plane 0.996257, 0, -0.086442, 0
		CULLED (outside planes)
	PORTAL 2 (2) myroom_x1y0 normal 1, -0, -0
		Outside plane 0.996257, 0, -0.086442, 0
		CULLED (outside planes)
```
Visibility starts from the room the camera is in, checks the objects within against the clipping planes (just the view frustum at first), then checks each portal to see if it is visible. If it is visible it checks the room on the other side of the portal, and continues the process recursively. Each time it moves through a portal it adds clipping planes from the camera to the edges of the portal. This ensures that objects that are hidden by portal planes are not rendered.

You can show the portal culling graphically by calling `rooms_set_debug_planes(true)`.

#### Getting maximum performance
LPortal is very efficient, and the culling process itself is only likely to be a bottleneck if you are creating too high a density of portals. On each frame, every portal the camera 'sees' through creates a new set of clipping planes for each edge of the portal, which can be a lot to check when you are seeing an object through a door, through a window, through another window etc. So bear this in mind when building levels.

Portal polygons with fewer edges are also faster to cull against. So usually an axis aligned quad will make a lot of sense, even when covering an irregular opening (say a cave).

Level design is thus a balancing act between creating a higher density of rooms / portals (with greater occlusion culling accuracy), and a greater number of clipping planes. In practice there is also the issue of drawcalls, often hardware is limited by how many objects it can draw performantly in a frame - often it is faster to merge a bunch of small objects together than to cull them.

### Command reference
* `void rooms_convert()` - prepare lportal for rendering
* `void rooms_set_camera(Node * pCam)` - set which camera visibility is calculated from
* `Node * rooms_get_rooms(int room_id)` - returns godot room node from lportal room id
* `void rooms_set_active(bool bActive)` - turns on and off lportal
* `void rooms_log_frame()` - output debug logs for the next frame
* `void rooms_set_logging(int level)` - set debug logging level, 0 - 6 (0 is none, 6 is most)
* `void rooms_set_debug_planes(bool bActive)` - turns on and off graphic debugging of portal planes

* `bool dob_register(Node * pDOB, float radius)` - have lportal find start room
* `bool dob_register_hint(Node * pDOB, float radius, Node * pRoom)` - user provides start room
* `bool dob_unregister(Node * pDOB)`
* `int dob_update(Node * pDOB)` - returns room ID within
* `bool dob_teleport(Node * pDOB)` - have lportal find start room
* `bool dob_teleport_hint(Node * pDOB, Node * pRoom)` - user provides start room
* `int dob_get_room_id(Node * pDOB)` - return room ID the dob is currently within

