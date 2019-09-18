# LPortal Tutorial

1) Create a new Godot project, and add a spatial, this will be the root of your scene graph. Name it 'Root'.
2) Add an LRoomManager node as a child of the Root, and call it 'RoomList'. This will be where you place your rooms.
3) Create a spatial under the RoomList, and name it 'room_kitchen'.
4) As children of room_kitchen, create simple MeshInstance boxes for a floor and 4 walls, but make one of the walls out of 2 boxes, leaving a hole for a doorway.
5) Duplicate the entire room, and change the name to 'room_lounge'.
6) Change the location and rotation of the lounge so that the two rooms join, and the doorways match up.
7) Now we will add a portal to see between the rooms. There is no need to add it to both. Create a MeshInstance inside room_kitchen and name it 'portal_lounge'. This tells the system that you are making a portal that will see from the kitchen to the lounge.
8) The MeshInstance for portal_lounge should be a plane, and you should rotate and translate and scale it into position so it covers the doorway, facing into the lounge.
9) At this point the room scene is complete, and we will create some of the usual objects needed in a game.
10) Create a camera as a child of the Root, and place it within the kitchen, pointing towards the lounge through the door.
11) Create a script for the Root node, call it Root.gd.



