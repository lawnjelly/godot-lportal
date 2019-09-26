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
#include "ldebug.h"
#include "scene/3d/immediate_geometry.h"
#include "scene/3d/light.h"
#include "lroom.h"

LRoomManager::LRoomManager()
{
	m_ID_camera = 0;
	m_ID_DebugPlanes = 0;
	m_uiFrameCounter = 0;
	m_iLoggingLevel = 2;
	m_bActive = true;
	m_bFrustumOnly = false;

	// to know which rooms to hide we keep track of which were shown this, and the previous frame
	m_pCurr_VisibleRoomList = &m_VisibleRoomList_A;
	m_pPrev_VisibleRoomList = &m_VisibleRoomList_B;

	m_bDebugPlanes = false;
	m_bDebugBounds = false;
}

int LRoomManager::FindClosestRoom(const Vector3 &pt) const
{

	//print_line("FindClosestRoom");
	int closest = -1;
	float closest_dist = FLT_MAX;

	// uses bounds if this is available
	int closest_within = -1;
	float within_dist = FLT_MAX;

	for (int n=0; n<m_Rooms.size(); n++)
	{
		const LRoom &lroom = m_Rooms[n];

		float d = pt.distance_squared_to(lroom.m_ptCentre);

		if (d < closest_dist)
		{
			closest = n;
			closest_dist = d;
		}

		// is there a bound?
		if (lroom.m_Bound.IsActive())
		{
			// is it within the aabb?
			if (lroom.m_AABB.has_point(pt))
			{
				// is it within the convex hull?
				float dist = lroom.m_Bound.GetClosestDistance(pt);

				// find the lowest within distance of the nearby room convex hulls
				if (dist < within_dist)
				{
					closest_within = n;
					within_dist = dist;
				}
			}
		}
	}

	// some logic whether to use the hulls or the closest dist
	if (within_dist < 1.0f)
	{
		return closest_within;
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


// register but let LPortal know which room the dob should start in
bool LRoomManager::dob_register_hint(Node * pDOB, float radius, Node * pRoom)
{
	if (!pDOB)
	{
		WARN_PRINT_ONCE("dob_register_hint : pDOB is NULL");
		return false;
	}

	LPRINT(3, "dob_register_hint " + pDOB->get_name());

	if (!pRoom)
	{
		WARN_PRINT_ONCE("dob_register_hint : pRoom is NULL");
		return false;
	}


	int iRoom = Obj_GetRoomNum(pRoom);

	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
	{
		WARN_PRINT_ONCE("dob_register_hint : DOB is not a spatial");
		return false;
	}


	return DobRegister(pSpat, radius, iRoom);
}


void LRoomManager::CreateDebug()
{
	ImmediateGeometry * p = memnew(ImmediateGeometry);
	p->set_name("debug_planes");
	add_child(p);

	m_ID_DebugPlanes = p->get_instance_id();

//	m_mat_Debug_Planes->set_as_toplevel(true);

	m_mat_Debug_Planes = Ref<SpatialMaterial>(memnew(SpatialMaterial));
	m_mat_Debug_Planes->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
//	m_mat_Debug_Planes->set_line_width(6.0);
//	m_mat_Debug_Planes->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
//	m_mat_Debug_Planes->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
//	m_mat_Debug_Planes->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
	m_mat_Debug_Planes->set_albedo(Color(1, 0, 1, 1));
	p->set_material_override(m_mat_Debug_Planes);
	p->hide();


	ImmediateGeometry * b = memnew(ImmediateGeometry);
	b->set_name("debug_bounds");
	add_child(b);
	m_ID_DebugBounds = b->get_instance_id();
	m_mat_Debug_Bounds = Ref<SpatialMaterial>(memnew(SpatialMaterial));
	//m_mat_Debug_Bounds->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
	m_mat_Debug_Bounds->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
	m_mat_Debug_Bounds->set_albedo(Color(0, 0, 1, 0.4));
	b->set_material_override(m_mat_Debug_Bounds);
	b->hide();
}


ObjectID LRoomManager::DobRegister_FindVIRecursive(Node * pNode) const
{
	// is the node a VI?
	VisualInstance * pVI = Object::cast_to<VisualInstance>(pNode);
	if (pVI)
		return pVI->get_instance_id();

	// try the children
	for (int n=0; n<pNode->get_child_count(); n++)
	{
		ObjectID res = DobRegister_FindVIRecursive(pNode->get_child(n));
		if (res)
			return res;
	}

	return 0;
}

bool LRoomManager::DobRegister(Spatial * pDOB, float radius, int iRoom)
{
	//LPRINT(3, "register_dob " + pDOB->get_name());

	if (iRoom == -1)
	{
		WARN_PRINT_ONCE("LRoomManager::DobRegister : room ID is -1");
		return false;
	}

	LRoom * pRoom = GetRoom(iRoom);
	if (!pRoom)
		return false;

	// The dob is derived from spatial, but the visual instances may be children of the dob
	// rather than the node itself .. we need visual instances for layer culling for shadows
	LDob dob;
	dob.m_ID_Spatial = pDOB->get_instance_id();
	dob.m_fRadius = radius;

	dob.m_ID_VI = DobRegister_FindVIRecursive(pDOB);

	pRoom->DOB_Add(dob);

	// save the room ID on the dob metadata
	Obj_SetRoomNum(pDOB, iRoom);

	// change visibility
	DobChangeVisibility(pDOB, 0, pRoom);

	return true;
}


bool LRoomManager::dob_register(Node * pDOB, float radius)
{
	if (!pDOB)
	{
		WARN_PRINT_ONCE("dob_register : pDOB is NULL");
		return false;
	}

	LPRINT(3, "dob_register " + pDOB->get_name());

	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
	{
		WARN_PRINT_ONCE("dob_register : DOB is not a spatial");
		return false;
	}

	Vector3 pt = pSpat->get_global_transform().origin;

	int iRoomNum = FindClosestRoom(pt);
	LPRINT(2, "dob_register closest room " + itos(iRoomNum));

	return DobRegister(pSpat, radius, iRoomNum);
}


int LRoomManager::dob_update(Node * pDOB)
{
	// find the room the object is attached to
	LRoom * pRoom = GetRoomFromDOB(pDOB);
	if (!pRoom)
		return -1;

	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
		return -1;

	LRoom * pNewRoom = pRoom->DOB_Update(*this, pSpat);

	if (pNewRoom)
	{
		// remove from the list in old room and add to list in new room, and change the metadata
		int iRoomNum = pNewRoom->m_RoomID;

		// get dob data to move to new room
		unsigned int dob_id = pRoom->DOB_Find(pDOB);
		assert (dob_id != -1);

		// copy across data before removing
		const LDob &data = pRoom->DOB_Get(dob_id);
		pNewRoom->DOB_Add(data);

		// remove from old room
		pRoom->DOB_Remove(dob_id);

		// change visibility
		DobChangeVisibility(pSpat, pRoom, pNewRoom);

		// save the room ID on the dob metadata
		Obj_SetRoomNum(pSpat, iRoomNum);

		// new room number
		return iRoomNum;
	}

	// still in the same room
	return pRoom->m_RoomID;
}

bool LRoomManager::dob_teleport_hint(Node * pDOB, Node * pRoom)
{
	if (!pDOB)
	{
		WARN_PRINT_ONCE("dob_teleport_hint : pDOB is NULL");
		return false;
	}

	LPRINT(1, "dob_teleport_hint " + pDOB->get_name());

	if (!pRoom)
	{
		WARN_PRINT_ONCE("dob_teleport_hint : pRoom is NULL");
		return false;
	}


	int iRoom = Obj_GetRoomNum(pRoom);

	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
	{
		WARN_PRINT_ONCE("dob_teleport_hint : DOB is not a spatial");
		return false;
	}

	return DobTeleport(pSpat, iRoom);
}


bool LRoomManager::DobTeleport(Spatial * pDOB, int iNewRoomID)
{
	// old room
	LRoom * pOldRoom = GetRoomFromDOB(pDOB);
	if (!pOldRoom)
	{
		WARN_PRINT_ONCE("LRoomManager::DobTeleport : pOldRoom is NULL");
		return false;
	}

	if (iNewRoomID == -1)
	{
		WARN_PRINT_ONCE("LRoomManager::DobTeleport : iNewRoomID is -1");
		return false;
	}

	LRoom * pNewRoom = GetRoom(iNewRoomID);
	if (!pNewRoom)
		return false;

	// detach from old room, add to new room
	// get dob data to move to new room
	unsigned int dob_id = pOldRoom->DOB_Find(pDOB);
	assert (dob_id != -1);

	// copy across data before removing
	const LDob &data = pOldRoom->DOB_Get(dob_id);
	pNewRoom->DOB_Add(data);

	// remove from old room
	pOldRoom->DOB_Remove(dob_id);

	// save the room ID on the dob metadata
	Obj_SetRoomNum(pDOB, iNewRoomID);

	// change visibility
	DobChangeVisibility(pDOB, pOldRoom, pNewRoom);

	return true;
}


// not tested...
bool LRoomManager::dob_teleport(Node * pDOB)
{
	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
		return false;

	Vector3 pt = pSpat->get_global_transform().origin;

	int iRoomNum = FindClosestRoom(pt);
	//print_line("dob_teleport closest room " + itos(iRoomNum));

	if (iRoomNum == -1)
		return false;

	return DobTeleport(pSpat, iRoomNum);
}



bool LRoomManager::dob_unregister(Node * pDOB)
{
	LRoom * pRoom = GetRoomFromDOB(pDOB);

	if (pRoom)
	{
		unsigned int dob_id = pRoom->DOB_Find(pDOB);
		return pRoom->DOB_Remove(dob_id);
	}

	return false;
}


bool LRoomManager::light_register(Node * pLightNode)
{
	if (!pLightNode)
	{
		WARN_PRINT_ONCE("light_register : pLightNode is NULL");
		return false;
	}

	LPRINT(3, "light_register " + pLightNode->get_name());

	Light * pLight = Object::cast_to<Light>(pLightNode);
	if (!pLight)
	{
		WARN_PRINT_ONCE("light_register : Node is not a light");
		return false;
	}

	// create new light
	LLight l;
	l.m_GodotID = pLight->get_instance_id();

	// direction
	Transform tr = pLight->get_global_transform();
	l.m_ptPos = tr.origin;
	l.m_ptDir = tr.basis.get_axis(2); // or possibly get_axis .. z is what we want

	m_Lights.push_back(l);

	return true;
}


void LRoomManager::DobChangeVisibility(Spatial * pDOB, const LRoom * pOld, const LRoom * pNew)
{
	bool bVisOld = false;
	bool bVisNew = false;

	if (pOld)
		bVisOld = pOld->IsVisible();

	if (pNew)
		bVisNew = pNew->IsVisible();


	if (bVisOld != bVisNew)
	{
		if (!bVisOld)
			pDOB->show();
		else
			pDOB->hide();
	}

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

void LRoomManager::rooms_set_debug_bounds(bool bActive)
{
	m_bDebugBounds = bActive;

	Object * pObj = ObjectDB::get_instance(m_ID_DebugBounds);
	ImmediateGeometry * im = Object::cast_to<ImmediateGeometry>(pObj);
	if (!im)
		return;

	if (bActive)
		im->show();
	else
		im->hide();
}


void LRoomManager::rooms_set_debug_planes(bool bActive)
{
	m_bDebugPlanes = bActive;

	Object * pObj = ObjectDB::get_instance(m_ID_DebugPlanes);
	ImmediateGeometry * im = Object::cast_to<ImmediateGeometry>(pObj);
	if (!im)
		return;

	if (bActive)
		im->show();
	else
		im->hide();
}


// turn on and off culling for debugging
void LRoomManager::rooms_set_active(bool bActive)
{
	if (bActive == m_bActive)
		return;

	m_bActive = bActive;

	if (m_bActive)
	{
		// clear these to ensure the system is initialized
		m_pCurr_VisibleRoomList->clear();
		m_pPrev_VisibleRoomList->clear();
	}

	// show all
	for (int n=0; n<m_Rooms.size(); n++)
	{
		LRoom &lroom = m_Rooms[n];
		lroom.Debug_ShowAll();
	}


}

void LRoomManager::rooms_set_logging(int level)
{
	// 0 is no logging, 6 is max logging (i.e. reverse of the priorities in the code)
	Lawn::LDebug::m_iLoggingLevel = 6-level;
}

// provide debugging output on the next frame
void LRoomManager::rooms_log_frame()
{
	Lawn::LDebug::m_bRunning = false;
}


void LRoomManager::rooms_set_camera(Node * pCam)
{
	m_ID_camera = 0;

	if (!pCam)
		return;

	Camera * pCamera = Object::cast_to<Camera>(pCam);
	if (!pCamera)
	{
		WARN_PRINT("Not a camera");
		return;
	}

	m_ID_camera = pCam->get_instance_id();

	// new .. select the cull layer
	pCamera->set_cull_mask_bit(LRoom::SOFT_HIDE_BIT, false);

	// use this temporarily to force debug
//	rooms_log_frame();
}

// convert empties and meshes to rooms and portals
void LRoomManager::rooms_convert()
{
	LRoomConverter conv;
	conv.Convert(*this);
}

// free memory for current set of rooms, prepare for converting a new game level
void LRoomManager::rooms_release()
{
	m_Lights.clear();
}


// debugging emulate view frustum
void LRoomManager::FrameUpdate_FrustumOnly()
{
	// NYI
}

void LRoomManager::FrameUpdate_Prepare()
{
	if (m_bDebugPlanes)
		m_DebugPlanes.clear();
	// clear the visible room list to write to each frame
	m_pCurr_VisibleRoomList->clear();

	// keep previous
	m_BF_master_SOBs_prev.CopyFrom(m_BF_master_SOBs);

	// note this can be done more efficiently with swapping pointer
	m_MasterList_SOBs_prev.copy_from(m_MasterList_SOBs);

	m_VisibleList_SOBs.clear();
	m_CasterList_SOBs.clear();
	m_MasterList_SOBs.clear();

	m_BF_caster_SOBs.Blank();
	m_BF_visible_SOBs.Blank();
	m_BF_master_SOBs.Blank();

	// as we hit visible rooms we will mark them in a bitset, so we can hide any rooms
	// that are showing that haven't been hit this frame
	m_BF_visible_rooms.Blank();

	// reset the planes pool for another frame
	m_Pool.Reset();
}

void LRoomManager::FrameUpdate()
{
	if (Engine::get_singleton()->is_editor_hint())
	{
		WARN_PRINT_ONCE("LRoomManager::FrameUpdate should not be called in editor");
		return;
	}

	// could turn off internal processing? not that important
	if (!m_bActive)
		return;

	if (m_bFrustumOnly)
	{
		// debugging emulate view frustum
		FrameUpdate_FrustumOnly();
		return;
	}

	// we keep a frame counter to prevent visiting things multiple times on the same frame in recursive functions
	m_uiFrameCounter++;
	LPRINT(5, "\nFRAME " + itos(m_uiFrameCounter));

	FrameUpdate_Prepare();


	// get the camera desired and make into lcamera
	Camera * pCamera = 0;
	if (m_ID_camera)
	{
		Object *pObj = ObjectDB::get_instance(m_ID_camera);
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

	// lcamera contains the info needed for culling
	LCamera cam;
	cam.m_ptPos = Vector3(0, 0, 0);
	cam.m_ptDir = Vector3 (-1, 0, 0);


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
	cam.m_ptDir = -tr.basis.get_axis(2); // or possibly get_axis .. z is what we want

	// luckily godot already has a function to return a list of the camera clipping planes
	planes.copy_from(pCamera->get_frustum());

	// the whole visibility algorithm is recursive, spreading out from the camera room,
	// rendering through any portals in view into other rooms, etc etc
	pRoom->DetermineVisibility_Recursive(*this, 0, cam, planes);

	// finally hide all the rooms that are currently visible but not in the visible bitfield as having been hit
	FrameUpdate_FinalizeRooms();

	FrameUpdate_AddShadowCasters();

	FrameUpdate_CreateMasterList();

	// set soft visibility of objects within visible rooms
	FrameUpdate_FinalizeVisibility_WithinRooms();

	FrameUpdate_FinalizeVisibility_SoftShow();

	// swap the current and previous visible room list
	LVector<int> * pTemp = m_pCurr_VisibleRoomList;
	m_pCurr_VisibleRoomList = m_pPrev_VisibleRoomList;
	m_pPrev_VisibleRoomList = pTemp;


	// draw debug
	FrameUpdate_DrawDebug(cam, *pRoom);

	// when running, emit less debugging output so as not to choke the IDE
	Lawn::LDebug::m_bRunning = true;
}

void LRoomManager::FrameUpdate_FinalizeRooms()
{
	// finally hide all the rooms that are currently visible but not in the visible bitfield as having been hit

	// to get started
	if (!m_pPrev_VisibleRoomList->size())
	{
		// NOTE this will be done more efficiently, but is okay to start with
		for (int n=0; n<m_Rooms.size(); n++)
		{
			if (!m_BF_visible_rooms.GetBit(n))
			{
				m_Rooms[n].Room_MakeVisible(false);
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
				m_Rooms[r].Room_MakeVisible(false);
		}

	}
}

// to optimize just 1 call to the visual server, we want a list of all sobs that are either visible or casters
// this allows 1 call to show / hide, and 1 call to layer flags
void LRoomManager::FrameUpdate_CreateMasterList()
{
	for (int n=0; n<m_VisibleList_SOBs.size(); n++)
	{
		int sob_id = m_VisibleList_SOBs[n];
		m_MasterList_SOBs.push_back(sob_id);
		m_BF_master_SOBs.SetBit(sob_id, true);
	}

	for (int n=0; n<m_CasterList_SOBs.size(); n++)
	{
		int sob_id = m_CasterList_SOBs[n];

		// if not already on master list
		if (m_BF_master_SOBs.GetBit(sob_id) == 0)
		{
			m_MasterList_SOBs.push_back(sob_id);
			m_BF_master_SOBs.SetBit(sob_id, true);
		}
	}

}


void LRoomManager::FrameUpdate_AddShadowCasters()
{
	// simple for the moment, add all objects in visible rooms as casters if they are not already visible
	for (int n=0; n<m_pCurr_VisibleRoomList->size(); n++)
	{
		int r = (*m_pCurr_VisibleRoomList)[n];
		m_Rooms[r].AddShadowCasters(*this);
	}

}

void LRoomManager::FrameUpdate_FinalizeVisibility_SoftShow()
{
	// apply the appropriate soft show for each sob in the render list
	int nSOBs = m_MasterList_SOBs.size();

	for (int n=0; n<nSOBs; n++)
	{
		int ID = m_MasterList_SOBs[n];
		const LSob &sob	 = m_SOBs[ID];

		VisualInstance * pVI = sob.GetVI();

		if (pVI)
		{
			//SoftShow(pVI, sob.m_bSOBVisible);
			bool bVisible = m_BF_visible_SOBs.GetBit(ID) != 0;
			LRoom::SoftShow(pVI, bVisible);
		}
	}

}


void LRoomManager::FrameUpdate_FinalizeVisibility_WithinRooms()
{
	// and hide all the dobs that are in visible rooms that haven't been made visible
//	if (m_pCurr_VisibleRoomList->size() == 0)
//		print_line("WARNING : vis room list size is 0");

	for (int n=0; n<m_pCurr_VisibleRoomList->size(); n++)
	{
		int r = (*m_pCurr_VisibleRoomList)[n];
		m_Rooms[r].FinalizeVisibility(*this);
	}

	// NEW shows and hides dobs according to the difference between the current and previous master list
	for (int n=0; n<m_MasterList_SOBs_prev.size(); n++)
	{
		int ID = m_MasterList_SOBs_prev[n];
		if (m_BF_master_SOBs.GetBit(ID) == 0)
		{
			LSob &sob = m_SOBs[ID];
			sob.Show(false);
		}
	}

	// show all in current master list
	for (int n=0; n<m_MasterList_SOBs.size(); n++)
	{
		int ID = m_MasterList_SOBs[n];
		LSob &sob = m_SOBs[ID];
		sob.Show(true);
	}

}


void LRoomManager::FrameUpdate_DrawDebug(const LCamera &cam, const LRoom &lroom)
{
	if (m_bDebugPlanes)
	{
		Vector3 ptCam = cam.m_ptPos;
		// slight adjustment to prevent parallel lines in viewport
		ptCam += (cam.m_ptDir * 0.1f);

		Object * pObj = ObjectDB::get_instance(m_ID_DebugPlanes);
		ImmediateGeometry * im = Object::cast_to<ImmediateGeometry>(pObj);
		if (!im)
			return;

		im->clear();

		im->begin(Mesh::PRIMITIVE_LINES, NULL);

		int nVerts = m_DebugPlanes.size();

		for (int n=0; n<nVerts; n++)
		{
			im->add_vertex(ptCam);
			im->add_vertex(m_DebugPlanes[n]);
		}
		im->end();
	}

	// if debug bounds are on and there is a bound for this room
	const Geometry::MeshData &md = lroom.m_Bound_MeshData;
	if (m_bDebugBounds && md.faces.size())
	{
		Object * pObj = ObjectDB::get_instance(m_ID_DebugBounds);
		ImmediateGeometry * im = Object::cast_to<ImmediateGeometry>(pObj);
		if (!im)
			return;

		im->clear();

		im->begin(Mesh::PRIMITIVE_TRIANGLES, NULL);

		for (int n=0; n<md.faces.size(); n++)
		{
			const Geometry::MeshData::Face &f = md.faces[n];

			int numTris = f.indices.size() - 2;

			for (int t=0; t<numTris; t++)
			{
				im->set_normal(f.plane.normal);
				im->add_vertex(md.vertices[f.indices[0]]);
				im->add_vertex(md.vertices[f.indices[t+1]]);
				im->add_vertex(md.vertices[f.indices[t+2]]);
			}
		}

		im->end();
	}
}

void LRoomManager::_notification(int p_what) {

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
			// turn on process, unless we are in the editor
			if (!Engine::get_singleton()->is_editor_hint())
			{
				set_process_internal(true);
				CreateDebug();
			}
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
	ClassDB::bind_method(D_METHOD("rooms_release"), &LRoomManager::rooms_release);

	ClassDB::bind_method(D_METHOD("rooms_set_camera"), &LRoomManager::rooms_set_camera);
	ClassDB::bind_method(D_METHOD("rooms_get_room"), &LRoomManager::rooms_get_room);

	// debugging
	ClassDB::bind_method(D_METHOD("rooms_set_logging"), &LRoomManager::rooms_set_logging);
	ClassDB::bind_method(D_METHOD("rooms_log_frame"), &LRoomManager::rooms_log_frame);
	ClassDB::bind_method(D_METHOD("rooms_set_active"), &LRoomManager::rooms_set_active);
	ClassDB::bind_method(D_METHOD("rooms_set_debug_planes"), &LRoomManager::rooms_set_debug_planes);
	ClassDB::bind_method(D_METHOD("rooms_set_debug_bounds"), &LRoomManager::rooms_set_debug_bounds);


	// functions to add dynamic objects to the culling system
	// Note that these should not be placed directly in rooms, the system will 'soft link' to them
	// so they can be held, e.g. in pools elsewhere in the scene graph
	ClassDB::bind_method(D_METHOD("dob_register"), &LRoomManager::dob_register);
	ClassDB::bind_method(D_METHOD("dob_unregister"), &LRoomManager::dob_unregister);
	ClassDB::bind_method(D_METHOD("dob_update"), &LRoomManager::dob_update);
	ClassDB::bind_method(D_METHOD("dob_teleport"), &LRoomManager::dob_teleport);

	ClassDB::bind_method(D_METHOD("dob_register_hint"), &LRoomManager::dob_register_hint);
	ClassDB::bind_method(D_METHOD("dob_teleport_hint"), &LRoomManager::dob_teleport_hint);

	ClassDB::bind_method(D_METHOD("dob_get_room_id"), &LRoomManager::dob_get_room_id);

	ClassDB::bind_method(D_METHOD("light_register"), &LRoomManager::light_register);

}
