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


class LRoom;

// simple min max aabb
class LAABB
{
public:
	Vector3 m_ptMins;
	Vector3 m_ptMaxs;
	void SetToMaxOpposite()
	{
		float ma = FLT_MAX;
		float mi = FLT_MIN;
		m_ptMins = Vector3(ma, ma, ma);
		m_ptMaxs = Vector3(mi, mi, mi);
	}
	void ExpandToEnclose(const AABB &bb)
	{
		if 	(bb.position.x < m_ptMins.x) m_ptMins.x = bb.position.x;
		if 	(bb.position.y < m_ptMins.y) m_ptMins.y = bb.position.y;
		if 	(bb.position.z < m_ptMins.z) m_ptMins.z = bb.position.z;
		if 	(bb.position.x + bb.size.x > m_ptMaxs.x) m_ptMaxs.x = bb.position.x + bb.size.x;
		if 	(bb.position.y + bb.size.y > m_ptMaxs.y) m_ptMaxs.y = bb.position.y + bb.size.y;
		if 	(bb.position.z + bb.size.z > m_ptMaxs.z) m_ptMaxs.z = bb.position.z + bb.size.z;
	}
	Vector3 FindCentre() const
	{
		Vector3 pt;
		pt.x = (m_ptMaxs.x - m_ptMins.x) * 0.5f;
		pt.y = (m_ptMaxs.y - m_ptMins.y) * 0.5f;
		pt.z = (m_ptMaxs.z - m_ptMins.z) * 0.5f;
		pt += m_ptMins;
		return pt;
	}
};


class LRoomManager : public Spatial {
	GDCLASS(LRoomManager, Spatial);

	friend class LRoom;

	// a quick list of object IDs of child rooms
	Vector<ObjectID> m_room_IDs;

//	ObjectID m_room_curr;
	ObjectID m_cameraID;

	// keep track of which rooms are visible, so we can hide ones that aren't hit that were previously on
	Lawn::LBitField_Dynamic m_BF_visible_rooms;
	Vector<int> m_VisibleRoomList[2];
	int m_CurrentVisibleRoomList;

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

	// updating dynamic objects in case they move out of their current room
	void register_dob(Node * pDOB);
	void unregister_dob(Node * pDOB);
	bool update_dob(Node * pDOB);
	bool teleport_dob(Node * pDOB);


protected:
	static void _bind_methods();
	void _notification(int p_what);

	LPlanesPool m_Pool;
private:
	// one time conversion and setup
	void Convert_Rooms();
	bool Convert_Room(Spatial * pNode);
	void Convert_Portals();
	void Find_Rooms();

	// helper funcs
	LRoom * GetRoomNum(int i) const;
	LRoom * GetRoomFromDOB(Node * pNode) const;
	int GetRoomNumFromLRoom(LRoom * pRoom) const;
	int FindClosestRoom(const Vector3 &pt) const;

	int Obj_GetRoomNum(Node * pNode) const;
	void Obj_SetRoomNum(Node * pNode, int num);


	void FrameUpdate();
};

#endif
