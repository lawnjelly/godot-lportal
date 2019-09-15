//	Copyright (c) 2019 Lawnjelly

//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:

//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.

//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.

/* room_manager.h */
#ifndef LROOM_MANAGER_H
#define LROOM_MANAGER_H

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/

#include "scene/3d/spatial.h"
#include "lbitfield_dynamic.h"
#include "lplanes_pool.h"

#include "lroom.h"
#include "lportal.h"




class LRoomManager : public Spatial {
	GDCLASS(LRoomManager, Spatial);

	friend class LRoom;
	friend class LRoomConverter;


	// godot ID of the camera (which should be registered as a DOB to allow moving between rooms)
	ObjectID m_cameraID;

	// keep track of which rooms are visible, so we can hide ones that aren't hit that were previously on
	Lawn::LBitField_Dynamic m_BF_visible_rooms;
//	Vector<int> m_VisibleRoomList[2];
//	int m_CurrentVisibleRoomList;

	// keep a frame counter, to mark when objects have been hit by the visiblity algorithm
	// already to prevent multiple hits on rooms and objects
	unsigned int m_uiFrameCounter;

public:
	LRoomManager();

	// convert empties and meshes to rooms and portals
	void convert();

	// choose which camera you want to use to determine visibility.
	// normally this will be your main camera, but you can choose another for debugging
	void set_camera(Node * pCam);

	// Dynamic objects .. cameras, players, boxes etc
	// These are defined by their ability to move from room to room.
	// You can still move static objects within the same room (e.g. elevators, moving platforms)
	// as these don't require checks for changing rooms.
	void register_dob(Node * pDOB);
	void unregister_dob(Node * pDOB);
	bool update_dob(Node * pDOB);
	bool teleport_dob(Node * pDOB);


protected:
	static void _bind_methods();
	void _notification(int p_what);

	// The recursive visibility function needs to allocate loads of planes.
	// We use a pool for this instead of allocating on the fly.
	LPlanesPool m_Pool;
private:

	// helper funcs
	const LRoom * GetRoom(int i) const;
	LRoom * GetRoom(int i);

	LRoom * GetRoomFromDOB(Node * pNode);
	int FindClosestRoom(const Vector3 &pt) const;

	// for DOBs, we need some way of storing the room ID on them, so we use metadata (currently)
	// this is pretty gross but hey ho
	int Obj_GetRoomNum(Node * pNode) const;
	void Obj_SetRoomNum(Node * pNode, int num);

	// this is where we do all the culling
	void FrameUpdate();

	// find which room is linked by a portal
	LRoom &Portal_GetLinkedRoom(const LPortal &port);

	// lists of rooms and portals, contiguous list so cache friendly
	LVector<LRoom> m_Rooms;
	LVector<LPortal> m_Portals;
};

#endif
