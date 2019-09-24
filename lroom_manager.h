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
	ObjectID m_ID_camera;

	// keep track of which rooms are visible, so we can hide ones that aren't hit that were previously on
	Lawn::LBitField_Dynamic m_BF_visible_rooms;

	LVector<int> m_VisibleRoomList_A;
	LVector<int> m_VisibleRoomList_B;

	LVector<int> * m_pCurr_VisibleRoomList;
	LVector<int> * m_pPrev_VisibleRoomList;

	// keep a frame counter, to mark when objects have been hit by the visiblity algorithm
	// already to prevent multiple hits on rooms and objects
	unsigned int m_uiFrameCounter;

	// for debugging, can turn LPortal on and off
	bool m_bActive;

	// for debugging, can emulate view frustum culling
	bool m_bFrustumOnly;

public:
	LRoomManager();

	// convert empties and meshes to rooms and portals
	void rooms_convert();

	// choose which camera you want to use to determine visibility.
	// normally this will be your main camera, but you can choose another for debugging
	void rooms_set_camera(Node * pCam);

	// get the Godot room that is associated with an LPortal room
	// (can be used to find the name etc of a room ID returned by dob_update)
	Node * rooms_get_room(int room_id);

	// turn on and off culling for debugging
	void rooms_set_active(bool bActive);
	void rooms_set_debug_planes(bool bActive);
	void rooms_set_debug_bounds(bool bActive);

	// 0 to 6 .. defaults to 4 which is (2) in our priorities (i.e. 6 - level)
	void rooms_set_logging(int level);

	// provide debugging output on the next frame
	void rooms_log_frame();

	// Dynamic objects .. cameras, players, boxes etc
	// These are defined by their ability to move from room to room.
	// You can still move static objects within the same room (e.g. elevators, moving platforms)
	// as these don't require checks for changing rooms.
	bool dob_register(Node * pDOB, float radius);
	// register but let LPortal know which room the dob should start in
	bool dob_register_hint(Node * pDOB, float radius, Node * pRoom);

	bool dob_unregister(Node * pDOB);

	// returns the room ID within
	int dob_update(Node * pDOB);

	// if we are moving the DOB possibly through multiple rooms, then teleport rather than detect
	// portal crossings
	bool dob_teleport(Node * pDOB);
	bool dob_teleport_hint(Node * pDOB, Node * pRoom);


	// helper func, not needed usually as dob_update returns the room
	int dob_get_room_id(Node * pDOB);

protected:
	static void _bind_methods();
	void _notification(int p_what);

	// The recursive visibility function needs to allocate loads of planes.
	// We use a pool for this instead of allocating on the fly.
	LPlanesPool m_Pool;

	// 0 to 5
	int m_iLoggingLevel;
private:
	// internal
	bool DobRegister(Spatial * pDOB, float radius, int iRoom);
	ObjectID DobRegister_FindVIRecursive(Node * pNode) const;
	bool DobTeleport(Spatial * pDOB, int iNewRoomID);
	void CreateDebug();
	void DobChangeVisibility(Spatial * pDOB, const LRoom * pOld, const LRoom * pNew);


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

	// debugging emulate view frustum
	void FrameUpdate_FrustumOnly();

	// draw planes and room hulls
	void FrameUpdate_DrawDebug(const LCamera &cam, const LRoom &lroom);

	// find which room is linked by a portal
	LRoom &Portal_GetLinkedRoom(const LPortal &port);

	// lists of rooms and portals, contiguous list so cache friendly
	LVector<LRoom> m_Rooms;
	LVector<LPortal> m_Portals;

public:
	// whether debug planes is switched on
	bool m_bDebugPlanes;
	bool m_bDebugBounds;

	// the planes are shown as a list of lines from the camera to the portal verts
	LVector<Vector3> m_DebugPlanes;
private:
	ObjectID m_ID_DebugPlanes;
	ObjectID m_ID_DebugBounds;
	Ref<SpatialMaterial> m_mat_Debug_Planes;
	Ref<SpatialMaterial> m_mat_Debug_Bounds;
};

#endif
