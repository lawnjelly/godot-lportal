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

#include "ldoblist.h"
#include "lroom.h"
#include "lportal.h"
#include "larea.h"
#include "ltrace.h"
#include "lmain_camera.h"

class LRoomManager : public Spatial {
	GDCLASS(LRoomManager, Spatial);

	friend class LRoom;
	friend class LRoomConverter;
	friend class LHelper;
	friend class LTrace;
	friend class LMainCamera;
	friend class LDobList;

public:
	// PUBLIC INTERFACE TO GDSCRIPT
	//______________________________________________________________________________________
	// Roomlist path
	void set_rooms(const Object *p_rooms);
	void _set_rooms(Object *p_rooms);
	void set_rooms_path(const NodePath &p_path);
	NodePath get_rooms_path() const;
	void remove_rooms_path();

	//______________________________________________________________________________________
	// MAIN
	// convert empties and meshes to rooms and portals
	bool rooms_convert(bool bVerbose, bool bDeleteLights);
	bool rooms_single_room_convert(bool bVerbose, bool bDeleteLights);
	// free memory for current set of rooms, prepare for converting a new game level
	void rooms_release();

	// choose which camera you want to use to determine visibility.
	// normally this will be your main camera, but you can choose another for debugging
	bool rooms_set_camera(int dob_id, Node * pCam);

	// get the Godot room that is associated with an LPortal room
	// (can be used to find the name etc of a room ID returned by dob_update)
	Node * rooms_get_room(int room_id);

	// CONVENTIONS
	void rooms_set_portal_plane_convention(bool bFlip);
	void rooms_set_hide_method_detach(bool bDetach);

	//______________________________________________________________________________________
	// DOBS
	// Dynamic objects .. cameras, players, boxes etc
	// These are defined by their ability to move from room to room.
	// You can still move static objects within the same room (e.g. elevators, moving platforms)
	// as these don't require checks for changing rooms.

	// returns DOB ID
	int dob_register(Node * pDOB, const Vector3 &pos, float radius);
	bool dob_unregister(int dob_id);
	// returns the room ID within
	int dob_update(int dob_id, const Vector3 &pos);

	//______________________________________________________________________________________
	// LIGHTS
	// global directional lights that will apply to all rooms
	bool global_light_register(Node * pLightNode, String szArea);

	// dynamic lights (spot or omni within rooms)
	// returns light ID
	// only lights within the rooms on conversion are supported so far
	int dynamic_light_register(Node * pLightNode, float radius);
	bool dynamic_light_unregister(int light_id);
	int dynamic_light_update(int light_id, const Vector3 &pos, const Vector3 &dir); // returns room within

	//______________________________________________________________________________________
	// LIGHTMAPS
	// helper function to merge SOB meshes for producing lightmaps VIA external blender workflow
	bool rooms_merge_sobs(Node * pMergeMeshInstance);
	bool rooms_unmerge_sobs(Node * pMergeMeshInstance, float thresh_dist, float thresh_dot);
	bool rooms_transfer_uv2s(Node * pMeshInstance_From, Node * pMeshInstance_To);

	bool lightmap_external_export(String szFilename); // DAE filename
	bool lightmap_external_unmerge(Node * pMergeMeshInstance, String szLevelFilename);
	void lightmap_set_unmerge_params(float thresh_dist, float thresh_dot);

	// one function to do all the uv mapping and lightmap creation in one
	// (for godot lightmap workflow)
	MeshInstance * lightmap_internal(String szProxyFilename, String szLevelFilename);

	//______________________________________________________________________________________
	// HELPERS
	// helper function for general use .. LPortal has the functionality, why not...
	bool rooms_save_scene(Node * pNode, String szFilename);
	// helpers to enable the client to manage switching on and off physics and AI
	int rooms_get_num_rooms() const;
	bool rooms_is_room_visible(int room_id) const;
	Array rooms_get_visible_rooms() const;
	// helper func, not needed usually as dob_update returns the room
	int dob_get_room_id(int dob_id);
	bool export_scene_DAE(Node * pNode, String szFilename);


	//______________________________________________________________________________________
	// DEBUGGING
	// turn on and off culling for debugging
	void rooms_set_active(bool bActive);
	void rooms_set_debug_planes(bool bActive);
	void rooms_set_debug_bounds(bool bActive);
	void rooms_set_debug_lights(bool bActive);
	void rooms_set_debug_shadows(bool bActive);
	void rooms_set_debug_frustums(bool bActive);
	void rooms_set_debug_frame_string(bool bActive);

