# LPortal Instructions
LPortal is a portal rendering system for Godot 3.2. It may compile with 3.1, but this is untested / supported.\
For simplified use without creating rooms / portals, please see [INSTRUCTIONS_SINGLEROOM.md](INSTRUCTIONS_SINGLEROOM.md).

## About
One of the most requested features for Godot 3 has been occlusion culling (in some form). It is on the roadmap to be added to the core of the engine in Godot 4.0, but until that is available, I figured that I could come up with an interrim solution that would allow the production of some of the games that need this.

There are several approaches to occlusion culling, but one of the oldest and tried and tested techniques is the use of rooms (or 'cells') and portals. Portal rendering is particularly useful in indoor scenes, especially 1st / 3rd person games. It is not so useful in outdoor scenes. There was some support for portal rendering in Godot 2 but I have implemented this new system from scratch. Portal rendering was particularly attractive as an interrim solution partly because it is easier to integrate with the existing engine, and partly because I have done it before(!).

There is a simple explanation here of the concept:

https://www.youtube.com/watch?v=8xgb-ZcZV9s

## Goals
* Easy to use
* Easy to iterate levels (re-exporting full levels from e.g. blender after editing)
* Fast (good algorithms, no dynamic allocation and cache friendly)

As with most room portal systems, LPortal works better if rooms are convex. Each room has a convex hull bound. This can either be auto generated from the room geometry by LPortal, or you can manually supply a bound shape.

The convex hull is only required for calculating the 'start room' of a dynamic object when you either initially place it (dob_register), or teleport it (dob_teleport). Both of these functions have an alternative hint version where the user can specify the start room manually, this can be more efficient and accurate in some cases, but usually the convex bounds will work fine.

If you are exclusively using the hint commands, and not creating bounds, there is strictly speaking no longer a requirement for the rooms to be convex, they can be concave shapes. This allows considerable flexibility in level design. HOWEVER, the portals should be placed such that no portal is in front of the portal plane of another portal in the room. I.e. the portals themselves should form some kind of convex structure. This is because for efficiency reasons, LPortal keeps track of the room that DOBs are within by monitoring them for crossing portal planes. If they cross a portal plane, by definition, they have entered the neighbouring room.

## Overview
The first thing you may notice is that LPortal doesn't make much use of special features within the editor, and simply relies on careful naming of spatials (or Empties within Blender). There is one new node type, 'LRoomManager', which behaves pretty much as any spatial however it has some added functions and runs an entire visibility system under the hood.

The reason for not creating bespoke editing within Godot is twofold:
1) This is a non-official plugin to the engine, without modifying core, or creating a specific importer
2) Using the node names to denote rooms and portals allows the whole level to be edited (and more importantly re-edited) within a modelling package such as blender.*

*_Note that although the whole game level can be created in the modelling package, this is optional, you can also create it in the Godot editor._

## Showing / Hiding Objects
An important consideration during development of LPortal was finding that in Godot 3.x, hiding nodes via the godot hide() function does NOT actually save as much performance as you would expect. On investigation I found that these nodes are still being processed in the octree in the visual server.

What does give the expected performance boost and prevent them being processed is _detaching them from the scene tree_ (but not deleting them). This is the default approach LPortal uses for culling nodes. This is not ideal because it has some wider ranging implications:

1) Objects that are detached from the scene tree will not have _process or _physics_process called. 
2) If an object is a child of a culled object (MeshInstance or Light), then it will also be removed from the scene tree when the object is culled:

For instance a collision shape child may mean the physics no longer works for the object when culled. To get around this it is suggested to either create collision shapes as siblings (rather than children) of MeshInstances, or place them in another part of the scene graph.

However, the ultimate choice of which method is up to you. You can choose either detaching objects to show / hide, or the more traditional godot show / hide approach (which will not affect physics etc). The command `rooms_set_hide_method_detach` can either be set to true (default) or false. This should be set before using `rooms_convert`.

You may also actively wish to deactivate some processing (e.g. AI), rather than just rendering in areas that are not visible. For this purpose you can query LPortal to find which rooms are visible.

# Components

### The room manager
Within your scene graph, to use LPortal you should add an LRoomManager node to the scene, e.g. as a child of the root node. You can reuse the same room manager as you load multiple game levels.

On another branch of the scene graph you should create a standard Spatial which will act as the root node for all the rooms and meshes, lights to be controlled by the LRoomManager. You can name this anything you like, but in the documentation I will refer to it as the 'roomlist'.

In order for the room manager to know which roomlist it should operate on, in the IDE you should _assign_ the roomlist to the 'rooms' property of the LRoomManager. You can do this by clicking on 'assign' in the LRoomManager inspector, and selecting the roomlist Spatial that you created.

