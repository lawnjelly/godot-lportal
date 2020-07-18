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

#include "ltrace.h"
#include "ldebug.h"
#include "ldob.h"
#include "lportal.h"
#include "lbitfield_dynamic.h"
#include "lroom_manager.h"

#define LMAN m_pManager



//void LTrace::Trace_Prepare(LRoomManager &manager, const LCamera &cam, Lawn::LBitField_Dynamic &BF_SOBs, Lawn::LBitField_Dynamic &BF_DOBs, Lawn::LBitField_Dynamic &BF_Rooms, LVector<int> &visible_SOBs, LVector<int> &visible_DOBs, LVector<int> &visible_Rooms)
void LTrace::Trace_Prepare(LRoomManager &manager, const LSource &cam, Lawn::LBitField_Dynamic &BF_SOBs, Lawn::LBitField_Dynamic &BF_Rooms, LVector<int> &visible_SOBs, LVector<int> &visible_Rooms)
{
	m_pManager = &manager;
	m_pCamera = &cam;

	// default
	m_TraceFlags = CULL_SOBS | CULL_DOBS | TOUCH_ROOMS | MAKE_ROOM_VISIBLE;

	m_pBF_SOBs = &BF_SOBs;
//	m_pBF_DOBs = &BF_DOBs;
	m_pBF_Rooms = &BF_Rooms;
	m_pVisible_SOBs = &visible_SOBs;
//	m_pVisible_DOBs = &visible_DOBs;
	m_pVisible_Rooms = &visible_Rooms;
}

void LTrace::CullSOBs(LRoom &room, const LVector<Plane> &planes)
{
	// clip all objects in this room to the clipping planes
	int last_sob = room.m_iFirstSOB + room.m_iNumSOBs;
	for (int n=room.m_iFirstSOB; n<last_sob; n++)
	{
		LSob &sob = LMAN->m_SOBs[n];

		//LPRINT_RUN(2, "sob " + itos(n) + " " + sob.GetSpatial()->get_name());

		// already determined to be visible through another portal
		if (m_pBF_SOBs->GetBit(n))
		{
			//LPRINT_RUN(2, "\talready visible");
			continue;
		}

		bool bShow = true;


		// estimate the radius .. for now
		const AABB &bb = sob.m_aabb;

//		print("\t\t\tculling object " + pObj->get_name());

		for (int p=0; p<planes.size(); p++)
		{
//				float dist = planes[p].distance_to(pt);
//				print("\t\t\t\t" + itos(p) + " : dist " + String(Variant(dist)));

			float r_min, r_max;
			bb.project_range_in_plane(planes[p], r_min, r_max);

	//		print("\t\t\t\t" + itos(p) + " : r_min " + String(Variant(r_min)) + ", r_max " + String(Variant(r_max)));


			if (r_min > 0.0f)
			{
				//LPRINT_RUN(2, "\tout of view");
				bShow = false;
				break;
			}
		}

		if (bShow)
		{
			// sob is renderable and visible (not shadow only)
			//LPRINT_RUN(2, "\tin view");
			m_pBF_SOBs->SetBit(n, true);
			m_pVisible_SOBs->push_back(n);
		}

	} // for through sobs

}

void LTrace::CullDOBs(LRoom &room, const LVector<Plane> &planes)
{
	// NYI this isn't efficient, there may be more than 1 portal to the same room
/*
	// cull DOBs
	int nDOBs = room.m_DOBs.size();

	for (int n=0; n<nDOBs; n++)
	{
		LDob &dob = room.m_DOBs[n];

		Spatial * pObj = dob.GetSpatial();

		if (pObj)
		{
			bool bShow = true;
			const Vector3 &pt = pObj->get_global_transform().origin;

			float radius = dob.m_fRadius;

			for (int p=0; p<planes.size(); p++)
			{
				float dist = planes[p].distance_to(pt);
				//print("\t\t\t\t" + itos(p) + " : dist " + String(Variant(dist)));

				if (dist > radius)
				{
					bShow = false;
					break;
				}
			}

			if (bShow)
			{
				LPRINT_RUN(1, "\tDOB " + pObj->get_name() + " visible");
				dob.m_bVisible = true;
			}
			else
			{
				LPRINT_RUN(1, "\tDOB " + pObj->get_name() + " culled");
			}
		}
	} // for through dobs
*/
}


