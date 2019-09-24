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

/* room.h */
#ifndef LROOM_H
#define LROOM_H

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/


#include "scene/3d/spatial.h"
#include "lvector.h"
#include "ldob.h"
#include "lbound.h"


namespace Lawn {class LBitField_Dynamic;}

class LPortal;
class LRoomManager;
class MeshInstance;

class LCamera
{
public:
	// all in world space, culling done in world space
	Vector3 m_ptPos;
	Vector3 m_ptDir;
};



class LRoom
{
private:

public:
	static const int SOFT_SHOW_BIT = 18;
	static const int SOFT_HIDE_BIT = 19;

	// static objects
	LVector<LSob> m_SOBs;

	// dynamic objects
	LVector<LDob> m_DOBs;

	// portals are stored in the manager in a contiguous list
	int m_iFirstPortal;
	int m_iNumPortals;

	// Just very rough, room centre for determining start rooms of dobs
	Vector3 m_ptCentre;
	AABB m_AABB; // world bound

	// ID in the Room Manager, NOT the godot object ID
	int m_RoomID;

	ObjectID m_GodotID;

	// frame counter when last touched .. prevents handling rooms multiple times
	unsigned int m_uiFrameTouched;

	// optional bounding convex hull, for accurate detection of which room to start in
	// when registering DOBs and teleporting them
	LBound m_Bound;

	String m_szName;
	////////////////////////////////////////////////////////////

	const String &get_name() const {return m_szName;}

	// main function
	void DetermineVisibility_Recursive(LRoomManager &manager, int depth, const LCamera &cam, const LVector<Plane> &planes, int portalID_from = -1);
	void FirstTouch(LRoomManager &manager);


	// allows us to show / hide all dobs as the room visibility changes
	void Room_MakeVisible(bool bVisible);

	// hide godot room and all linked dobs
	// USED AT RUNTIME
//	void Hide_All();

	// show godot room and all linked dobs and all sobs
	void Debug_ShowAll();

	// hide all the objects not hit on this frame .. instead of calling godot hide without need
	// (it might be expensive)
	void FinalizeVisibility(LRoomManager &manager);


	void DOB_Add(const LDob &dob);
	const LDob &DOB_Get(unsigned int ui) const {return m_DOBs[ui];}
	unsigned int DOB_Find(Node * pDOB) const;
	bool DOB_Remove(unsigned int ui);
	LRoom * DOB_Update(LRoomManager &manager, Spatial * pDOB);

	LRoom();
	Spatial * GetGodotRoom() const;

	// retained purely for debugging visualization
	Geometry::MeshData m_Bound_MeshData;

	bool IsVisible() const {return m_bVisible;}
private:
	// instead of directly showing and hiding objects we now set their layer,
	// and the camera will hide them with a cull mask. This is so that
	// objects can still be rendered outside immediate view for casting shadows.
	void SoftShow(VisualInstance * pVI, bool bShow) const;

	// whether lportal thinks this room is currently visible
	// this allows us to show / hide dobs as they cross room boundaries
	bool m_bVisible;

};


#endif