	// rough .. for debugging
	Vector3 rooms_get_room_centre(int room_id) const;

	// 0 to 6 .. less to more
	// defaults to 4 which is (2) in our priorities (i.e. 6 - level)
	void rooms_set_logging(int level);

	// optionally lportal can output some debug info in a string each frame
	String rooms_get_debug_frame_string();

	// provide debugging output on the next frame
	void rooms_log_frame();

private:
	// PER FRAME STUFF

	// camera
	int m_DOB_id_camera;

	// keep track of which rooms are visible, so we can hide ones that aren't hit that were previously on
	Lawn::LBitField_Dynamic m_BF_visible_rooms;


	// the render list is all objects that are in view,
	// and also objects out of view but casting shadows INTO the view
	LVector<int> m_VisibleList_SOBs;
	LVector<int> m_CasterList_SOBs;

	LVector<int> m_MasterList_SOBs;


	Lawn::LBitField_Dynamic m_BF_visible_SOBs;
	Lawn::LBitField_Dynamic m_BF_caster_SOBs;
	Lawn::LBitField_Dynamic m_BF_master_SOBs;

	// previous frame
	LVector<int> m_MasterList_SOBs_prev;
	Lawn::LBitField_Dynamic m_BF_master_SOBs_prev;


	LVector<int> m_VisibleRoomList_A;
	LVector<int> m_VisibleRoomList_B;

	LVector<int> * m_pCurr_VisibleRoomList;
	LVector<int> * m_pPrev_VisibleRoomList;

	// active lights
	LVector<int> m_ActiveLights;
	LVector<int> m_ActiveLights_prev;
	Lawn::LBitField_Dynamic m_BF_ActiveLights;
	Lawn::LBitField_Dynamic m_BF_ActiveLights_prev;

	// some lights may be processed on a frame but found not to intersect the view frustum
	Lawn::LBitField_Dynamic m_BF_ProcessedLights;

	// keep all the light rendering stuff together
	struct LLightRender
	{
		// each time we render from a light point of view, we reuse this list to store each caster ID
		Lawn::LBitField_Dynamic m_BF_Temp_SOBs;
		Lawn::LBitField_Dynamic m_BF_Temp_Visible_Rooms;
		LVector<int> m_Temp_Visible_SOBs;
		LVector<int> m_Temp_Visible_Rooms;
	} m_LightRender;


	// keep a frame counter, to mark when objects have been hit by the visiblity algorithm
	// already to prevent multiple hits on rooms and objects
	unsigned int m_uiFrameCounter;

	// the portal plane determined by the artwork geometry for room conversion can either point in or out,
	// this convention is switchable
	bool m_bPortalPlane_Convention;

private:
	// lists of rooms and portals, contiguous list so cache friendly
	LVector<LRoom> m_Rooms;
	LVector<LPortal> m_Portals;
	LVector<LArea> m_Areas;

	// static objects
	LVector<LSob> m_SOBs;

	// lights
	LVector<LLight> m_Lights;

	// SHADOWS
	// master list of shadow casters for each room
	LVector<uint32_t> m_ShadowCasters_SOB; // not used any more?

	// master list of casters for each light (precalculated list)
	LVector<uint32_t> m_LightCasters_SOB;

	// AREAS
	// master list of lights affecting each area
	LVector<uint32_t> m_AreaLights;

	// master list of rooms in each area
	LVector<uint32_t> m_AreaRooms;

	// The recursive visibility function needs to allocate loads of planes.
	// We use a pool for this instead of allocating on the fly.
	LPlanesPool m_Pool;

	LDobList m_DobList;

public:
	// whether debug planes is switched on
	bool m_bDebugPlanes;
	bool m_bDebugBounds;
	bool m_bDebugLights;
	bool m_bDebugLightVolumes;
	bool m_bDebugFrustums;

	// the planes are shown as a list of lines from the camera to the portal verts
	LVector<Vector3> m_DebugPlanes;
	LVector<Vector3> m_DebugPortalLightPlanes;
	LVector<Vector3> m_DebugLightVolumes;
	LVector<Vector3> m_DebugFrustums;