bool LTrace::Trace_Light(LRoomManager &manager, const LLight &light, eLightRun eRun)
{
	m_pManager = &manager;


	LRoom * pRoom;

	// non area light
	if (light.m_iArea == -1)
	{
		// can only deal with lights in rooms for now
		if (light.m_Source.m_RoomID == -1)
		{
			WARN_PRINT_ONCE("LTrace::Trace_Light can only trace lights in rooms");
			return true;
		}

		pRoom = manager.GetRoom(light.m_Source.m_RoomID);
		if (!pRoom)
			return true;
	}
	else
	{
		// area light
		pRoom = 0;
	}

	const LSource &cam = light.m_Source;

	unsigned int pool_member = manager.m_Pool.Request();
	assert (pool_member != (unsigned int) -1);

	LVector<Plane> &planes = manager.m_Pool.Get(pool_member);
	planes.clear();

	// we now need to trace either just DOBs (in the case of static lights)
	// or SOBs and DOBs (in the case of dynamic lights)
	LRoomManager::LLightRender &lr = manager.m_LightRender;
	lr.m_BF_Temp_SOBs.Blank();
	lr.m_Temp_Visible_SOBs.clear();
	lr.m_BF_Temp_Visible_Rooms.Blank();
	lr.m_Temp_Visible_Rooms.clear();

	bool bLightInView = true;

	switch (eRun)
	{
	// finding all shadow casters at runtime
	case LR_ALL:
		{
			//Trace_Prepare(manager, cam, lr.m_BF_Temp_SOBs, manager.m_BF_visible_rooms, lr.m_Temp_Visible_SOBs, *manager.m_pCurr_VisibleRoomList);
			Trace_Prepare(manager, cam, lr.m_BF_Temp_SOBs, lr.m_BF_Temp_Visible_Rooms, lr.m_Temp_Visible_SOBs, lr.m_Temp_Visible_Rooms);

			Trace_SetFlags(CULL_SOBS | CULL_DOBS | MAKE_ROOM_VISIBLE);

			// create subset planes of light frustum and camera frustum
			bLightInView = manager.m_MainCamera.AddCameraLightPlanes(manager, cam, planes);
		}
		break;
	// finding only visible rooms at runtime
	case LR_ROOMS:
		{
			Trace_Prepare(manager, cam, lr.m_BF_Temp_SOBs, lr.m_BF_Temp_Visible_Rooms, lr.m_Temp_Visible_SOBs, lr.m_Temp_Visible_Rooms);

			// we ONLY want a list of rooms hit
			Trace_SetFlags(MAKE_ROOM_VISIBLE);
		}
		break;
	// finding all in preconversion
	case LR_CONVERT:
		{
			Trace_Prepare(manager, cam, lr.m_BF_Temp_SOBs, lr.m_BF_Temp_Visible_Rooms, lr.m_Temp_Visible_SOBs, lr.m_Temp_Visible_Rooms);

			// we want sobs but not to touch rooms
			m_TraceFlags = CULL_SOBS | MAKE_ROOM_VISIBLE; //  | CULL_DOBS | TOUCH_ROOMS;
		}
		break;
	}


	if (bLightInView)
	{
		// non area light
		if (pRoom)
		{
			Trace_Begin(*pRoom, planes);
		}
		else
		{
			// area light

			// area lights don't go through portals, e.g. coming from above like sunlight
			// they instead have a predefined list of rooms governed by the area
			m_TraceFlags |= DONT_TRACE_PORTALS;

			// new .. trace according to area, not affected rooms, as affected rooms has a limit
			assert (light.m_iArea != -1);
			const LArea &area = LMAN->m_Areas[light.m_iArea];

			int last_room = area.m_iFirstRoom + area.m_iNumRooms;

			for (int r=area.m_iFirstRoom; r<last_room; r++)
			{
				int room_id = LMAN->m_AreaRooms[r];
				LRoom * pRoom = manager.GetRoom(room_id);

				// should not happen, assert?
				assert (pRoom);

				// trace as usual but don't go through the portals
				Trace_Recursive(0, *pRoom, planes, 0);
			}

/*
			// go through each affected room
			for (int r=0; r<light.m_NumAffectedRooms; r++)
			{
				int room_id = light.m_AffectedRooms[r];
				LRoom * pRoom = manager.GetRoom(room_id);

				// should not happen, assert?
				assert (pRoom);

				// trace as usual but don't go through the portals
				Trace_Recursive(0, *pRoom, planes, 0);
			}
*/
		} // if area light
	} // if light in view

	// we no longer need these planes
	manager.m_Pool.Free(pool_member);

	return bLightInView;
}


