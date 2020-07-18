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
#include "scene/3d/baked_lightmap.h"
#include "lroom.h"
#include "lhelper.h"
#include "lscene_saver.h"
#include "ldae_exporter.h"

#define LROOMLIST m_pRoomList
#define CHECK_ROOM_LIST if (!CheckRoomList())\
{\
WARN_PRINT_ONCE("rooms is unset");\
return false;\
}


LRoomManager::LRoomManager()
{
//	m_ID_camera = 0;
	m_DOB_id_camera = -1;
	m_ID_DebugPlanes = 0;
	m_ID_DebugBounds = 0;
	m_ID_DebugLights = 0;
	m_ID_DebugLightVolumes = 0;
	m_ID_DebugFrustums = 0;

	m_ID_RoomList = 0;

	m_uiFrameCounter = 0;
	m_iLoggingLevel = 2;
	m_bActive = true;
	m_bFrustumOnly = false;

	m_bPortalPlane_Convention = false;

	// to know which rooms to hide we keep track of which were shown this, and the previous frame
	m_pCurr_VisibleRoomList = &m_VisibleRoomList_A;
	m_pPrev_VisibleRoomList = &m_VisibleRoomList_B;

	m_bDebugPlanes = false;
	m_bDebugBounds = false;
	m_bDebugLights = false;
	m_bDebugLightVolumes = false;
	m_bDebugFrustums = false;
	m_bDebugFrameString = false;

	m_pRoomList = 0;



	// loses detail at approx .. 0.00001
	// dot 0.9999
	// however, this will depend on the map so we will use some more approximate figures, and allow
	// the user to change in rare cases where this fails
	m_fLightmapUnMerge_ThresholdDist = 0.001f;
	m_fLightmapUnMerge_ThresholdDot = 0.99f;

	if (!Engine::get_singleton()->is_editor_hint())
	{
		CreateDebug();
	}
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
				float dist = lroom.m_Bound.GetSmallestPenetrationDistance(pt);

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
	if ((unsigned int) i >= (unsigned int) m_Rooms.size())
	{
		WARN_PRINT_ONCE("LRoomManager::GetRoom out of range");
		return 0;
	}
	return &m_Rooms[i];
}

