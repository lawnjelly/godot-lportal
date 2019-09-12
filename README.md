# godot-lportal
Portal rendering module for Godot

Work in progress, not yet fully functional

## Roadmap
* Auto conversion of named room spatials and portal mesh instances to LRoom and LPortal DONE
* Auto creation of mirror portals DONE
* Recursive determine visibility DONE
  * (basic function works, culls backward pointing portals, tomorrow add clipping planes formed by portal edge and camera)
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
* At game / level start, call 'convert' method on the LRoomManager. This will convert room and portal spatials to LRooms and LPortals, and make portals 2 way.
* Call 'set_camera()' on LRoomManager to set which camera is used for visibility determination (this is useful for debugging)

That's pretty much it for now. There will also be calls to help determine which room the camera starts in, it just defaults to the first room for now. I'll try and figure a way of automatically tracking the camera room.