	// we are now referencing the rooms indirectly via a nodepath rather than directly being children
	// of the LRoomManager node
	NodePath m_path_RoomList;
	ObjectID m_ID_RoomList;


private:
	LTrace m_Trace;
	// unchecked
	Spatial * m_pRoomList;
	LMainCamera m_MainCamera;

	// DEBUGGING
	// 0 to 5
	int m_iLoggingLevel;

	ObjectID m_ID_DebugPlanes;
	ObjectID m_ID_DebugBounds;
	ObjectID m_ID_DebugLights;
	ObjectID m_ID_DebugLightVolumes;
	ObjectID m_ID_DebugFrustums;

	Ref<SpatialMaterial> m_mat_Debug_Planes;
	Ref<SpatialMaterial> m_mat_Debug_Bounds;
	Ref<SpatialMaterial> m_mat_Debug_LightVolumes;
	Ref<SpatialMaterial> m_mat_Debug_Frustums;

	String m_szDebugString;
	bool m_bDebugFrameString;

	// for debugging, can turn LPortal on and off
	bool m_bActive;

	// for debugging, can emulate view frustum culling
	bool m_bFrustumOnly;

	// lightmap unmerge params
	float m_fLightmapUnMerge_ThresholdDist;
	float m_fLightmapUnMerge_ThresholdDot;

private:
	// PRIVATE FUNCS
	// this is where we do all the culling
	bool FrameUpdate();
	void FrameUpdate_Prepare();
	void FrameUpdate_FinalizeRooms();
	void FrameUpdate_AddShadowCasters();
	void FrameUpdate_CreateMasterList();
	void FrameUpdate_FinalizeVisibility_WithinRooms();
	void FrameUpdate_FinalizeVisibility_SoftShow();

	// debugging emulate view frustum
	void FrameUpdate_FrustumOnly();

	// draw planes and room hulls
	void FrameUpdate_DrawDebug(const LSource &cam, const LRoom &lroom);

	// internal
	bool RoomsConvert(bool bVerbose, bool bDeleteLights, bool bSingleRoomMode);

	// dobs
	int DobRegister(Spatial * pDOB, const Vector3 &pos, float radius, int iRoom);

	ObjectID DobRegister_FindVIRecursive(Node * pNode) const;
//	bool DobTeleport(Spatial * pDOB, int iNewRoomID);

	//int DobUpdate(Spatial * pDOB_Spatial, LRoom * pRoom);
	void DobUpdateVisibility(int dob_id);

	// for debugging only have autoupdate mode where all dobs are updated
//	void DobsAutoUpdate();

	void CreateDebug();
	void ReleaseResources(bool bPrepareConvert);
	void ShowAll(bool bShow);
	void ResolveRoomListPath();

	// frame debug string
	void DebugString_Set(String sz) {m_szDebugString = sz;}
	void DebugString_Add(String sz) {m_szDebugString += sz;}
	void DebugString_Light_AffectedRooms(int light_id);

	// now we are centralizing the tracing out from static and dynamic lights for each frame to this function
	bool LightCreate(Light * pLight, int roomID, String szArea = "");
	void Light_UpdateTransform(LLight &light, const Light &glight) const;
	void Light_FrameProcess(int lightID);
	bool Light_FindCasters(int lightID);


	// helper funcs
	const LRoom * GetRoom(int i) const;
	LRoom * GetRoom(int i);

	int FindClosestRoom(const Vector3 &pt) const;

	LRoom &Portal_GetLinkedRoom(const LPortal &port);

	// for DOBs, we need some way of storing the room ID on them, so we use metadata (currently)
	// this is pretty gross but hey ho
//	int Meta_GetRoomNum(Node * pNode) const;
//	void Meta_SetRoomNum(Node * pNode, int num);

	// for lights we store the light ID in the metadata
	void Meta_SetLightID(Node * pNode, int id);
	int Meta_GetLightID(Node * pNode) const;

public:
	// makes sure m_pRoomList is up to date and valid
	bool CheckRoomList() {return GetRoomList_Checked() != 0;}

	Spatial * GetRoomList_Checked();
	// unchecked, be sure to call checked version first which will set m_pRoomList
	Spatial * GetRoomList() const {return m_pRoomList;}

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	LRoomManager();
};

#endif
