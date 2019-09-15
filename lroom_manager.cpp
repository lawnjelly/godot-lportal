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

#include "lroom_manager.h"
#include "core/engine.h"
#include "scene/3d/camera.h"
#include "scene/3d/mesh_instance.h"
#include "lroom_converter.h"

LRoomManager::LRoomManager()
{
	m_cameraID = 0;
	m_uiFrameCounter = 0;

	// to know which rooms to hide we keep track of which were shown this, and the previous frame
	m_pCurr_VisibleRoomList = &m_VisibleRoomList[0];
	m_pPrev_VisibleRoomList = &m_VisibleRoomList[1];
}

int LRoomManager::FindClosestRoom(const Vector3 &pt) const
{
	//print_line("FindClosestRoom");
	int closest = -1;
	float closest_dist = FLT_MAX;

	for (int n=0; n<m_Rooms.size(); n++)
	{
		const LRoom &lroom = m_Rooms[n];

		float d = pt.distance_squared_to(lroom.m_ptCentre);

		if (d < closest_dist)
		{
			closest = n;
			closest_dist = d;
		}
	}

	return closest;
}


const LRoom * LRoomManager::GetRoom(int i) const
{
	if ((unsigned int) i >= m_Rooms.size())
	{
		WARN_PRINT_ONCE("LRoomManager::GetRoom out of range");
		return 0;
	}
	return &m_Rooms[i];
}

LRoom * LRoomManager::GetRoom(int i)
{
	if ((unsigned int) i >= m_Rooms.size())
	{
		WARN_PRINT_ONCE("LRoomManager::GetRoom out of range");
		return 0;
	}
	return &m_Rooms[i];
}


LRoom &LRoomManager::Portal_GetLinkedRoom(const LPortal &port)
{
	return m_Rooms[port.m_iRoomNum];
}


void LRoomManager::Obj_SetRoomNum(Node * pNode, int num)
{
	pNode->set_meta("_lroom", num);

	assert (Obj_GetRoomNum(pNode) == num);
}

int LRoomManager::Obj_GetRoomNum(Node * pNode) const
{
	//assert (pNode->has_meta("_lroom"));
	Variant v = pNode->get_meta("_lroom");
	if (v.get_type() == Variant::NIL)
		return -1;

	return v;
}

LRoom * LRoomManager::GetRoomFromDOB(Node * pNode)
{
	int iRoom = Obj_GetRoomNum(pNode);
	if (iRoom == -1)
	{
		WARN_PRINT_ONCE("LRoomManager::GetRoomFromDOB : metadata is empty");
		return 0;
	}

	LRoom * pRoom = GetRoom(iRoom);
	if (pRoom == 0)
	{
		WARN_PRINT_ONCE("LRoomManager::GetRoomFromDOB : pRoom is NULL");
	}
	return pRoom;
}


void LRoomManager::dob_register(Node * pDOB)
{
	print_line("register_dob " + pDOB->get_name());

	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
		return;

	Vector3 pt = pSpat->get_global_transform().origin;

	int iRoomNum = FindClosestRoom(pt);
	print_line("register_dob closest room " + itos(iRoomNum));

	if (iRoomNum == -1)
		return;

	LRoom * pRoom = GetRoom(iRoomNum);
	if (!pRoom)
		return;

	pRoom->DOB_Add(pSpat);

	// save the room ID on the dob metadata
	Obj_SetRoomNum(pSpat, iRoomNum);
}


bool LRoomManager::dob_update(Node * pDOB)
{
	// find the room the object is attached to
	LRoom * pRoom = GetRoomFromDOB(pDOB);
	if (!pRoom)
		return false;

	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
		return false;

	LRoom * pNewRoom = pRoom->DOB_Update(*this, pSpat);

	if (pNewRoom)
	{
		// remove from the list in old room and add to list in new room, and change the metadata
		int iRoomNum = pNewRoom->m_RoomID;

		pRoom->DOB_Remove(pDOB);
		pNewRoom->DOB_Add(pSpat);

		// save the room ID on the dob metadata
		Obj_SetRoomNum(pSpat, iRoomNum);

		return true;
	}

	return false;
}

bool LRoomManager::dob_teleport(Node * pDOB)
{
	return true;
}



void LRoomManager::dob_unregister(Node * pDOB)
{
	LRoom * pRoom = GetRoomFromDOB(pDOB);
	pRoom->DOB_Remove(pDOB);
}

int LRoomManager::dob_get_room_id(Node * pDOB)
{
	return Obj_GetRoomNum(pDOB);
}

Node * LRoomManager::rooms_get_room(int room_id)
{
	const LRoom * pRoom = GetRoom(room_id);

	if (!pRoom)
		return NULL;

	return pRoom->GetGodotRoom();
}



void LRoomManager::rooms_set_camera(Node * pCam)
{
	m_cameraID = 0;

	if (!pCam)
		return;

	Camera * pCamera = Object::cast_to<Camera>(pCam);
	if (!pCamera)
	{
		WARN_PRINT("Not a camera");
		return;
	}

	m_cameraID = pCam->get_instance_id();

	// use this temporarily to force debug
	LPortal::m_bRunning = false;
}