The children of the roomlist should be rooms, or areas (which themselves contain rooms). The rooms contain the meshes, lights that you want to be shown and hidden by the room system.

### Rooms
Rooms are simply spatials, whose children should contain the mesh instances that are static (non-moving) within the room. The naming of the room spatial is crucial in order for it to be recognised. All rooms should start with 'room_'.
e.g.:

* room_kitchen
* room_hall
* room_bathroom1

You can use any character within the name (except '*', that is reserved, see the portal naming).

### Areas
Areas are used to define groups of rooms, currently this is only used to determine which area global directional lights (such as the sun) should apply to. For example if your game level has an outside area and indoor where the sun cannot reach, you may want to create two areas.

When using areas, you would place these between the roomlist and the rooms, like so:
```
roomlist
	area_outside
		room_outside1
		room_kitchen
	area_inside
		room_cellar
```
Note that a room can be a member of more than one area if desired. Simply make a child of the room with the desired area, and it will also be added to that area.
e.g.
```
roomlist
	area_outside
		room_outside1
		room_kitchen
			area_inside
	area_inside
		room_cellar
```

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

1) Do not supply a bound, in which case LPortal will auto generate a bound using qhull from the geometry
2) Manually supply the convex hull (bound) for a room
3) Use the dob_register_hint and teleport_hint versions, where the user specifies the start room

Approach (1) can often be used, but in the case where the geometry is not a good match, you can create a manual bound, either in your modelling package or within Godot IDE.

The bound should be stored as a MeshInstance child of the room, with a specific naming convention, it should start with the prefix 'bound_'. You can put anything after the prefix (or leave as is).

Although the bound will be treated as a convex hull at runtime, you don't have to be perfect when creating it because the geometry for the bound mesh instance will be run through the quickhull algorithm.

### Ignore objects
You can optionally prevent objects being added to LPortal internal room system (so they will not be culled to the planes). To do this, their name should begin with 'ignore_'. Note this is only relevant for objects derived from VisualInstance, LPortal ignores all non-visual instance objects. However note that these objects will still be culled when entire rooms are hidden by LPortal (_?is this still true?_).

## DOBs (Dynamic objects)
While most of the world in the room portal is assumed to be static (non-moving), you will inevitably want some objects to move around in the scene. These objects that can move _between_ rooms are special, the visibility system has to keep track of which room they are in in order to render them correctly.

Players, boxes, props, physics objects are all examples of candidates for DOBs, as well as the most crucial example, the camera itself. Unless the visibility system knows which room the camera is in, the system will not know what to render! We will use the camera as an example as you will need to register this as a DOB.

Somewhat counter-intuitively, all DOBs should be maintained in your game __OUTSIDE__ the room list subtree. I.e., __NOT__ in a room. This is because the relationship between the DOB and the room is a soft one, the DOB can move between rooms. This is also handy because it makes it easier to use things like pools for your dynamic objects, because you don't have to worry about them being in different rooms in different places in the scene graph.

All the following functions should be called on the LRoomManager node.

* Call `dob_register(cam, 0.0)` to register a DOB to be handled

The number 0.0 is the radius of the DOB in the visibility system. All DOBs are managed as spheres. For a camera the radius can be zero because it will never be visible (however it DOES require to be a DOB so that the system can keep track of which room it is in).

If a DOB is being culled (popping out of view) when it should not, it is normally because the bounding radius needs to be larger. On the other hand, too large a radius will make the DOB render when it is not necessary, so it is a good idea to tweak this value. It is in Godot world units, so you may be able to simply measure you object in the IDE. 

* Each frame, call `dob_update(cam)` to keep that DOB updated in the system

If the DOB is not moving, or you want to deactivate it to save processing, simply don't call update again until you want to reactivate it. There is no need to call dob_update on the selected camera, it will be updated automatically.

* When you have finished with a DOB you should call `dob_unregister(cam)` to remove the soft link from the system. This is more important when you are creating and deleting DOBs (say with multiple game levels). If you call rooms_release when unloading a level and want to keep DOBs in between levels, it is crucial that you do not update them until you have re-registered them after calling rooms_convert to create the new level (you will get an error message otherwise).

* If you are suddenly moving a DOB a large distance (rather than into a neighbouring room), you should call `dob_teleport(cam)`. This uses a different system to estimate the new room that the DOB is in. If you know in advance what room the dob will teleport to, there is a hint version of the function.

