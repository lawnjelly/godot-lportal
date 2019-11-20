# LPortal SingleRoom mode

### About
Although LPortal is primarily designed for occlusion culling with rooms and portals, it also contains some optimizations to increase the accuracy of general culling (both for the camera and lights). While I couldn't easily integrate these into Godot 3.2 core (because of shadow map caching), they can still be used relatively simply with most 3d games via LPortal.

### How does it work?
Single room mode uses a subset of the functionality of LPortal to speed up culling in a general situation. It works by treating your entire game level as a single room, without portals (so there will be no occlusion culling). However the meshes / lights within will be culled by LPortal.

The optimizations speed up and increase the accuracy of general camera / light culling using several methods, especially the following:

* Shadow casters are culled so that only those that affect the camera frustum are drawn to shadow maps
* Entire lights are culled if they do not affect the camera frustum
* Objects are culled / shown by detaching / attaching to the scene graph 

Note that these methods will primarily speed up rendering in games that use realtime lighting, rather than baked lighting. They aren't designed to work with GI probe and may produce artefacts (such as shadow popping) when used with GI probe.

### How to
1) You will typically need to separate the objects in your game into those that are long-lived (staying present throughout a game level) and those that may be added and removed dynamically (such as players).\
\
_I will refer to the long-lived objects as 'static', and the short-lived as 'dynamic', just to fit the conventions of the rest of LPortal. However, in single room mode all objects can move around, so these names should be taken to refer to the lifetimes._
2) Make sure all the static objects are placed as children / grandchildren of a single Spatial (named anything you like), this will represent your single room.
3) Add an LRoomManager node to the scene graph, but not within the room. This node contains all the functionality of LPortal.
4) You now need to 'link' the LRoomManager to your room. Under the inspector for the LRoomManager, click to assign the roomlist and select your 'room' spatial.
5) When you load your game level at runtime (especially the room), you need to convert it to work with LPortal. To do this you should call
```
LRoomManager.rooms_single_room_convert(true, false)
```
Note that if your game has several levels, before unloading a level you should call
```
LRoomManager.rooms_release()
```

6) There is one further step to get started, you must setup your camera to work with LPortal. The camera should NOT be in the room.
7) First you have to register the camera as a dynamic object (DOB) within LPortal:
```
# where $Camera is your camera node
LRoomManager.dob_register($Camera, 0)
```
_Note that there is no need to call dob_update, as the camera cannot move between rooms in single room mode._

8) Next you have to select this camera as the active camera:
```
LRoomManager.rooms_set_camera($Camera)
```
9) This should be enough to get started, try running the game and see if everything is working.

# Lighting
There are some extra considerations for lighting, especially moving lights. Please see the main instructions for more detail.

### Omni and Spot Lights
Omnis and Spot lights should be placed in the room. If they are non-moving, that is all that is required. If they are moving lights there is an extra step required for them to cull correctly:

1) After converting the level, you should call:
```
# where node_my_light is your light node (use e.g. the Godot find_node function)
$LRoomManager.dynamic_light_register(node_my_light)
```
2) Upon moving or rotating the light you should call:
```
$LRoomManager.dynamic_light_update(node_my_light)
```
### Directional Lights
Directional lights are not associated with a room, so should be added to the scene graph OUTSIDE the room. To tell LPortal to use them in culling you should call:
```
$LRoomManager.light_register($DirectionalLight, "default")
```
Where $DirectionalLight is your light node. The string "default" is the name of the area this light should affect - in single room mode everything is automatically assigned to one area, named "default".

This is just the basics of lighting, for more information see the main Instructions.