void LTrace::AddSpotlightPlanes(LVector<Plane> &planes) const
{
	Plane p(m_pCamera->m_ptPos, -m_pCamera->m_ptDir);
	planes.push_back(p);

	// this is kinda crappy, because ideally we'd want a cone, but instead we'll fake a frustum
	Vector3 pts[4];

	// assuming here that d is normalized!
	const Vector3 &d = m_pCamera->m_ptDir;
	const Vector3 &ptCam = m_pCamera->m_ptPos;

	assert (d.length_squared() < 1.1f);
	assert (d.length_squared() > 0.9f);

	// spotlight has no 'up' vector, as it is regular shape around direction axis
	// so we can use anything for side vector

	// this might balls up with a light pointing straight up
	Vector3 ptSide = Vector3(0, 1, 0).cross(d);

	float l = ptSide.length();
	if (l < 0.1f)
	{
		// special case straight up, lets cross against something else
		ptSide = d.cross(Vector3(1, 0, 0));
		l = ptSide.length();
		assert (l);
	}

	// unitize side
	ptSide *= 1.0 / l;

	Vector3 ptUp = ptSide.cross(d);
	ptUp.normalize();

	// now we've got the vecs, lets create some planes

	// spotlight spread definition (light.cpp, line 146)
	//float size = Math::tan(Math::deg2rad(param[PARAM_SPOT_ANGLE])) * len;

	// this is the size at distance 1 .. it would be more efficient to calc distance at which sides were 1, but whatever...
	float size = Math::tan(Math::deg2rad(m_pCamera->m_fSpread));

	ptSide *= size; // or half size? not sure yet
	ptUp *= -size;

	// pts will be bot left, top left, top right, bot right
	Vector3 ptEx = ptCam + d;

	pts[0] = ptEx - ptSide - ptUp;
	pts[1] = ptEx - ptSide + ptUp;
	pts[2] = ptEx + ptSide + ptUp;
	pts[3] = ptEx + ptSide - ptUp;

	Plane left(ptCam, pts[0], pts[1], COUNTERCLOCKWISE);
	Plane top(ptCam, pts[1], pts[2], COUNTERCLOCKWISE);
	Plane right(ptCam, pts[2], pts[3], COUNTERCLOCKWISE);
	Plane bottom(ptCam, pts[3], pts[0], COUNTERCLOCKWISE);

	planes.push_back(left);
	planes.push_back(top);
	planes.push_back(right);
	planes.push_back(bottom);

	// debug
	if (LMAN->m_bDebugFrustums)
	{
		for (int n=0; n<4; n++)
		{
			LMAN->m_DebugFrustums.push_back(ptCam);
			LMAN->m_DebugFrustums.push_back(pts[n]);
		}
	}
}

void LTrace::Trace_Begin(LRoom &room, LVector<Plane> &planes)
{
	int first_plane = 0;

	switch (m_pCamera->m_eType)
	{
	case LSource::ST_SPOTLIGHT:
		{
			// special cases of spotlight, add some extra planes to define the cone
			AddSpotlightPlanes(planes);
		}
		break;
	case LSource::ST_CAMERA:
		first_plane = 1;
		break;
	default:
		break;
	}


	LPRINT_RUN(2, "TRACE BEGIN");
	LPRINT_RUN(2, m_pCamera->MakeDebugString());


	Trace_Recursive(0, room, planes, first_plane);
}

