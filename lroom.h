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



class LRoom
{
private:

public:
	// using flags we can determine which the object is visible to - the camera, or the lights (i.e. shadow caster), or both
	static const int LAYER_LIGHT_BIT = 18;
	static const int LAYER_CAMERA_BIT = 19;

	static const int LAYER_MASK_LIGHT = 1 << LAYER_LIGHT_BIT;
	static const int LAYER_MASK_CAMERA = 1 << LAYER_CAMERA_BIT;

	// static objects are stored in the manager in a contiguous list
	int m_iFirstSOB;
	int m_iNumSOBs;

	// dynamic objects
	//LVector<uint32_t> m_DOB_ids;

	// local lights affecting this room
	LVector<int> m_LocalLights;

	// global lights affecting this room
	LVector<int> m_GlobalLights;

	// areas this room is in
	LVector<int> m_Areas;

	// portals are stored in the manager in a contiguous list
	int m_iFirstPortal;
	int m_iNumPortals;

	int m_iFirstShadowCaster_SOB;
	int m_iNumShadowCasters_SOB;

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
//	void DetermineVisibility_Recursive(LRoomManager &manager, int depth, const LSource &cam, const LVector<Plane> &planes, int first_portal_plane = 1);
//	void FirstTouch(LRoomManager &manager);


	// allows us to show / hide all dobs as the room visibility changes
	void Room_MakeVisible(bool bVisible);

	// show godot room and all linked dobs and all sobs
	void Debug_ShowAll(bool bActive);

	// hide all the objects not hit on this frame .. instead of calling godot hide without need
	// (it might be expensive)
	void FinalizeVisibility(LRoomManager &manager);

	// naive version, adds all the non visible objects in visible rooms as shadow casters
	void AddShadowCasters(LRoomManager &manager);

//	void DOB_Add(int id);
//	int DOB_GetID(unsigned int ui) const {return m_DOB_ids[ui];}
//	unsigned int DOB_Find(int id) const;
//	bool DOB_Remove(unsigned int ui);
//	LRoom * DOB_Update(LRoomManager &manager, Spatial * pDOB);

	LRoom();
	Spatial * GetGodotRoom() const;

	// light casting .. changing the local light list
	bool RemoveLocalLight(int light_id);
	void AddLocalLight(int light_id) {m_LocalLights.push_back(light_id);}

	// retained purely for debugging visualization
	Geometry::MeshData m_Bound_MeshData;

	bool IsVisible() const {return m_bVisible;}
	// instead of directly showing and hiding objects we now set their layer,
	// and the camera will hide them with a cull mask. This is so that
	// objects can still be rendered outside immediate view for casting shadows.
	static void SoftShow(VisualInstance * pVI, uint32_t show_flags);
	bool IsInArea(int area) const;

private:
	// whether lportal thinks this room is currently visible
	// this allows us to show / hide dobs as they cross room boundaries
	bool m_bVisible;

};


#endif