* DOBs should only move between rooms via the portals. In fact that is how their movement between rooms is defined. This is why a room's portals should form a convex space, never concave. In order to limit movement between rooms to the portals, you should use e.g. physics, or a navmesh.

## Conversion
Build your rooms and portals and placed them as children in a scene graph structure something like this:

```
Root
  LRoomManager
  MyRoomList
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
* Ensure that you have assigned the roomlist to the rooms property in the LRoomManager (either in the IDE or in script)

* At startup / when you load the level, call `rooms_convert()`

This will provide some output to indicate the building of the optimized internal visibility structure.

* Set which camera you want LPortal to use by calling `rooms_set_camera(cam)`

* Register the camera as a DOB (see above section on DOBs) and update each frame.

* If you want to unload a level and load a new one, call `rooms_release()`, free the old level, load the new one, and call `rooms_convert()` again (which clears the old data), and repeat each step.

_Be aware that if you unload (using queue_free) and load levels with the same name then Godot can rename them to avoid name conflicts. The nodepath to the rooms assigned to the LRoomManager must be correct!_

### Notes
* The most involved step is building your original rooms and portals, and getting the names right. Watch the debug output in the console when converting the rooms, it will let you know or give indications where it is having trouble converting.
* You can use hierarchies of nodes to place your static MeshInstances within a room, so it is easier to place groups e.g.:
```
room_office
  Desk (spatial)
    Table (mesh instance)
    Chair (mesh instance)