LRoom * LRoomManager::GetRoom(int i)
{
	if ((unsigned int) i >= (unsigned int) m_Rooms.size())
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


// for lights we store the light ID in the metadata
void LRoomManager::Meta_SetLightID(Node * pNode, int id)
{
	pNode->set_meta("_llight", id);
}

int LRoomManager::Meta_GetLightID(Node * pNode) const
{
	//assert (pNode->has_meta("_lroom"));
	Variant v = pNode->get_meta("_llight");
	if (v.get_type() == Variant::NIL)
		return -1;

	return v;
}


/*
  CANT BE TRUSTED
void LRoomManager::Meta_SetRoomNum(Node * pNode, int num)
{
	pNode->set_meta("_lroom", num);

	assert (Meta_GetRoomNum(pNode) == num);
}

int LRoomManager::Meta_GetRoomNum(Node * pNode) const
{
	//assert (pNode->has_meta("_lroom"));
	Variant v = pNode->get_meta("_lroom");
	if (v.get_type() == Variant::NIL)
		return -1;

	return v;
}
*/


/*
// register but let LPortal know which room the dob should start in
bool LRoomManager::dob_register_hint(Node * pDOB, float radius, Node * pRoom)
{
	CHECK_ROOM_LIST

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


	int iRoom = Meta_GetRoomNum(pRoom);

	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
	{
		WARN_PRINT_ONCE("dob_register_hint : DOB is not a spatial");
		return false;
	}


	return DobRegister(pSpat, radius, iRoom);
}
*/

void LRoomManager::CreateDebug()
{
	ImmediateGeometry * p = memnew(ImmediateGeometry);
	p->set_name("debug_planes");
	add_child(p);
	move_child(p, get_child_count()-1);

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
	move_child(b, get_child_count()-1);
	m_ID_DebugBounds = b->get_instance_id();
	m_mat_Debug_Bounds = Ref<SpatialMaterial>(memnew(SpatialMaterial));
	//m_mat_Debug_Bounds->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
	m_mat_Debug_Bounds->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
	m_mat_Debug_Bounds->set_cull_mode(SpatialMaterial::CULL_DISABLED);
	m_mat_Debug_Bounds->set_albedo(Color(0, 0, 1, 0.4));
	b->set_material_override(m_mat_Debug_Bounds);
	b->hide();

	{
	ImmediateGeometry * b = memnew(ImmediateGeometry);
	b->set_name("debug_lights");
	add_child(b);
	move_child(b, get_child_count()-1);
	m_ID_DebugLights = b->get_instance_id();
	b->set_material_override(m_mat_Debug_Bounds);
	b->hide();
	}

	{
	ImmediateGeometry * p = memnew(ImmediateGeometry);
	p->set_name("debug_lightvolumes");
	add_child(p);
	move_child(p, get_child_count()-1);

	m_ID_DebugLightVolumes = p->get_instance_id();

//	m_mat_Debug_Planes->set_as_toplevel(true);

	m_mat_Debug_LightVolumes = Ref<SpatialMaterial>(memnew(SpatialMaterial));
	m_mat_Debug_LightVolumes->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
//	m_mat_Debug_Planes->set_line_width(6.0);
//	m_mat_Debug_Planes->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
//	m_mat_Debug_Planes->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
//	m_mat_Debug_Planes->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
	m_mat_Debug_LightVolumes->set_albedo(Color(0, 1, 1, 1));
	p->set_material_override(m_mat_Debug_LightVolumes);
	p->hide();
	}

	{
	ImmediateGeometry * p = memnew(ImmediateGeometry);
	p->set_name("debug_frustums");
	add_child(p);
	move_child(p, get_child_count()-1);

	m_ID_DebugFrustums = p->get_instance_id();

//	m_mat_Debug_Planes->set_as_toplevel(true);

	m_mat_Debug_Frustums = Ref<SpatialMaterial>(memnew(SpatialMaterial));
	m_mat_Debug_Frustums->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
//	m_mat_Debug_Planes->set_line_width(6.0);
//	m_mat_Debug_Planes->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
//	m_mat_Debug_Planes->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
//	m_mat_Debug_Planes->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
	m_mat_Debug_Frustums->set_albedo(Color(1, 0, 0, 1));
	p->set_material_override(m_mat_Debug_Frustums);
	p->hide();
	}

}


ObjectID LRoomManager::DobRegister_FindVIRecursive(Node * pNode) const
{
	// is the node a VI?
	VisualInstance * pVI = Object::cast_to<VisualInstance>(pNode);
	if (pVI)
	{
		// take away layer 0 from the dob, so it can be culled effectively
		pVI->set_layer_mask(0);

		return pVI->get_instance_id();
	}

	// try the children
	for (int n=0; n<pNode->get_child_count(); n++)
	{
		ObjectID res = DobRegister_FindVIRecursive(pNode->get_child(n));
		if (res)
			return res;
	}

	return 0;
}

int LRoomManager::DobRegister(Spatial * pDOB, const Vector3 &pos, float radius, int iRoom)
{
	//LPRINT(3, "register_dob " + pDOB->get_name());

	if (iRoom == -1)
	{
		WARN_PRINT_ONCE("LRoomManager::DobRegister : room ID is -1");
		return false;
	}

	int did = m_DobList.Request();
	LDob &dob = m_DobList.GetDob(did);

	dob.m_iRoomID = iRoom;
	dob.m_ID_Spatial = pDOB->get_instance_id();
	dob.m_fRadius = radius;

//	LRoom * pRoom = GetRoom(iRoom);
//	if (!pRoom)
//		return false;

	// The dob is derived from spatial, but the visual instances may be children of the dob
	// rather than the node itself .. we need visual instances for layer culling for shadows
//	LDob dob;
//	dob.m_ID_Spatial = pDOB->get_instance_id();
//	dob.m_fRadius = radius;

	dob.m_ID_VI = DobRegister_FindVIRecursive(pDOB);

//	pRoom->DOB_Add(dob);

	// save the room ID on the dob metadata
//	Meta_SetRoomNum(pDOB, iRoom);

#ifdef LPORTAL_DOBS_NO_SOFTSHOW
	VisualInstance * pVI = dob.GetVI();
	if (pVI)
	{
		uint32_t mask = 0;
		mask = LRoom::LAYER_MASK_CAMERA | LRoom::LAYER_MASK_LIGHT;
		LRoom::SoftShow(pVI, mask);
	}
#endif

	// change visibility
	m_DobList.UpdateDob(*this, did, pos);
//	DobUpdateVisibility(pDOB, pRoom);

	return did;
}


int LRoomManager::dob_register(Node * pDOB, const Vector3 &pos, float radius)
{
	CHECK_ROOM_LIST

	if (!pDOB)
	{
		WARN_PRINT_ONCE("dob_register : pDOB is NULL");
		return -1;
	}

	LPRINT(3, "dob_register " + pDOB->get_name() + " instance ID " + itos(pDOB->get_instance_id()));

	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
	{
		WARN_PRINT_ONCE("dob_register : DOB is not a spatial");
		return -1;
	}

//	Vector3 pt = pSpat->get_global_transform().origin;

	int iRoomNum = FindClosestRoom(pos);
	LPRINT(2, "dob_register closest room " + itos(iRoomNum));

	return DobRegister(pSpat, pos, radius, iRoomNum);
}


/*
// where pRoom is current room
int LRoomManager::DobUpdate(Spatial * pDOB_Spatial, LRoom * pRoom)
{
	LRoom * pNewRoom = pRoom->DOB_Update(*this, pDOB_Spatial);

//	// update visibility
//	if (pNewRoom)
//		DobUpdateVisibility(pSpat, pNewRoom);
//	else
//		DobUpdateVisibility(pSpat, pRoom);


	if (pNewRoom)
	{
		// remove from the list in old room and add to list in new room, and change the metadata
		int iRoomNum = pNewRoom->m_RoomID;

		// get dob data to move to new room
		//unsigned int uidob_instance_id = pDOB->get_instance_id();

		unsigned int dob_id = pRoom->DOB_Find(pDOB_Spatial);
//		if (dob_id == -1)
//		{
//			WARN_PRINT_ONCE("DOB is not found in room");
//			return -1;
//		}
		assert (dob_id != -1);

		print_line("dob " + itos(dob_id) + " entering room " + pNewRoom->get_name());

		// copy across data before removing
		const LDob &data = pRoom->DOB_Get(dob_id);
		pNewRoom->DOB_Add(data);

		// remove from old room
		pRoom->DOB_Remove(dob_id);

		// change visibility
		DobUpdateVisibility(pDOB_Spatial, pNewRoom);

		// save the room ID on the dob metadata
		Meta_SetRoomNum(pDOB_Spatial, iRoomNum);

		// new room number
		return iRoomNum;
	}

//#ifdef LDEBUG_DOB_VISIBILITY
//#pragma message ("LPortal LDEBUG_DOB_VISIBILITY, dobs will flicker when hidden")
//	bool bVis = pRoom->IsVisible();

//	bool bSpatVisible = pSpat->is_visible_in_tree();
//	if (bVis != bSpatVisible)
//	{
//		WARN_PRINT("DOB visibility incorrect");
//	}

//	bool bShow = true;

//	if (!bVis)
//	{
//		if ((Engine::get_singleton()->get_frames_drawn() % 2) == 0)
//		{
//			pSpat->show();
//		}
//		else
//		{
//			pSpat->hide();
//		}
//	}
//	else
//	{
//		pSpat->show();
//	}
//#endif


	// still in the same room
	return pRoom->m_RoomID;
}
*/

int LRoomManager::dob_update(int dob_id, const Vector3 &pos)
{
#ifdef LPORTAL_DOBS_AUTO_UPDATE
	return -1;
#endif



	return m_DobList.UpdateDob(*this, dob_id, pos);



	// find the room the object is attached to
//	LRoom * pRoom = GetRoomFromDOB(pDOB);
//	if (!pRoom)
//		return -1;

//	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
//	if (!pSpat)
//		return -1;

//	return DobUpdate(pSpat, pRoom);
}

/*
bool LRoomManager::dob_teleport_hint(Node * pDOB, Node * pRoom)
{
	CHECK_ROOM_LIST

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


	int iRoom = Meta_GetRoomNum(pRoom);

	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
	{
		WARN_PRINT_ONCE("dob_teleport_hint : DOB is not a spatial");
		return false;
	}

	return DobTeleport(pSpat, iRoom);
}
*/

/*
bool LRoomManager::DobTeleport(Spatial * pDOB, int iNewRoomID)
{
	LPRINT(5, "teleporting " + pDOB->get_name() + " to room " + itos(iNewRoomID));

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

	// special case, if teleporting within the same room, no op
	// (if we do try and move from same room to same room we get data corruption because of
	// const ref)
	if (pOldRoom == pNewRoom)
		return true;

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
	Meta_SetRoomNum(pDOB, iNewRoomID);

	// change visibility
	DobUpdateVisibility(pDOB, pNewRoom);

	return true;
}
*/

// not tested...
/*
bool LRoomManager::dob_teleport(Node * pDOB)
{
	CHECK_ROOM_LIST

	return true;

//	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
//	if (!pSpat)
//		return false;

//	Vector3 pt = pSpat->get_global_transform().origin;

//	int iRoomNum = FindClosestRoom(pt);
//	//print_line("dob_teleport closest room " + itos(iRoomNum));

//	if (iRoomNum == -1)
//		return false;

//	return DobTeleport(pSpat, iRoomNum);
}
*/


bool LRoomManager::dob_unregister(int dob_id)
{
	CHECK_ROOM_LIST

//	LRoom * pRoom = GetRoomFromDOB(pDOB);

//	// change the meta data on the DOB .. this will catch trying to update an unregistered DOB
//	Meta_SetRoomNum(pDOB, -2);

//	if (pRoom)
//	{
//		unsigned int dob_id = pRoom->DOB_Find(pDOB);
//		return pRoom->DOB_Remove(dob_id);
//	}

	m_DobList.DeleteDob(dob_id);

	return true;
}


void LRoomManager::Light_FrameProcess(int lightID)
{
	if (!m_BF_ProcessedLights.GetBit(lightID))
	{
		m_BF_ProcessedLights.SetBit(lightID, true);

		// some lights may be processed but found not to intersect the camera frustum
		if (Light_FindCasters(lightID))
		{
			m_BF_ActiveLights.SetBit(lightID, true);
			m_ActiveLights.push_back(lightID);
		}
	}
}

// now we are centralizing the tracing out from static and dynamic lights for each frame to this function
// returns false if the entire light should be culled
bool LRoomManager::Light_FindCasters(int lightID)
{
	// add all shadow casters for this light (new method)
	const LLight &light = m_Lights[lightID];
/*
	if (light.m_eClass == LLight::LT_STATIC)
	{
		int last_caster = light.m_FirstCaster + light.m_NumCasters;
		for (int c=light.m_FirstCaster; c<last_caster; c++)
		{
			int sobID = m_LightCasters_SOB[c];

			// only add to the caster list if not in it already (does this check need to happen, can this ever occur?)
			if (!m_BF_caster_SOBs.GetBit(sobID))
			{
				LPRINT_RUN(2, "\t" + itos(sobID) + ", " + m_SOBs[sobID].GetSpatial()->get_name());
				m_BF_caster_SOBs.SetBit(sobID, true);
				m_CasterList_SOBs.push_back(sobID);
			}
			else
			{
				//LPRINT(2, "\t" + itos(sobID) + ", ALREADY CASTER " + manager.m_SOBs[sobID].GetSpatial()->get_name());
			}

		} // for c through caster
	} // static lights have a list of SOB casters
*/

	// special case of global area lights
	if (light.m_iArea != -1)
	{
		// special trace for area light
		if (m_Trace.Trace_Light(*this, light, LTrace::LR_ALL) == false)
			return false;
	}
	else
	{
		// can only deal with lights in rooms for now
		if (light.m_Source.m_RoomID == -1)
		{
			return true;
		}

		LRoom * pRoom = GetRoom(light.m_Source.m_RoomID);
		if (!pRoom)
			return true;

		if (m_Trace.Trace_Light(*this, light, LTrace::LR_ALL) == false)
			return false;

	} // non-area light

	/*
	// we now need to trace either just DOBs (in the case of static lights)
	// or SOBs and DOBs (in the case of dynamic lights)
	m_LightRender.m_BF_Temp_SOBs.Blank();
	m_LightRender.m_Temp_Visible_SOBs.clear();

	const LSource &cam = light.m_Source;
//	cam.Source_SetDefaults();
//	cam.m_ptPos = light.m_ptPos;
//	cam.m_ptDir = light.m_ptDir;

	m_Trace.Trace_Prepare(*this, cam, m_LightRender.m_BF_Temp_SOBs, m_BF_visible_rooms, m_LightRender.m_Temp_Visible_SOBs, *m_pCurr_VisibleRoomList);


	unsigned int pool_member = m_Pool.Request();
	assert (pool_member != -1);

	LVector<Plane> &planes = m_Pool.Get(pool_member);
	planes.clear();

	// create subset planes of light frustum and camera frustum
	m_MainCamera.AddCameraLightPlanes(*this, cam, planes);

	m_Trace.Trace_Begin(*pRoom, planes);

	// we no longer need these planes
	m_Pool.Free(pool_member);
*/
	// process the sobs that were visible
	for (int n=0; n<m_LightRender.m_Temp_Visible_SOBs.size(); n++)
	{
		int sobID = m_LightRender.m_Temp_Visible_SOBs[n];

		// only add to the caster list if not in it already (does this check need to happen, can this ever occur?)
		if (!m_BF_caster_SOBs.GetBit(sobID))
		{
			LPRINT_RUN(2, "\t" + itos(sobID) + ", " + m_SOBs[sobID].GetSpatial()->get_name());
			m_BF_caster_SOBs.SetBit(sobID, true);
			m_CasterList_SOBs.push_back(sobID);
		}
		else
		{
			//LPRINT(2, "\t" + itos(sobID) + ", ALREADY CASTER " + manager.m_SOBs[sobID].GetSpatial()->get_name());
		}

	}

	return true;
}

void LRoomManager::Light_UpdateTransform(LLight &light, const Light &glight) const
{
	if (!glight.get_parent())
	{
		WARN_PRINT_ONCE("LRoomManager::Light_UpdateTransform Light is not in tree");
		return;
	}

	// get global transform only works if glight is in the tree
	Transform tr = glight.get_global_transform();
	light.m_Source.m_ptPos = tr.origin;
	light.m_Source.m_ptDir = -tr.basis.get_axis(2); // or possibly get_axis .. z is what we want
}

// common stuff for global and local light creation
bool LRoomManager::LightCreate(Light * pLight, int roomID, String szArea)
{
	// set culling flag for light
	// 1 is for lighting objects outside the room system
	pLight->set_cull_mask(1 | LRoom::LAYER_MASK_LIGHT);

	// create new light
	LLight l;
	l.Light_SetDefaults();
	l.Hidable_Create(pLight);
	l.m_GodotID = pLight->get_instance_id();
	//l.m_iArea = areaID;

	// store the area name as a string if an area light
	// as the areas aren't actually created until calling convert
	if (szArea != "")
		l.m_szArea = szArea;

	LSource &lsource = l.m_Source;

	// direction
	Light_UpdateTransform(l, *pLight);
	lsource.m_RoomID = roomID;

	bool bOK = false;

	// what kind of light?
	SpotLight * pSL = Object::cast_to<SpotLight>(pLight);
	if (pSL)
	{
		LPRINT(2, "\tSPOTLIGHT detected " + pLight->get_name());
		lsource.m_eType = LSource::ST_SPOTLIGHT;
		lsource.m_fSpread = pSL->get_param(Light::PARAM_SPOT_ANGLE);
		lsource.m_fRange = pLight->get_param(Light::PARAM_RANGE);

		bOK = true;
	}

	OmniLight * pOL = Object::cast_to<OmniLight>(pLight);
	if (pOL)
	{
		LPRINT(2, "\tOMNILIGHT detected " + pLight->get_name());
		lsource.m_eType = LSource::ST_OMNI;
		lsource.m_fRange = pLight->get_param(Light::PARAM_RANGE);
		bOK = true;
	}

	DirectionalLight * pDL = Object::cast_to<DirectionalLight>(pLight);
	if (pDL)
	{
		LPRINT(2, "\tDIRECTIONALLIGHT detected " + pLight->get_name());
		lsource.m_eType = LSource::ST_DIRECTIONAL;

		// no range but max distance? NYI

		bOK = true;
	}

	// don't add if not recognised
	if (!bOK)
	{
		LPRINT(2, "\tLIGHT type unrecognised " + pLight->get_name());
		return false;
	}


	// turn the local light off to start with
	if (!lsource.IsGlobal())
	{
//		l.Show(false);
		//pLight->hide();
	}

	m_Lights.push_back(l);

	return true;
}


int LRoomManager::dynamic_light_register(Node * pLightNode, float radius)
{
	CHECK_ROOM_LIST
	if (!pLightNode)
	{
		WARN_PRINT_ONCE("dynamic_light_register : pLightNode is NULL");
		return -1;
	}

	ObjectID light_id = pLightNode->get_instance_id();

	// does the light already exist in the light list? I.e. was it imported as part of the roomlist?
	for (int n=0; n<m_Lights.size(); n++)
	{
		if (m_Lights[n].m_GodotID == light_id)
		{
			m_Lights[n].m_Source.m_eClass = LSource::SC_DYNAMIC;


			// do an update on the light
			//dynamic_light_update(n, pos, dir);

			// store the light ID in the metadata for the node
			//Meta_SetLightID(pLightNode, n);
			return n;
		}
	}

	return -1;
}


bool LRoomManager::dynamic_light_unregister(int light_id)
{
	// NYI
	return true;
}

// returns room within or -1 if no dob
int LRoomManager::dynamic_light_update(int light_id, const Vector3 &pos, const Vector3 &dir) // returns room within
{
	// doesn't now matter if not in tree as position and dir are passed directly
	if ((unsigned int) light_id >= (unsigned int) m_Lights.size())
	{
		WARN_PRINT_ONCE("dynamic_light_update : meta light ID out of range");
		return -1;
	}

	LLight &light = m_Lights[light_id];

	int iRoom = light.m_Source.m_RoomID;
	if (iRoom == -1)
	{
		WARN_PRINT_ONCE("dynamic_light_update : can't update global light");
		return -1;
	}

	light.m_Source.m_ptPos = pos;
	light.m_Source.m_ptDir = dir.normalized();

	// update dob
	if (light.m_DOB_id != -1)
	{
		int iNewRoom = m_DobList.UpdateDob(*this, light.m_DOB_id, pos);
		light.m_Source.m_RoomID = iNewRoom;
	}

	// update with a new Trace (we are assuming update is only called if the light has moved)
	// remove the old local lights
	for (int n=0; n<light.m_NumAffectedRooms; n++)
	{
		int r = light.m_AffectedRooms[n];
		GetRoom(r)->RemoveLocalLight(light_id);
	}
	light.ClearAffectedRooms();


	// now do a new trace, and add all the rooms that are hit
	m_Trace.Trace_Light(*this, light, LTrace::LR_ROOMS);

	// we should now have a list of the rooms hit in m_LightRender.m_Temp_Visible_Rooms
	for (int n=0; n<m_LightRender.m_Temp_Visible_Rooms.size(); n++)
	{
		int r = m_LightRender.m_Temp_Visible_Rooms[n];

		// add to the list on the light
		light.AddAffectedRoom(r);

		// add to the list of local lights in the room
		GetRoom(r)->AddLocalLight(light_id);

	}

	// this may or may not have changed
	return light.m_Source.m_RoomID;

	/*
	if (!pLightNode)
	{
		WARN_PRINT_ONCE("dynamic_light_update : pLightNode is NULL");
		return -1;
	}

	if (pLightNode->is_inside_tree() == false)
	{
#ifdef LDEBUG_LIGHT_AFFECTED_ROOMS
		if (m_bDebugFrameString)
			DebugString_Add("DynamicLight not in tree\n");
#endif
		return -1;
	}

	// find the light ID from meta data
	int light_id = Meta_GetLightID(pLightNode);

	if ((unsigned int) light_id >= (unsigned int) m_Lights.size())
	{
		WARN_PRINT_ONCE("dynamic_light_update : meta light ID out of range");
		return -1;
	}

	LLight &light = m_Lights[light_id];

	// update the llight transform from the node
	Light * pGLight = light.GetGodotLight();
	Light_UpdateTransform(light, *pGLight);

	Spatial * pSpat = Object::cast_to<Spatial>(pGLight);
	if (!pSpat)
		return -1;

	int iRoom = light.m_Source.m_RoomID;
	if (iRoom == -1)
	{
		WARN_PRINT_ONCE("dynamic_light_update : can't update global light");
		return -1;
	}


	LRoom * pRoom = GetRoom(iRoom);
	if (pRoom == 0)
	{
		WARN_PRINT_ONCE("dynamic_light_update : pRoom is NULL");
		return -1;
	}

	LRoom * pNewRoom = pRoom->DOB_Update(*this, pSpat);

	if (pNewRoom)
	{
		// remove from the list in old room and add to list in new room, and change the metadata
		int iNewRoomID = pNewRoom->m_RoomID;
		light.m_Source.m_RoomID = iNewRoomID;

		// change visibility
		//DobChangeVisibility(pSpat, pRoom, pNewRoom);
	}


	// update with a new Trace (we are assuming update is only called if the light has moved)
	// remove the old local lights
	for (int n=0; n<light.m_NumAffectedRooms; n++)
	{
		int r = light.m_AffectedRooms[n];
		GetRoom(r)->RemoveLocalLight(light_id);
	}
	light.ClearAffectedRooms();


	// now do a new trace, and add all the rooms that are hit
	m_Trace.Trace_Light(*this, light, LTrace::LR_ROOMS);

	// we should now have a list of the rooms hit in m_LightRender.m_Temp_Visible_Rooms
	for (int n=0; n<m_LightRender.m_Temp_Visible_Rooms.size(); n++)
	{
		int r = m_LightRender.m_Temp_Visible_Rooms[n];

		// add to the list on the light
		light.AddAffectedRoom(r);

		// add to the list of local lights in the room
		GetRoom(r)->AddLocalLight(light_id);

	}

	// this may or may not have changed
	return light.m_Source.m_RoomID;
	*/
	return 0;
}

void LRoomManager::DebugString_Light_AffectedRooms(int light_id)
{
#ifdef LDEBUG_LIGHT_AFFECTED_ROOMS
	if (m_bDebugFrameString)
	{
		const LLight &light = m_Lights[light_id];
		if (!light.m_bShow)
			return;

		DebugString_Add("Light " + itos(light_id) + " affect room ");
		// affected rooms
		for (int n=0; n<light.m_NumAffectedRooms; n++)
		{
			int room_id = light.m_AffectedRooms[n];
			DebugString_Add(itos(room_id) + ", ");
		}
		DebugString_Add("\n");
	}
#endif
}


bool LRoomManager::global_light_register(Node * pLightNode, String szArea)
{
	//CHECK_ROOM_LIST

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

	DirectionalLight * pDLight = 	Object::cast_to<DirectionalLight>(pLightNode);
	if (!pDLight)
	{
		WARN_PRINT_ONCE("light_register : only DirectionalLights are supported, place spotlights and omnis within rooms");
		return false;
	}

	return LightCreate(pLight, -1, szArea);
}

/*
void LRoomManager::DobUpdateVisibility(int dob_id)
{
	LDob &dob = m_DobList.GetDob(dob_id);

//	return;

	bool bRoomVis = pRoom->IsVisible();
	bool bDobVis = pDOB->is_visible_in_tree();

	if (bDobVis != bRoomVis)
	{
		String sz = "DOB " + pDOB->get_name() + "\t";

		if (bRoomVis)
		{
			pDOB->show();
			sz += "coming into view";
		}
		else
		{
			pDOB->hide();
			sz += "exiting view";
		}

		print_line(sz);
	}
}
*/

int LRoomManager::dob_get_room_id(int dob_id)
{
	return m_DobList.GetDob(dob_id).m_iRoomID;
}

// helpers to enable the client to manage switching on and off physics and AI
int LRoomManager::rooms_get_num_rooms() const
{
	return m_Rooms.size();
}

Vector3 LRoomManager::rooms_get_room_centre(int room_id) const
{
	const LRoom * pRoom = GetRoom(room_id);

	if (!pRoom)
		return Vector3(0, 0, 0);

	return pRoom->m_ptCentre;
}


bool LRoomManager::rooms_is_room_visible(int room_id) const
{
	if (room_id >= m_Rooms.size())
	{
		LWARN(5, "LRoomManager::rooms_is_room_visible : room id higher than number of rooms");
		return false;
	}

	return m_BF_visible_rooms.GetBit(room_id) != 0;
}

Array LRoomManager::rooms_get_visible_rooms() const
{
	Array rooms;
	for (int n=0; n<m_pCurr_VisibleRoomList->size(); n++)
	{
		rooms.push_back((*m_pCurr_VisibleRoomList)[n]);
	}

	return rooms;
}


Node * LRoomManager::rooms_get_room(int room_id)
{
	const LRoom * pRoom = GetRoom(room_id);

	if (!pRoom)
		return NULL;

	return pRoom->GetGodotRoom();
}



// move the initial hiding to where the camera is set, so we can save the scene etc
void LRoomManager::ShowAll(bool bShow)
{
	for (int n=0; n<m_SOBs.size(); n++)
	{
		LSob &sob = m_SOBs[n];
		sob.Show(bShow);
	}

	// hide all lights that are non global
	for (int n=0; n<m_Lights.size(); n++)
	{
		LLight &light = m_Lights[n];
		if (!light.m_Source.IsGlobal())
			light.Show(bShow);
	}

}


// turn on and off culling for debugging
void LRoomManager::rooms_set_active(bool bActive)
{
	if (bActive == m_bActive)
	{
		LWARN(2, "LRoomManager::rooms_set_active : noop, no state change");
		return;
	}

	CheckRoomList();

	m_bActive = bActive;

//	if (m_bActive)
//	{
		// clear these to ensure the system is initialized
		m_pCurr_VisibleRoomList->clear();
		m_pPrev_VisibleRoomList->clear();
//	}

	// show all
	for (int n=0; n<m_Rooms.size(); n++)
	{
		LRoom &lroom = m_Rooms[n];
		lroom.Debug_ShowAll(bActive);
	}


	// SOBS
	for (int n=0; n<m_SOBs.size(); n++)
	{
		LSob &sob = m_SOBs[n];
		sob.Show(!bActive);

		VisualInstance * pVI = sob.GetVI();
		if (pVI)
		{
			uint32_t mask = 0;
			if (!bActive)
			{
				mask = LRoom::LAYER_MASK_CAMERA | LRoom::LAYER_MASK_LIGHT;
			}
			LRoom::SoftShow(pVI, mask);
		}
	}

	// LIGHTS
	for (int n=0; n<m_Lights.size(); n++)
	{
		LLight &light = m_Lights[n];
//		if (!light.m_Source.IsGlobal())
		light.Show(!bActive);
	}

	m_ActiveLights.clear();
	m_ActiveLights_prev.clear();

	m_VisibleRoomList_A.clear();
	m_VisibleRoomList_B.clear();

	m_MasterList_SOBs.clear();
	m_MasterList_SOBs_prev.clear();

	m_VisibleList_SOBs.clear();
	m_CasterList_SOBs.clear();

	m_BF_ActiveLights_prev.Blank();
	m_BF_ActiveLights.Blank();
}

String LRoomManager::rooms_get_debug_frame_string()
{
	return m_szDebugString;
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


bool LRoomManager::rooms_set_camera(int dob_id, Node * pCam)
{
	CHECK_ROOM_LIST

	// is it the first setting of the camera? if so hide all
	if (m_DOB_id_camera == -1)
		ShowAll(false);


	if (!pCam)
		return false;

	Camera * pCamera = Object::cast_to<Camera>(pCam);
	if (!pCamera)
	{
		WARN_PRINT("Not a camera");
		return false;
	}


//	int id = pCam->get_instance_id();

	// was this a change in camera?
	if (m_DOB_id_camera != dob_id)
	{
		m_DOB_id_camera = dob_id;
		//m_ID_camera = id;

		// make sure the camera room is correct by doing a teleport
		//dob_teleport(pCam);
	}

	// new .. select the cull layer
	// 1 is for showing objects outside the room system
	pCamera->set_cull_mask(1 | LRoom::LAYER_MASK_CAMERA);

	// use this temporarily to force debug
//	rooms_log_frame();

	return true;
}


// convert empties and meshes to rooms and portals
bool LRoomManager::RoomsConvert(bool bVerbose, bool bDeleteLights, bool bSingleRoomMode)
{
	ResolveRoomListPath();
	CHECK_ROOM_LIST

	LRoomConverter conv;
	conv.Convert(*this, bVerbose, false, bDeleteLights, bSingleRoomMode);
	return true;
}

bool LRoomManager::rooms_single_room_convert(bool bVerbose, bool bDeleteLights)
{
	return RoomsConvert(bVerbose, bDeleteLights, true);
}


void LRoomManager::rooms_set_hide_method_detach(bool bDetach)
{
	LHidable::m_bDetach = bDetach;
}


void LRoomManager::rooms_set_portal_plane_convention(bool bFlip)
{
	m_bPortalPlane_Convention = bFlip;
}

// convert empties and meshes to rooms and portals
bool LRoomManager::rooms_convert(bool bVerbose, bool bDeleteLights)
{
	return RoomsConvert(bVerbose, bDeleteLights, false);
}

bool LRoomManager::rooms_transfer_uv2s(Node * pMeshInstance_From, Node * pMeshInstance_To)
{
	CheckRoomList();
	MeshInstance * pMI_from = Object::cast_to<MeshInstance>(pMeshInstance_From);
	MeshInstance * pMI_to = Object::cast_to<MeshInstance>(pMeshInstance_To);

	if (!pMI_from)
		return false;
	if (!pMI_to)
		return false;

	LHelper helper;
	Lawn::LDebug::m_bRunning = false;
	bool res = helper.TransferUV2(*pMI_from, *pMI_to);
	Lawn::LDebug::m_bRunning = true;
	return res;
}


bool LRoomManager::rooms_unmerge_sobs(Node * pMergeMeshInstance, float thresh_dist, float thresh_dot)
{
	MeshInstance * pMI = Object::cast_to<MeshInstance>(pMergeMeshInstance);

	LHelper helper;
	helper.SetUnMergeParams(thresh_dist, thresh_dot);
	Lawn::LDebug::m_bRunning = false;
	bool res = helper.UnMergeSOBs(*this, pMI);
	Lawn::LDebug::m_bRunning = true;
	return res;
}

bool LRoomManager::export_scene_DAE(Node * pNode, String szFilename)
{
	LDAEExporter dae;
	return dae.SaveScene(pNode, szFilename);
}

bool LRoomManager::rooms_save_scene(Node * pNode, String szFilename)
{
	LSceneSaver saver;
	return saver.SaveScene(pNode, szFilename);
}

void LRoomManager::lightmap_set_unmerge_params(float thresh_dist, float thresh_dot)
{
	// sanitize? NYI
	m_fLightmapUnMerge_ThresholdDist = thresh_dist;
	m_fLightmapUnMerge_ThresholdDot = thresh_dot;
}

bool LRoomManager::lightmap_external_unmerge(Node * pMergeMeshInstance, String szLevelFilename)
{
	ResolveRoomListPath();
	if (!CheckRoomList())
	{
		WARN_PRINT_ONCE("rooms_convert_lightmap_external : rooms not set");
		return 0;
	}

	LRoomConverter conv;
	conv.Convert(*this, true, true, false);

	bool res = rooms_unmerge_sobs(pMergeMeshInstance, m_fLightmapUnMerge_ThresholdDist, m_fLightmapUnMerge_ThresholdDot);

	if (res)
	{
		// save the UV2 mapped level
		if (szLevelFilename != "")
			rooms_save_scene(LROOMLIST, szLevelFilename);

		return true;
	}

	return false;
}


MeshInstance * LRoomManager::lightmap_internal(String szProxyFilename, String szLevelFilename)
{
	ResolveRoomListPath();
	if (!CheckRoomList())
	{
		WARN_PRINT_ONCE("rooms_convert_lightmap_internal : rooms not set");
		return 0;
	}

	LRoomConverter conv;
	conv.Convert(*this, true, true, false);

	LHelper helper;

	// as we are doing an unmerge, we want to allow the user to override the parameters of the unmerge
	helper.SetUnMergeParams(m_fLightmapUnMerge_ThresholdDist, m_fLightmapUnMerge_ThresholdDot);

	Lawn::LDebug::m_bRunning = false;
	MeshInstance * pMI = helper.CreateLightmapProxy(*this);

	// if there is a save filename for the proxy, save
	if ((szProxyFilename != "") && pMI)
	{
		LSceneSaver saver;
		saver.SaveScene(pMI, szProxyFilename);
	}

	Lawn::LDebug::m_bRunning = true;

	// save the UV2 mapped level
	if (szLevelFilename != "")
		rooms_save_scene(LROOMLIST, szLevelFilename);

	return pMI;
}

bool LRoomManager::lightmap_external_export(String szFilename) // DAE filename
{
	ResolveRoomListPath();
	CHECK_ROOM_LIST

	LRoomConverter conv;
	conv.Convert(*this, true, true, false, true);

//	LRoomConverter conv;
//	conv.Convert(*this, true, true, false);

//#m_Manager.rooms_single_room_convert(true, false)
//	rooms_single_room_convert(



//	if (!m_pRoomList)
//		return false;

	rooms_set_active(false);

	// first create a temporary mesh instance
	MeshInstance * pMerged = memnew(MeshInstance);
	pMerged->set_name("Merged");
	add_child(pMerged);


	LHelper helper;
	Lawn::LDebug::m_bRunning = false;
	bool res = helper.MergeSOBs(*this, pMerged, false);
	Lawn::LDebug::m_bRunning = true;

	// create the merged geometry
	if (res)
	{
		// save as a DAE
		LDAEExporter dae;
		dae.SetMergedMeshInstance(pMerged);

		res = dae.SaveScene(m_pRoomList, szFilename, true);
	}

	pMerged->queue_delete();

	rooms_set_active(true);
	return res;
}

bool LRoomManager::rooms_merge_sobs(Node * pMergeMeshInstance)
{
	MeshInstance * pMI = Object::cast_to<MeshInstance>(pMergeMeshInstance);


	// first make sure all SOBs are showing by deactivating LPortal
	rooms_set_active(false);

	LHelper helper;
	Lawn::LDebug::m_bRunning = false;
	bool res = helper.MergeSOBs(*this, pMI);
	Lawn::LDebug::m_bRunning = true;

	// finally reactivate LPortal
	rooms_set_active(true);

	return res;
}


// free memory for current set of rooms, prepare for converting a new game level
void LRoomManager::rooms_release()
{
	CheckRoomList();

	// unhide all the objects and reattach to scene graph
	rooms_set_active(false);

	// unregister all the dobs
//	for (int n=0; n<m_Rooms.size(); n++)
//	{
//		LRoom &lroom = m_Rooms[n];
//		lroom.Release(*this);
//	}


	ReleaseResources(false);


	m_DOB_id_camera = -1;
	//m_ID_camera = 0;
	m_ID_RoomList = 0;

	m_uiFrameCounter = 0;
//	m_iLoggingLevel = 2;
	m_bActive = true;
	m_bFrustumOnly = false;

//	m_bDebugPlanes = false;
//	m_bDebugBounds = false;
//	m_bDebugLights = false;

	m_pRoomList = 0;
}

void LRoomManager::ReleaseResources(bool bPrepareConvert)
{
	m_ShadowCasters_SOB.clear();
	m_LightCasters_SOB.clear();
	m_Rooms.clear(true);
	m_Portals.clear(true);
	m_Areas.clear(true);
	m_SOBs.clear();

	m_AreaLights.clear(true);
	m_AreaRooms.clear(true);

	if (!bPrepareConvert)
		m_Lights.clear();

	m_ActiveLights.clear();
	m_ActiveLights_prev.clear();

	m_VisibleRoomList_A.clear();
	m_VisibleRoomList_B.clear();

	m_MasterList_SOBs.clear();
	m_MasterList_SOBs_prev.clear();

	m_VisibleList_SOBs.clear();
	m_CasterList_SOBs.clear();
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

	if (m_bDebugLightVolumes)
		m_DebugLightVolumes.clear();

	if (m_bDebugFrustums)
		m_DebugFrustums.clear();

	// clear the visible room list to write to each frame
	m_pCurr_VisibleRoomList->clear();

	// keep previous
	m_BF_master_SOBs_prev.CopyFrom(m_BF_master_SOBs);
	m_BF_master_SOBs.Blank();

	// note this can be done more efficiently with swapping pointer
	m_MasterList_SOBs_prev.copy_from(m_MasterList_SOBs);

	m_VisibleList_SOBs.clear();
	m_CasterList_SOBs.clear();
	m_MasterList_SOBs.clear();

	m_BF_caster_SOBs.Blank();
	m_BF_visible_SOBs.Blank();

	// lights
	m_BF_ActiveLights_prev.CopyFrom(m_BF_ActiveLights);
	m_ActiveLights_prev.copy_from(m_ActiveLights);
	m_ActiveLights.clear();
	m_BF_ActiveLights.Blank();
	m_BF_ProcessedLights.Blank();

	// as we hit visible rooms we will mark them in a bitset, so we can hide any rooms
	// that are showing that haven't been hit this frame
	m_BF_visible_rooms.Blank();

	// reset the planes pool for another frame
	m_Pool.Reset();
}

bool LRoomManager::FrameUpdate()
{
	if (Engine::get_singleton()->is_editor_hint())
	{
		WARN_PRINT_ONCE("LRoomManager::FrameUpdate should not be called in editor");
		return false;
	}

	DebugString_Set("");

	// could turn off internal processing? not that important
	if (!m_bActive)
		return false;

	CHECK_ROOM_LIST

#ifdef LPORTAL_DOBS_AUTO_UPDATE
	DobsAutoUpdate();
#endif


	if (m_bFrustumOnly)
	{
		// debugging emulate view frustum
		FrameUpdate_FrustumOnly();
		return true;
	}

	// we keep a frame counter to prevent visiting things multiple times on the same frame in recursive functions
	m_uiFrameCounter++;
	LPRINT_RUN(5, "\nFRAME " + itos(m_uiFrameCounter));

	FrameUpdate_Prepare();


	// get the camera desired and make into lcamera
	Camera * pCamera = 0;
	if (m_DOB_id_camera == -1)
	{
		// camera not set .. do nothing
		return false;
	}

	LDob &dob = m_DobList.GetDob(m_DOB_id_camera);
	pCamera = Object::cast_to<Camera>(dob.GetSpatial());

	// camera not a camera?? shouldn't happen but we'll check
	if (!pCamera)
		return false;

	//Object *pObj = ObjectDB::get_instance(m_ID_camera);
	//pCamera = Object::cast_to<Camera>(pObj);

	// always doing dob update here for camera, this ensures it is not one frame behind
	// depending on scene tree, which can cause camera lroom id to be the old one after crossing
	// a portal plane, causing a flicker on changing room...
	dob_update(m_DOB_id_camera, pCamera->get_global_transform().origin);

	//dob_update(pCamera);


	// Which room is the camera currently in?
	LRoom * pRoom = GetRoom(dob.m_iRoomID);

	if (!pRoom)
	{
		WARN_PRINT_ONCE("LRoomManager::FrameUpdate : Camera is not in an LRoom");
		return false;
	}

#ifdef LDEBUG_CAMERA
	if (m_bDebugFrameString)
		DebugString_Add("Camera in room " + itos(pRoom->m_RoomID) + "\n");
#endif


	// lcamera contains the info needed for running the recursive trace using the main camera
	LSource cam; cam.Source_SetDefaults();
	cam.m_ptPos = Vector3(0, 0, 0);
	cam.m_ptDir = Vector3 (-1, 0, 0);

	// get the camera desired and make into lcamera
	assert (pCamera);
	Transform tr = pCamera->get_global_transform();
	cam.m_ptPos = tr.origin;
	cam.m_ptDir = -tr.basis.get_axis(2); // or possibly get_axis .. z is what we want

	// if we can't prepare the frustum is invalid
	if (!m_MainCamera.Prepare(*this, pCamera))
		return false;

	// the first set of planes are allocated and filled with the view frustum planes
	// Note that the visual server doesn't actually need to do view frustum culling as a result...
	// (but is still doing it for now)
	unsigned int pool_member = m_Pool.Request();
	assert (pool_member != (unsigned int) -1);

	LVector<Plane> &planes = m_Pool.Get(pool_member);
	planes.clear();

	// luckily godot already has a function to return a list of the camera clipping planes
	planes.copy_from(m_MainCamera.m_Planes);

	// the whole visibility algorithm is recursive, spreading out from the camera room,
	// rendering through any portals in view into other rooms, etc etc
	m_Trace.Trace_Prepare(*this, cam, m_BF_visible_SOBs, m_BF_visible_rooms, m_VisibleList_SOBs, *m_pCurr_VisibleRoomList);
	m_Trace.Trace_Begin(*pRoom, planes);

	// we no longer need these planes
	m_Pool.Free(pool_member);

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

	return true;
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

#ifdef LDEBUG_LIGHTS
	if (m_bDebugFrameString)
		DebugString_Add("TOTAL shadow casters " + itos(m_CasterList_SOBs.size()) + "\n");
#endif

	LPRINT_RUN(2, "TOTAL shadow casters " + itos(m_CasterList_SOBs.size()));
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
			bool bCaster = m_BF_caster_SOBs.GetBit(ID) != 0;

			uint32_t flags = 0;
			if (bVisible) flags |= LRoom::LAYER_MASK_CAMERA;
			if (bCaster) flags |= LRoom::LAYER_MASK_LIGHT;

			LRoom::SoftShow(pVI, flags);
		}
	}


#ifdef LDEBUG_LIGHTS
	if (m_bDebugFrameString)
		DebugString_Add("nActiveLights " + itos(m_ActiveLights.size()) + "\n");
#endif

	// lights
	for (int n=0; n<m_ActiveLights.size(); n++)
	{
		int lid = m_ActiveLights[n];

		if (!m_BF_ActiveLights_prev.GetBit(lid))
		{
			LLight &light = m_Lights[lid];
			light.Show(true);

			Light * pLight = light.GetGodotLight();
			if (pLight)
			{
				//Lawn::LDebug::print("Showing light " + itos (n));
				//pLight->show();

				// FIX GODOT BUG - light cull mask is not preserved when hiding and showing
				// set culling flag for light
				// 1 is for lighting objects outside the room system
				//pLight->set_shadow(false);
				//pLight->set_shadow(true);
				//pLight->set_cull_mask(1 | LRoom::LAYER_MASK_LIGHT);


//				Vector3 ptBugFix = pLight->get_translation();
//				pLight->set_translation(ptBugFix);
			}
		}

		// debug
		DebugString_Light_AffectedRooms(lid);
	}
	for (int n=0; n<m_ActiveLights_prev.size(); n++)
	{
		int lid = m_ActiveLights_prev[n];
		if (!m_BF_ActiveLights.GetBit(lid))
		{
			LLight &light = m_Lights[lid];
			light.Show(false);
//			Light * pLight = light.GetGodotLight();
//			if (pLight)
//			{
//				//Lawn::LDebug::print("Hiding light " + itos (n));
//				pLight->hide();
//			}
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

		// show / hide is relatively expensive because of propagating messages between nodes ...
		// should be minimized
		sob.Show(true);
	}

}


void LRoomManager::FrameUpdate_DrawDebug(const LSource &cam, const LRoom &lroom)
{
	// light portal planes
	if (m_bDebugLights)
	{
		Object * pObj = ObjectDB::get_instance(m_ID_DebugLights);
		ImmediateGeometry * im = Object::cast_to<ImmediateGeometry>(pObj);
		if (!im)
			return;

		im->clear();

		im->begin(Mesh::PRIMITIVE_TRIANGLES, NULL);

		int nVerts = m_DebugPortalLightPlanes.size();

		for (int n=0; n<nVerts; n++)
		{
			im->add_vertex(m_DebugPortalLightPlanes[n]);
		}
		im->end();
	}

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

	if (m_bDebugLightVolumes)
	{
		Object * pObj = ObjectDB::get_instance(m_ID_DebugLightVolumes);
		ImmediateGeometry * im = Object::cast_to<ImmediateGeometry>(pObj);
		if (!im)
			return;

		im->clear();

		im->begin(Mesh::PRIMITIVE_LINES, NULL);

		int nVerts = m_DebugLightVolumes.size();

		for (int n=0; n<nVerts; n++)
		{
			im->add_vertex(m_DebugLightVolumes[n]);
		}
		im->end();
	}

	if (m_bDebugFrustums)
	{
		Object * pObj = ObjectDB::get_instance(m_ID_DebugFrustums);
		ImmediateGeometry * im = Object::cast_to<ImmediateGeometry>(pObj);
		if (!im)
			return;

		im->clear();

		im->begin(Mesh::PRIMITIVE_LINES, NULL);

		int nVerts = m_DebugFrustums.size();

		for (int n=0; n<nVerts; n++)
		{
			im->add_vertex(m_DebugFrustums[n]);
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
				//CreateDebug();
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
	ClassDB::bind_method(D_METHOD("rooms_convert", "verbose", "delete lights"), &LRoomManager::rooms_convert);
	ClassDB::bind_method(D_METHOD("rooms_single_room_convert", "verbose", "delete lights"), &LRoomManager::rooms_single_room_convert);
	ClassDB::bind_method(D_METHOD("rooms_set_portal_plane_convention", "flip"), &LRoomManager::rooms_set_portal_plane_convention);

	ClassDB::bind_method(D_METHOD("rooms_set_hide_method_detach", "detach"), &LRoomManager::rooms_set_hide_method_detach);

	ClassDB::bind_method(D_METHOD("rooms_release"), &LRoomManager::rooms_release);

	ClassDB::bind_method(D_METHOD("rooms_set_camera", "camera"), &LRoomManager::rooms_set_camera);

	ClassDB::bind_method(D_METHOD("rooms_save_scene", "node", "filename"), &LRoomManager::rooms_save_scene);

	// stuff for external workflow .. works but don't expose yet
	//ClassDB::bind_method(D_METHOD("rooms_merge_sobs"), &LRoomManager::rooms_merge_sobs);
	//ClassDB::bind_method(D_METHOD("rooms_unmerge_sobs"), &LRoomManager::rooms_unmerge_sobs);
	//ClassDB::bind_method(D_METHOD("rooms_transfer_uv2s"), &LRoomManager::rooms_transfer_uv2s);

	ClassDB::bind_method(D_METHOD("lightmap_external_export", "DAE filename"), &LRoomManager::lightmap_external_export);
	ClassDB::bind_method(D_METHOD("lightmap_external_unmerge", "merged mesh instance", "output level filename"), &LRoomManager::lightmap_external_unmerge);
	ClassDB::bind_method(D_METHOD("lightmap_set_unmerge_params", "threshold distance (e.g. 0.001)", "threshold dot (e.g. 0.99)"), &LRoomManager::lightmap_set_unmerge_params);


	// lightmapping
	ClassDB::bind_method(D_METHOD("lightmap_internal", "output proxy filename", "output level filename"), &LRoomManager::lightmap_internal);

	// debugging
	ClassDB::bind_method(D_METHOD("rooms_set_logging", "level 0 to 6"), &LRoomManager::rooms_set_logging);
	ClassDB::bind_method(D_METHOD("rooms_log_frame"), &LRoomManager::rooms_log_frame);
	ClassDB::bind_method(D_METHOD("rooms_set_active", "active"), &LRoomManager::rooms_set_active);
	ClassDB::bind_method(D_METHOD("rooms_set_debug_planes", "active"), &LRoomManager::rooms_set_debug_planes);
	ClassDB::bind_method(D_METHOD("rooms_set_debug_bounds", "active"), &LRoomManager::rooms_set_debug_bounds);
	ClassDB::bind_method(D_METHOD("rooms_set_debug_lights", "active"), &LRoomManager::rooms_set_debug_lights);
	ClassDB::bind_method(D_METHOD("rooms_set_debug_shadows", "active"), &LRoomManager::rooms_set_debug_shadows);
	ClassDB::bind_method(D_METHOD("rooms_set_debug_frustums", "active"), &LRoomManager::rooms_set_debug_frustums);

	ClassDB::bind_method(D_METHOD("rooms_set_debug_frame_string", "active"), &LRoomManager::rooms_set_debug_frame_string);
	ClassDB::bind_method(D_METHOD("rooms_get_debug_frame_string"), &LRoomManager::rooms_get_debug_frame_string);

	ClassDB::bind_method(D_METHOD("rooms_get_room_centre", "room_id"), &LRoomManager::rooms_get_room_centre);

	// functions to add dynamic objects to the culling system
	// Note that these should not be placed directly in rooms, the system will 'soft link' to them
	// so they can be held, e.g. in pools elsewhere in the scene graph
	ClassDB::bind_method(D_METHOD("dob_register", "node", "pos", "radius"), &LRoomManager::dob_register);
	ClassDB::bind_method(D_METHOD("dob_unregister", "dob_id"), &LRoomManager::dob_unregister);
	ClassDB::bind_method(D_METHOD("dob_update", "dob_id", "pos"), &LRoomManager::dob_update);
//	ClassDB::bind_method(D_METHOD("dob_teleport", "dob"), &LRoomManager::dob_teleport);

//	ClassDB::bind_method(D_METHOD("dob_register_hint", "dob", "radius", "room"), &LRoomManager::dob_register_hint);
//	ClassDB::bind_method(D_METHOD("dob_teleport_hint", "dob", "room"), &LRoomManager::dob_teleport_hint);

	ClassDB::bind_method(D_METHOD("dob_get_room_id", "dob"), &LRoomManager::dob_get_room_id);


	ClassDB::bind_method(D_METHOD("global_light_register", "light", "area"), &LRoomManager::global_light_register);

	ClassDB::bind_method(D_METHOD("dynamic_light_register", "light", "radius"), &LRoomManager::dynamic_light_register);
//	ClassDB::bind_method(D_METHOD("dynamic_light_register_hint", "light", "radius", "room"), &LRoomManager::dynamic_light_register_hint);
	ClassDB::bind_method(D_METHOD("dynamic_light_unregister", "light"), &LRoomManager::dynamic_light_unregister);
	ClassDB::bind_method(D_METHOD("dynamic_light_update", "light"), &LRoomManager::dynamic_light_update);

	// helper
	ClassDB::bind_method(D_METHOD("rooms_get_room", "room id"), &LRoomManager::rooms_get_room);
	ClassDB::bind_method(D_METHOD("rooms_get_num_rooms"), &LRoomManager::rooms_get_num_rooms);
	ClassDB::bind_method(D_METHOD("rooms_is_room_visible", "room id"), &LRoomManager::rooms_is_room_visible);
	ClassDB::bind_method(D_METHOD("rooms_get_visible_rooms"), &LRoomManager::rooms_get_visible_rooms);


	ClassDB::bind_method(D_METHOD("set_rooms", "rooms"), &LRoomManager::set_rooms);
	ClassDB::bind_method(D_METHOD("set_rooms_path", "rooms"), &LRoomManager::set_rooms_path);
	ClassDB::bind_method(D_METHOD("get_rooms_path"), &LRoomManager::get_rooms_path);

	ClassDB::bind_method(D_METHOD("export_scene_DAE", "node", "filename"), &LRoomManager::export_scene_DAE);


	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "rooms"), "set_rooms_path", "get_rooms_path");
}

//////////////////////////////

void LRoomManager::_set_rooms(Object *p_rooms)
{
		if (p_rooms)
		{
			LPRINT(2, "\t_set_rooms");
		}
		else
		{
			LPRINT(2, "\t_set_rooms NULL");
		}

		m_ID_RoomList = 0;
		m_pRoomList = 0;

		if (p_rooms)
		{
			// check is spatial
			Spatial *pSpatial = Object::cast_to<Spatial>(p_rooms);

			if (pSpatial)
			{
				m_ID_RoomList = pSpatial->get_instance_id();
				m_pRoomList = pSpatial;
				LPRINT(2, "\t\tRoomlist was Godot ID " + itos(m_ID_RoomList));
			}
			else
			{
				// should never get here?
				WARN_PRINT("_set_rooms must be a Spatial");
			}
		}

}


void LRoomManager::set_rooms(const Object *p_rooms)
{
	// handle null
	if (p_rooms == NULL)
	{
		WARN_PRINT("SCRIPT set_rooms NULL");
		remove_rooms_path();
		return;
	}
	LPRINT(2, "SCRIPT set_rooms");

	// is it a spatial?
	const Spatial * pSpatial = Object::cast_to<Spatial>(p_rooms);

	if (!pSpatial)
	{
		WARN_PRINT("set_rooms must be a spatial.");
		remove_rooms_path();
		return;
	}

	NodePath path = get_path_to(pSpatial);
	set_rooms_path(path);
}


void LRoomManager::set_rooms_path(const NodePath &p_path)
{
	LPRINT(2, "set_rooms_path " + p_path);
	m_path_RoomList = p_path;
	ResolveRoomListPath();
}

NodePath LRoomManager::get_rooms_path() const
{
	return m_path_RoomList;
}

void LRoomManager::remove_rooms_path()
{
	NodePath pathnull;
	set_rooms_path(pathnull);
}


Spatial * LRoomManager::GetRoomList_Checked()
{
	if (m_ID_RoomList == 0)
		return 0;

	m_pRoomList = (Spatial *) ObjectDB::get_instance(m_ID_RoomList);

	if (!m_pRoomList)
		m_ID_RoomList = 0; // the node is no longer valid

	return m_pRoomList;
}


void LRoomManager::ResolveRoomListPath()
{
	LPRINT(2, "ResolveRoomListPath " + m_path_RoomList);

	if (has_node(m_path_RoomList))
	{
		LPRINT(2, "has_node");
		Spatial * pNode = Object::cast_to<Spatial>(get_node(m_path_RoomList));
		if (pNode)
		{
			_set_rooms(pNode);
			return;
		}
		else
		{
			WARN_PRINT("RoomList must be a Spatial");
		}
	}

	// default to setting to null
	_set_rooms(NULL);
}

/////////////////////////////////////////////////////////////////////////////////
// rooms_set_debug commands .. done as macros to make easier to change

// macros to combine to single identifier
//#define SCU_IDENT(x) x
#define COMB_NX(A, B) A##B
#define COMB_IDENT(A, B) COMB_NX(A,B)
#define IMPLEMENT_DEBUG_MESH(a, b) void LRoomManager::COMB_IDENT(rooms_set_debug_, a)(bool bActive)\
{\
COMB_IDENT(m_bDebug, b) = bActive;\
Object * pObj = ObjectDB::get_instance(COMB_IDENT(m_ID_Debug, b)); \
ImmediateGeometry * im = Object::cast_to<ImmediateGeometry>(pObj); \
if (!im) \
return; \
if (bActive) \
im->show(); \
else \
im->hide(); \
}

IMPLEMENT_DEBUG_MESH(shadows,LightVolumes)
IMPLEMENT_DEBUG_MESH(frustums,Frustums)
IMPLEMENT_DEBUG_MESH(lights,Lights)
IMPLEMENT_DEBUG_MESH(bounds,Bounds)
IMPLEMENT_DEBUG_MESH(planes,Planes)

#undef COMB_NX
#undef COMB_IDENT
#undef IMPLEMENT_DEBUG_MESH

void LRoomManager::rooms_set_debug_frame_string(bool bActive)
{
	m_bDebugFrameString = bActive;
}



//void LRoomManager::DobsAutoUpdate()
//{
//	// go through each room, each dob
//	for (int n=0; n<m_Rooms.size(); n++)
//	{
//		LRoom &lroom = m_Rooms[n];
//		int iRoomNum = lroom.m_RoomID;


//		for (int d=0; d<lroom.m_DOBs.size(); d++)
//		{
//			LDob &dob = lroom.m_DOBs[d];
//			Spatial * pDOB_Spatial = dob.GetSpatial();

//			if (!pDOB_Spatial)
//				continue;

//			int iNewRoom = DobUpdate(pDOB_Spatial, &lroom);

//			// changed?
//			if (iNewRoom != iRoomNum)
//			{
//				// very inefficient but will do for testing
//				DobsAutoUpdate();
//				return;
//			}
//		}
//	}
//}