// convert empties and meshes to rooms and portals
void LRoomManager::rooms_convert()
{
	LRoomConverter conv;
	conv.Convert(*this);
}


void LRoomManager::FrameUpdate()
{
	if (Engine::get_singleton()->is_editor_hint())
	{
		WARN_PRINT_ONCE("LRoomManager::FrameUpdate should not be called in editor");
		return;
	}

	// we keep a frame counter to prevent visiting things multiple times on the same frame in recursive functions
	m_uiFrameCounter++;

	// clear the visible room list to write to each frame
	m_pCurr_VisibleRoomList->clear();

	// get the camera desired and make into lcamera
	Camera * pCamera = 0;
	if (m_cameraID)
	{
		Object *pObj = ObjectDB::get_instance(m_cameraID);
		pCamera = Object::cast_to<Camera>(pObj);
	}
	else
		// camera not set .. do nothing
		return;

	// camera not a camera?? shouldn't happen but we'll check
	if (!pCamera)
		return;

	// Which room is the camera currently in?
	LRoom * pRoom = GetRoomFromDOB(pCamera);

	if (!pRoom)
	{
		WARN_PRINT_ONCE("LRoomManager::FrameUpdate : Camera is not in an LRoom");
		return;
	}

	// as we hit visible rooms we will mark them in a bitset, so we can hide any rooms
	// that are showing that haven't been hit this frame
	m_BF_visible_rooms.Blank();

	// lcamera contains the info needed for culling
	LCamera cam;
	cam.m_ptPos = Vector3(0, 0, 0);
	cam.m_ptDir = Vector3 (-1, 0, 0);

	// reset the planes pool for another frame
	m_Pool.Reset();

	// the first set of planes are allocated and filled with the view frustum planes
	// Note that the visual server doesn't actually need to do view frustum culling as a result...
	// (but is still doing it for now)
	unsigned int pool_member = m_Pool.Request();
	assert (pool_member != -1);

	LVector<Plane> &planes = m_Pool.Get(pool_member);
	planes.clear();

	// get the camera desired and make into lcamera
	assert (pCamera);
	Transform tr = pCamera->get_global_transform();
	cam.m_ptPos = tr.origin;
	cam.m_ptDir = tr.basis.get_row(2); // or possibly get_axis .. z is what we want

	// luckily godot already has a function to return a list of the camera clipping planes
	planes.copy_from(pCamera->get_frustum());

	// the whole visibility algorithm is recursive, spreading out from the camera room,
	// rendering through any portals in view into other rooms, etc etc
	pRoom->DetermineVisibility_Recursive(*this, 0, cam, planes);

	// finally hide all the rooms that are currently visible but not in the visible bitfield as having been hit

	// to get started
	if (!m_pPrev_VisibleRoomList->size())
	{
		// NOTE this will be done more efficiently, but is okay to start with
		for (int n=0; n<m_Rooms.size(); n++)
		{
			if (!m_BF_visible_rooms.GetBit(n))
			{
				m_Rooms[n].GetGodotRoom()->hide();
			}
		}
	}
	else
	{
		// hide all rooms that were visible last frame but aren't visible this frame
		for (int n=0; n<m_pPrev_VisibleRoomList->size(); n++)
		{
			int r = (*m_pPrev_VisibleRoomList)[n];

			if (!m_BF_visible_rooms.GetBit(r))
				m_Rooms[r].GetGodotRoom()->hide();
		}

		// swap the current and previous visible room list
		LVector<int> * pTemp = m_pCurr_VisibleRoomList;
		m_pCurr_VisibleRoomList = m_pPrev_VisibleRoomList;
		m_pPrev_VisibleRoomList = pTemp;
	}

	// when running, emit less debugging output so as not to choke the IDE
	LPortal::m_bRunning = true;
}


void LRoomManager::_notification(int p_what) {

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
			// turn on process, unless we are in the editor
			if (!Engine::get_singleton()->is_editor_hint())
				set_process_internal(true);
			else
				set_process_internal(false);
		} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
			FrameUpdate();
		} break;
	}
}


void LRoomManager::_bind_methods()
{
	// main functions
	ClassDB::bind_method(D_METHOD("rooms_convert"), &LRoomManager::rooms_convert);
	ClassDB::bind_method(D_METHOD("rooms_set_camera"), &LRoomManager::rooms_set_camera);
	ClassDB::bind_method(D_METHOD("rooms_get_room"), &LRoomManager::rooms_get_room);

	// functions to add dynamic objects to the culling system
	// Note that these should not be placed directly in rooms, the system will 'soft link' to them
	// so they can be held, e.g. in pools elsewhere in the scene graph
	ClassDB::bind_method(D_METHOD("dob_register"), &LRoomManager::dob_register);
	ClassDB::bind_method(D_METHOD("dob_unregister"), &LRoomManager::dob_unregister);
	ClassDB::bind_method(D_METHOD("dob_update"), &LRoomManager::dob_update);
	ClassDB::bind_method(D_METHOD("dob_teleport"), &LRoomManager::dob_teleport);

	ClassDB::bind_method(D_METHOD("dob_get_room_id"), &LRoomManager::dob_get_room_id);
}