```

## Debugging
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

You can show the portal culling graphically by calling `rooms_set_debug_planes(true)`, and show the room bounds (if defined) by calling `rooms_set_debug_bounds(true)`.

#### Getting maximum performance
LPortal is very efficient, and the culling process itself is only likely to be a bottleneck if you are creating too high a density of portals.

Portal polygons with fewer edges are also faster to cull against. So usually an axis aligned quad will make a lot of sense, even when covering an irregular opening (say a cave).

Level design is a balancing act:

1) More objects cull more effectively
2) Fewer objects result in fewer draw calls (which can be a bottleneck)

# Lighting
#### Introduction
Although deciding what is in view of the camera is relatively straightforward, what complicates matters is that objects in view may be lit by lights that are not in view. Even worse, objects in view may be shadowed by objects that are NOT in view!

### Realtime lighting
Godot's realtime lighting works using shadow mapping, that is, before rendering the view from the camera, it first renders the view from each light affecting the scene, drawing each object to a _shadow map_. During the camera render it then uses the shadow map (or maps) to determine which pixels are in shadow.

It should be obvious that this is potentially very expensive, both the extra steps to render from each light, and the lookup at runtime to determine whether pixels are in shadow. As a result, realtime shadows are not a good option on some low powered devices.

If we cull our visible objects using LPortal but still render all objects when rendering the shadow maps from each light, we would be missing out on a lot of the benefits of our rooms / portal system. Due to this, I have made considerable effort to allow LPortal to cull not only the objects in view of the camera, but also to cull objects that are in view of the lights, and ONLY those lights that are relevant to the visible scene (a game level may contain 100 lights, but if only 2 are lighting the current view, those are the only 2 that are needed).

There are 3 light types in Godot :

1) Directional Lights (global)
2) Spotlights (local)
3) Omni lights (local)

#### Local Lights - Spotlights and Omni lights

Spotlights and Omnis are treated in a very similar manner within LPortal. You should place them within your rooms, in a similar manner to meshes. If these lights are static (non moving), that is all that needs to be done, and they should work automatically.

#### Dynamic Local Lights

Making these lights dynamic (movable) is possible too. Place them in a room as normal, but make sure to give them a unique name (e.g. 'kitchen_light'). From gdscript or similar you will want to retain a reference to the light after loading the level.

E.g.
```
var m_node_KitchenLight = get_node("/root").find_node("kitchen_light", true, false)
```
Then after converting the level, call
```
# where LRoomManager is your LRoomManager
$LRoomManager.dynamic_light_register(m_node_KitchenLight)
```
Move or rotate the light as you would normally, but to keep LPortal up to date, immediately after moving the light call 'dynamic_light_update':
```
m_node_KitchenLight.rotate_y(0.01) # just an example
$LRoomManager.dynamic_light_update(m_node_KitchenLight)
```

#### Global Directional lights

While spotlights and omnis can be placed within rooms, directional lights (such as the sun) are slightly different. Their position is not important (because they affect _everything_), only their direction is important. But this means they cannot be placed in any particular room, because they could affect every room. This potentially makes directional lights incredibly inefficient. You could have an entire cave system where sunlight does not reach, and yet the sun might draw shadows for all the objects, and render the shadows on every object (despite everything being in shadow from the sun).

LPortal helps solve this problem by allowing you to specify areas (groups of rooms) that a global light affects. To do this, you should start by placing your rooms inside areas within the roomlist (see the _areas_ section above).

As directional lights are not associated with any particular room, you should add them to the scene graph OUTSIDE the roomlist. By default godot will draw these lights affecting everything. In order to turn them on and off according to the location of the camera (i.e. is the camera inside the cave system?) we need to register the global light with LPortal.
```
# where $DirectionalLight is your light node,
# and "myarea" is the name of your area (e.g. area_myarea) that you want it to affect
$LRoomManager.light_register($DirectionalLight, "myarea")
```
That's all you need to do. If any of the visible rooms (as calculated by LPortal) are part of the area affected by the light, the light will be drawn. If not, it will not. LPortal also calculates only those shadow casters that are relevant, but directional lights cannot as yet (Godot 3) take advantage of this information. This may change in Godot 4.

### Baked Lighting
Often a far better fit for lighting with occlusion culling systems is the use of baked lighting, such as lightmaps. Lightmaps can store the entire static lighting for a level in precalculated form, as one or more textures, called lightmaps. This has pros and cons.

#### Pros
* Lightmaps are pre-calculated ahead of time, so can use far more complex and realistic models of lighting than would be possible in realtime. Techniques such as radiosity, and multiple bounces of light rays are possible.
* Lightmaps are very fast at runtime. There is no need to render a shadow map for each light, and when rendering from the camera, instead of an expensive lookup into shadow maps, simple texture mapping can be used to lookup the lightmap texture. As such they can produce fantastic lighting even on low powered machines such as mobile phones. This is the reason many of the first action 3d games used lightmaps, such as early quake and unreal.

#### Cons
* Lightmaps are limited in what kind of lighting information they store. Classical lightmaps only store the diffuse component of the lighting, and ignore the specular component. There are some more complex types of lightmaps but only the diffuse type is directly supported in Godot so far.
* Lightmaps only deal with static lighting. As the lighting is baked it gives a snapshot of lighting in one particular arrangement. If you want moving lights, or lights that change in brightness, you will need to either use other techniques or combine them with lightmapping. (One way of getting some simple variation is to bake more than one lightmap for a level, then blend the lightmaps on each frame prior to rendering, however this feature is not standard in Godot)
* Another big consequence of lightmaps being static is they don't show shadows for moving objects. This can be acceptable for the best performance on low end machines, but in practice, many games combine lightmaps with other techniques to get some kind of shadows on moving objects.

This may mean using lightmapping in combination with traditional realtime lighting, but for example, only rendering dynamic objects to the shadow maps. You can also render the realtime lighting as normal for all objects, but only put indirect lighting into the lightmaps. This gives some increase in visual quality compared to fully realtime lighting but does not help performance wise.

### Using Baked Lighting with LPortal
I have been preparing two workflows to simplify lightmapping a level with LPortal.

1) Internal workflow - Uses Godot to UV map the objects and the BakedLightmap node to create the lightmaps
2) External workflow - Allows UV mapping and rendering lightmaps in an external 3d modelling program such as Blender

The first workflow is fast to use but the end results are not as high quality as using an external program. There are also currently (as of 3.2 alpha) bugs in the Godot uvmapper and lightmapper, which can result in visual anomalies.

#### Internal workflow

It can be a good idea to examine the [BoxRooms demo](https://github.com/lawnjelly/lportal-demos/tree/master/BoxRooms), which shows the process in detail. Note that all this can be done manually, or with your own code (it is not even a core part of LPortal, more convenience functionality), the workflow is just a way of simplifying this for most people.

1) Create your level as normal with rooms and portals.
2) Add spotlights and omnis as desired within the rooms.
3) You can also use directional lights (outside the room list).
4) Make sure each light has the property set 'Use In Baked Lighting'.

5) The workflow allows having one shared lightmap for the entire level, instead of one per object. To do this as well as running the game as normal you must also have a 'preparation' phase where the level will be UV mapped (a second set of UV coordinates, which are used for the lightmap). The preparation phase can be a separate godot project if desired, the only output required for a game level is (1) the uv mapped level and (2) the lightmap.

6) For the preparation phase instead of calling `rooms_convert` you should call:
`MeshInstance * lightmap_internal(String szProxyFilename, String szLevelFilename);`

This will save 2 files, a .tscn containing a merged mesh (proxy) of the entire level, and a .tscn containing the entire level, but with a second set of UV coordinates matched to the proxy.

7) You now have to create the lightmap. Make sure there is a BakedLightmap node in the scene, and add the proxy .tscn to the scene using the 'link' button in the Godot IDE. Hide any other geometry aside from the proxy. Make sure the original level roomlist is in the scene, but hidden. The original level contains the lights that are needed for baking.

8) When all is set up, select the BakedLightmap node and click 'Bake'. If all is well it should bake a single lightmap, and save it in the folder specified in the BakedLightmap path.

9) The only 2 things of interest at the end of the preparation phase are the final level .tscn file, and the lightmap. You should copy these to your main project (if you are using 2 separate projects, one for the game and one for preparation / baking).

10) The only other thing necessary for your game to use the lightmap is to use a shader that blends the lightmap texture with the level textures. Here is a simple example:
```
shader_type spatial;
render_mode unshaded;
uniform sampler2D texture_albedo : hint_albedo;
uniform sampler2D texture_lightmap : hint_albedo;