void LTrace::Trace_Recursive(int depth, LRoom &room, const LVector<Plane> &planes, int first_portal_plane)
{
	// prevent too much depth
	if (depth > 8)
	{
		LPRINT_RUN(2, "\t\t\tDEPTH LIMIT REACHED");
		WARN_PRINT_ONCE("LPortal Depth Limit reached (seeing through > 8 portals)");
		return;
	}

	// for debugging
	Lawn::LDebug::m_iTabDepth = depth;
	LPRINT_RUN(2, "");

	LPRINT_RUN(2, "ROOM '" + itos(room.m_RoomID) + " : " + room.get_name() + "' planes " + itos(planes.size()) + " portals " + itos(room.m_iNumPortals) );

	// only handle one touch per frame so far (one portal into room)
	//assert (manager.m_uiFrameCounter > m_uiFrameTouched);

	// first touch
	DetectFirstTouch(room);

	if (m_TraceFlags & CULL_SOBS)
		CullSOBs(room, planes);

	if (m_TraceFlags & CULL_DOBS)
		CullDOBs(room, planes);

	// portals
	if (m_TraceFlags & DONT_TRACE_PORTALS)
		return;

	// look through portals
	int nPortals = room.m_iNumPortals;

	for (int port_num=0; port_num<nPortals; port_num++)
	{
		int port_id = room.m_iFirstPortal + port_num;

		const LPortal &port = LMAN->m_Portals[port_id];

		// have we already handled the room on this frame?
		// get the room pointed to by the portal
		LRoom * pLinkedRoom = &LMAN->Portal_GetLinkedRoom(port);



		// cull by portal angle to camera.

		// NEW! I've come up with a much better way of culling portals by direction to camera...
		// instead of using dot product with a varying view direction, we simply find which side of the portal
		// plane the camera is on! If it is behind, the portal can be seen through, if in front, it can't! :)
		float dist_cam = port.m_Plane.distance_to(m_pCamera->m_ptPos);
		LPRINT_RUN(2, "\tPORTAL " + itos (port_num) + " (" + itos(port_id) + ") " + port.get_name());
		if (dist_cam >= 0.0f) // was >
		{
			LPRINT_RUN(2, "\t\tCULLED (back facing)");
			continue;
		}

		/*
		// Note we need to deal with 'side on' portals, and the camera has a spreading view, so we cannot simply dot
		// the portal normal with camera direction, we need to take into account angle to the portal itself.
		const Vector3 &portal_normal = port.m_Plane.normal;
		LPRINT_RUN(2, "\tPORTAL " + itos (port_num) + " (" + itos(port_id) + ") " + port.get_name() + " normal " + portal_normal);

		// we will dot the portal angle with a ray from the camera to the portal centre
		// (there might be an even better ray direction but this will do for now)
		Vector3 dir_portal = port.m_ptCentre - m_pCamera->m_ptPos;

		// doesn't actually need to be normalized?
		float dot = dir_portal.dot(portal_normal);

		if (dot <= -0.0f) // 0.0
		{
			//LPRINT_RUN(2, "\t\tCULLED (wrong direction) dot is " + String(Variant(dot)) + ", dir_portal is " + dir_portal);
			LPRINT_RUN(2, "\t\tCULLED (wrong direction)");
			continue;
		}
		*/

		// is it culled by the planes?
		LPortal::eClipResult overall_res = LPortal::eClipResult::CLIP_INSIDE;

		// while clipping to the planes we maintain a list of partial planes, so we can add them to the
		// recursive next iteration of planes to check
		static LVector<int> partial_planes;
		partial_planes.clear();

		// for portals, we want to ignore the near clipping plane, as we might be right on the edge of a doorway
		// and still want to look through the portal.
		// So we are starting this loop from 1, ASSUMING that plane zero is the near clipping plane.
		// If it isn't we would need a different strategy
		// Note that now this only occurs for the first portal out of the current room. After that,
		// 0 is passed as first_portal_plane, because the near plane will probably be irrelevant,
		// and we are now not necessarily copying the camera planes.
		for (int l=first_portal_plane; l<planes.size(); l++)
		{
			LPortal::eClipResult res = port.ClipWithPlane(planes[l]);

			switch (res)
			{
			case LPortal::eClipResult::CLIP_OUTSIDE:
				overall_res = res;
				break;
			case LPortal::eClipResult::CLIP_PARTIAL:
				overall_res = res;
				partial_planes.push_back(l);
				break;
			default: // suppress warning
				break;
			}

			if (overall_res == LPortal::eClipResult::CLIP_OUTSIDE)
				break;
		}

		// this portal is culled
		if (overall_res == LPortal::eClipResult::CLIP_OUTSIDE)
		{
			LPRINT_RUN(2, "\t\tCULLED (outside planes)");
			continue;
		}

		// else recurse into that portal
		unsigned int uiPoolMem = LMAN->m_Pool.Request();
		if (uiPoolMem != (unsigned int) -1)
		{
			// get a vector of planes from the pool
			LVector<Plane> &new_planes = LMAN->m_Pool.Get(uiPoolMem);
			new_planes.clear();

			// NEW!! if portal is totally inside the planes, don't copy the old planes
			if (overall_res != LPortal::eClipResult::CLIP_INSIDE)
			{
				// copy the existing planes
				//new_planes.copy_from(planes);

				// new .. only copy the partial planes that the portal cuts through
				for (int n=0; n<partial_planes.size(); n++)
					new_planes.push_back(planes[partial_planes[n]]);
			}

			// add the planes for the portal
			// NOTE that we can also optimize by not adding portal planes for edges that
			// were behind a partial plane. NYI
			port.AddPlanes(*LMAN, m_pCamera->m_ptPos, new_planes);


			if (pLinkedRoom)
			{
				Trace_Recursive(depth+1, *pLinkedRoom, new_planes, 0);
				//pLinkedRoom->DetermineVisibility_Recursive(manager, depth + 1, cam, new_planes, 0);
				// for debugging need to reset tab depth
				Lawn::LDebug::m_iTabDepth = depth;
			}

			// we no longer need these planes
			LMAN->m_Pool.Free(uiPoolMem);
		}
		else
		{
			// planes pool is empty!
			// This will happen if the view goes through shedloads of portals
			// The solution is either to increase the plane pool size, or build levels
			// with views through multiple portals. Looking through multiple portals is likely to be
			// slow anyway because of the number of planes to test.
			WARN_PRINT_ONCE("Planes pool is empty");
		}

	} // for p through portals

}

void LTrace::DetectFirstTouch(LRoom &room)
{
	// mark if not reached yet on this trace
	if (!m_pBF_Rooms->GetBit(room.m_RoomID))
	{
		m_pBF_Rooms->SetBit(room.m_RoomID, true);

		if (m_TraceFlags & MAKE_ROOM_VISIBLE)
		{
			// keep track of which rooms are shown this trace
			m_pVisible_Rooms->push_back(room.m_RoomID);
		}

		// camera and light traces
		if (m_TraceFlags & TOUCH_ROOMS)
		{
			if (room.m_uiFrameTouched < LMAN->m_uiFrameCounter)
				FirstTouch(room);
		}
	}

}


void LTrace::FirstTouch(LRoom &room)
{
	// set the frame counter
	room.m_uiFrameTouched = LMAN->m_uiFrameCounter;

	// show this room and add to visible list of rooms
	room.Room_MakeVisible(true);

//	m_pBF_Rooms->SetBit(room.m_RoomID, true);

	// keep track of which rooms are shown this frame
//	m_pVisible_Rooms->push_back(room.m_RoomID);

	// hide all dobs
	/*
	for (int n=0; n<room.m_DOBs.size(); n++)
		room.m_DOBs[n].m_bVisible = false;
		*/
}
