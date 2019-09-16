# godot-lportal
Portal rendering module for Godot
Work in progress, not yet fully functional

Video of initial testing:
https://www.youtube.com/watch?v=xF_3Fe2HRdk

_Feel free to leave suggestions / feature requests on the issue tracker, especially regarding ease of use._

## Current status
The system is mostly working. I am now testing / polishing the interface and adding a few features. I will make a first release before implementing PVS as PVS is an optional feature.

## Roadmap
* Auto conversion of named room spatials and portal mesh instances to LRoom and LPortal DONE
* Auto creation of mirror portals DONE
* Recursive determine visibility DONE
* Prevent memory allocations (use pools for plane vectors) DONE
* Add support for objects moving between rooms - cameras, players, physics etc - DONE
* Refactor code, moving LRooms and LPortals outside scene graph DONE
* Cleanup code, Optimize DONE
* Handle special cases (multiple portals views into room etc) DONE
* Bug fixing / testing ONGOING
* Optimize non-moving statics DONE
* Closable portals
* PVS (primary and secondary)
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
* At game / level start, call 'rooms_convert' method on the LRoomManager. This will setup internal room and portal lists within the room manager prepared for fast rendering, and make portals 2 way (LRooms and LPortals are now handled internally within LRoomManager, and as such are separate from the scene graph after initial conversion.)
* Call 'rooms_set_camera()' on LRoomManager to set which camera is used for visibility determination (this is useful for debugging)

Dynamic objects that may move in between rooms (DOBs), like cameras, players, boxes etc are handled slightly differently. You should currently maintain them outside the roomlist, and instead of adding them to the rooms directly, you call:
* `void register_dob(Node * pDOB);`
to register with room system, which creates a soft reference, so that the DOB will be culled as part of the system.

I'm still in two minds whether to have the user manually call

`void update_dob(Node * pDOB);`

or have the system automatically manage DOBs. DOB updates check to see whether the DOB has crossed any of the portals out of the current room, as such could get expensive for large numbers of DOBs (hundreds maybe). As such, for efficiency, you usually will want the DOB updates to only take place if the DOB is within the primary or secondary PVS (potentially visible set) of rooms. Likewise you will probably only want to move around / update the AI of these localized DOBs.

For now this function is left to be called by the user but I might be able to make an auto mode for convenience when I implement PVS.