void fragment() {
	vec4 albedo_tex = texture(texture_albedo,UV);
	vec4 lightmap_tex = texture(texture_lightmap,UV2);
	ALBEDO = albedo_tex.rgb * lightmap_tex.rgb * 2.0;
}
```

11) In your final running level, I currently would recommend that if you have a BakedLightmap node present, to hide it. The BakedLightmap node does allow dynamically changing the lightness of meshes as you move them around the level (as it stores a rough 3d array of voxel lighting levels) HOWEVER it may also apply this to the background meshes, which can result in considerable performance loss / visual anomalies. This recommendation may change if I find a way of getting the first effect without the second.

#### External workflow

1) Create your level as normal with rooms and portals.
2) For the preparation phase, instead of calling rooms_convert, call:
```
lightmap_external_export("my_level.dae")
```
Where 'my_level' is the filename/path you want to use. This will export all the rooms (and lights within the roomlist) as a Collada DAE file which can be loaded in a 3d modelling package such as blender. When imported, as well as the roomlist, there will be a merged mesh which contains all the individual meshes merged into one.

The next steps describe the process in blender but similar can be achieved in other packages.

3) The next step is to uv map the merged mesh prior to baking. Select the merged mesh, go into edit mode, select all the faces and choose to UV map as desired.
4) Before baking lightmaps, you should now export this merged mesh with the new UVs, as a wavefront 'obj' file (only include the selected mesh in the obj). This will be required later in Godot to 'unmerge' the new UVs to the secondary UVs on the objects within the room list, so that the lightmap is correctly mapped to the objects in Godot.
5) Now you can create the lightmap, by baking to a texture assigned to the merged mesh. Consult the blender documentation for information on baking, I will not describe this here. You may wish to bake both lights and ambient occlusion, and combine them in photoshop or similar. Save the lightmap as a PNG (you can convert the lightmap as JPG to use with Godot if desired, but it is wise to keep an original uncompressed version as PNG or similar).
6) Back to Godot, for the middle stage you should add a MeshInstance to your scene using the merged obj file you exported from blender (with UVs). We want to take these UVs and assign them as secondary UVs to the original objects making up the level. To do this we run again but use the command
```
lightmap_external_unmerge($Proxy, "final_level.tscn")
```
Where $Proxy is a MeshInstance containing the merged obj with UVs, and 'final_level' is the filename we wish to save our level after the UVs have been transferred.
7) If all goes well, the final_level.tscn can be loaded in game and can be used to show the lightmaps, using a shader similar to that used in the internal workflow.

Note that you should usually assign the shader to the materials while creating the level initially, and prior to creating the lightmap. You don't have to, but you may wish to re-export the lightmap several times to get it looking perfect in game, so the nearer the level is to 'finished' before going through this process, the easier it will be.

### Command reference
_(There is a full reference available from the help section in the IDE under 'LRoomManager')_
* `rooms_convert()` - prepare lportal for rendering
* `rooms_set_camera()` - set which camera visibility is calculated from
* `Node * rooms_get_room()` - returns godot room node from lportal room id
* `rooms_set_active()` - turns on and off lportal
* `rooms_log_frame()` - output debug logs for the next frame
* `rooms_set_logging()` - set debug logging level, 0 - 6 (0 is none, 6 is most)
* `rooms_set_debug_planes()` - turns on and off graphic debugging of portal planes
* `rooms_set_debug_bounds()` - turns on and off graphic debugging of room bounds

* `dob_register()` - have lportal find start room
* `dob_register_hint()` - user provides start room
* `dob_unregister()`
* `int dob_update()` - returns room ID within
* `dob_teleport()` - have lportal find start room
* `dob_teleport_hint()` - user provides start room
* `int dob_get_room_id()` - return room ID the dob is currently within

