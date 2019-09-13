# godot-lportal
Portal rendering module for Godot

Work in progress, not yet fully functional

## Roadmap
* Auto conversion of named room spatials and portal mesh instances to LRoom and LPortal DONE
* Auto creation of mirror portals DONE
* Recursive determine visibility DONE
* Prevent memory allocations (use pools for plane vectors) DONE
* Add support for objects moving between rooms - cameras, players, physics etc - DONE
* Cleanup code, Optimize
* Investigate multiple passes (shadows, lights)

## Instructions
This is all subject to change, but to give a rough idea of the current process:

* Make a list of spatials for each room under a parent spatial (which will become the room manager)
* Name the rooms 'room_kitchen', where kitchen is your room name
* Place your objects within the room as children of the room
* Also place portals inside the rooms
* Portals should be MeshInstances, and convex polygons (e.g. you can create a plane and move it into position for a door)
* Portals should be named 'portal_hall', where hall is the name of the room the portal should link to
* Portals only need to be made in one of the adjoining rooms, the system will automatically create the mirror image portal

Once this structure is set up in the scene graph:
* Convert the parent of the rooms to an LRoomManager node
* At game / level start, call 'rooms_convert' method on the LRoomManager. This will convert room and portal spatials to LRooms and LPortals, and make portals 2 way.
* Call 'rooms_set_camera()' on LRoomManager to set which camera is used for visibility determination (this is useful for debugging)

Dynamic objects (DOBs) like players, boxes etc are handled slightly differently. You currently can maintain them outside the roomlist, but instead of adding them to the rooms directly, you call:
* `void register_dob(Node * pDOB);`
to register with room system, so the DOB will be culled as part of the system.
* `bool update_dob(Node * pDOB);`
each frame you move the DOB to keep it up to date within the system. This will automatically move the DOB between rooms when it crosses portals.

I may also add the option of having the system automatically update the dob, instead of having to call update manually. I only didn't do this initially because you might want dynamic objects to go to sleep when they aren't moving, so there is no need to update in the system.

