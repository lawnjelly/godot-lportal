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

#include "lroom.h"
#include "core/engine.h"
#include "scene/3d/mesh_instance.h"
#include "lportal.h"
#include "lbitfield_dynamic.h"
#include "lroom_manager.h"

//#define LROOM_VERBOSE

void LRoom::print(String sz)
{
#ifdef LROOM_VERBOSE
	LPortal::print(sz);
#endif
}

LRoom::LRoom() {
	m_RoomID = -1;
	m_uiFrameTouched = 0;
	m_iFirstPortal = 0;
	m_iNumPortals = 0;
}




Spatial * LRoom::GetGodotRoom() const
{
	Object *pObj = ObjectDB::get_instance(m_GodotID);

	// assuming is a portal
	Spatial * pSpat = Object::cast_to<Spatial>(pObj);

	return pSpat;
}



void LRoom::DOB_Add(const LDob &dob)
{
//	LDob dob;
//	dob.m_ID = pDOB->get_instance_id();

	m_DOBs.push_back(dob);
}

unsigned int LRoom::DOB_Find(Node * pDOB) const
{
	ObjectID id = pDOB->get_instance_id();

	for (int n=0; n<m_DOBs.size(); n++)
	{
		if (m_DOBs[n].m_ID == id)
		{
			return n;
		}
	}

	return -1;
}

bool LRoom::DOB_Remove(unsigned int ui)
{
	if (ui < m_DOBs.size())
	{
		m_DOBs.remove_unsorted(ui);
		return true;
	}

	return false;
}


// returns -1 if no change, or the linked room we are moving into
LRoom * LRoom::DOB_Update(LRoomManager &manager, Spatial * pDOB)
{
	const Vector3 &pt = pDOB->get_global_transform().origin;

	// is it the camera?
	bool bCamera = pDOB->get_instance_id() == manager.m_cameraID;
	float slop = 0.2f;
	if (bCamera)
		slop = 0.0f;

	// the camera can't have slop because we might end up front side of a door without entering the room,
	// hence can't see into the room through the portal!
//	if (bCamera)
//		slop = 0.0f;

	// check each portal - has the object crossed it into the neighbouring room?
	for (int p=0; p<m_iNumPortals; p++)
	{
		const LPortal &port = manager.m_Portals[m_iFirstPortal + p];

		float dist = port.m_Plane.distance_to(pt);

		if (dist > slop)
		{
#ifdef LROOM_VERBOSE
			print("DOB at pos " + pt + " ahead of portal " + port.get_name() + " by " + String(Variant(dist)));
#endif

			// we want to move into the adjoining room
			return &manager.Portal_GetLinkedRoom(port);
		}
	}

	return 0;
}

// hide all the objects not hit on this frame .. instead of calling godot hide without need
// (it might be expensive)
void LRoom::FinalizeVisibility(LRoomManager &manager)
{
	//print_line("FinalizeVisibility room " + get_name() + " NumSOBs " + itos(m_SOBs.size()) + ", NumDOBs " + itos(m_DOBs.size()));

	for (int n=0; n<m_SOBs.size(); n++)
	{
		const LSob &sob = m_SOBs[n];
		Spatial * pS = sob.GetSpatial();

		if (pS)
		{
			if (sob.m_bVisible)
				pS->show();
			else
				pS->hide();
		}
	}

	for (int n=0; n<m_DOBs.size(); n++)
	{
		const LDob &dob = m_DOBs[n];

		// don't cull the main camera
		if (dob.m_ID == manager.m_cameraID)
			continue;

		Spatial * pS = dob.GetSpatial();
		if (pS)
		{
			if (dob.m_bVisible)
			{
				//print("LRoom::FinalizeVisibility making visible dob " + pS->get_name());
				pS->show();
			}
			else
				pS->hide();
		}
	}
}

// hide godot room and all linked dobs
void LRoom::Hide_All()
{
	GetGodotRoom()->hide();

	for (int n=0; n<m_DOBs.size(); n++)
	{
		LDob &dob = m_DOBs[n];
		Spatial * pS = dob.GetSpatial();
		if (pS)
			pS->hide();
	}
}

void LRoom::FirstTouch(LRoomManager &manager)
{
	// set the frame counter
	m_uiFrameTouched = manager.m_uiFrameCounter;

	// keep track of which rooms are shown this frame
	manager.m_pCurr_VisibleRoomList->push_back(m_RoomID);

	// hide all objects
	for (int n=0; n<m_SOBs.size(); n++)
		m_SOBs[n].m_bVisible = false;

	// hide all dobs
	for (int n=0; n<m_DOBs.size(); n++)
		m_DOBs[n].m_bVisible = false;
}


void LRoom::DetermineVisibility_Recursive(LRoomManager &manager, int depth, const LCamera &cam, const LVector<Plane> &planes, int portalID_from)
{
	// prevent too much depth
	if (depth >= 8)
	{
#ifdef LROOM_VERBOSE
		print("\t\t\tDEPTH LIMIT REACHED");
#endif
		return;
	}

#ifdef LROOM_VERBOSE
	print("DetermineVisibility_Recursive from " + get_name());
#endif

	// only handle one touch per frame so far (one portal into room)
	//assert (manager.m_uiFrameCounter > m_uiFrameTouched);

	// first touch
	if (m_uiFrameTouched < manager.m_uiFrameCounter)
		FirstTouch(manager);

	// show this room and add to visible list of rooms
	GetGodotRoom()->show();
	manager.m_BF_visible_rooms.SetBit(m_RoomID, true);

#define LPORTAL_CULL_STATIC
#ifdef LPORTAL_CULL_STATIC

	// clip all objects in this room to the clipping planes
	for (int n=0; n<m_SOBs.size(); n++)
	{
		LSob &sob = m_SOBs[n];

		// already determined to be visible through another portal
		if (sob.m_bVisible)
			continue;

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
				bShow = false;
				break;
			}
		}

		if (bShow)
			sob.m_bVisible = true;

	}


#else
	// clip all objects in this room to the clipping planes
	for (int n=0; n<m_SOBs.size(); n++)
	{
		const LSob sob = m_SOBs[n];
		Object * pNode = ObjectDB::get_instance(sob.m_ID);

		VisualInstance * pObj = Object::cast_to<VisualInstance>(pNode);

		// should always be a visual instance, only these are added as SOBs
		if (pObj)
		{
			//Vector3 pt = pObj->get_global_transform().origin;

			bool bShow = true;


			// estimate the radius .. for now
			AABB bb = pObj->get_transformed_aabb();

			print("\t\t\tculling object " + pObj->get_name());

			for (int p=0; p<planes.size(); p++)
			{
//				float dist = planes[p].distance_to(pt);
//				print("\t\t\t\t" + itos(p) + " : dist " + String(Variant(dist)));

				float r_min, r_max;
				bb.project_range_in_plane(planes[p], r_min, r_max);

				print("\t\t\t\t" + itos(p) + " : r_min " + String(Variant(r_min)) + ", r_max " + String(Variant(r_max)));


				if (r_min > 0.0f)
				{
					bShow = false;
					break;
				}
			}

			if (bShow)
				sob.m_bVisible = true;
//				pObj->show();
//			else
//				pObj->hide();

		}
	}
#endif

	// cull DOBs
	for (int n=0; n<m_DOBs.size(); n++)
	{
		LDob &dob = m_DOBs[n];

		Spatial * pObj = dob.GetSpatial();

		if (pObj)
		{
			bool bShow = true;
			const Vector3 &pt = pObj->get_global_transform().origin;

			//print_line("\t\t\tculling dob " + pObj->get_name());
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
				dob.m_bVisible = true;
		}
	}



	// look through portals
	for (int p=0; p<m_iNumPortals; p++)
	{
		int port_id = m_iFirstPortal + p;

		// ignore if the portal we are looking in from
		// is this needed? surely the portal we are looking in from is in another room?
		if (port_id == portalID_from)
			continue;

		const LPortal &port = manager.m_Portals[port_id];

		// have we already handled the room on this frame?
		// get the room pointed to by the portal
		LRoom * pLinkedRoom = &manager.Portal_GetLinkedRoom(port);

//		if (pLinkedRoom->m_uiFrameTouched == manager.m_uiFrameCounter)
//			continue;

		// cull by portal angle to camera.
		// Note we need to deal with 'side on' portals, and the camera has a spreading view, so we cannot simply dot
		// the portal normal with camera direction, we need to take into account angle to the portal itself.
		const Vector3 &portal_normal = port.m_Plane.normal;
#ifdef LROOM_VERBOSE
		print("\ttesting portal " + port.get_name() + " normal " + portal_normal);
#endif

		// we will dot the portal angle with a ray from the camera to the portal centre
		// (there might be an even better ray direction but this will do for now)
		Vector3 dir_portal = port.m_ptCentre - cam.m_ptPos;

		// doesn't actually need to be normalized?
		float dot = dir_portal.dot(portal_normal);

//		float dot = cam.m_ptDir.dot(portal_normal);
		if (dot <= -0.0f) // 0.0
		{
#ifdef LROOM_VERBOSE
			print("\t\tportal culled (wrong direction) dot is " + String(Variant(dot)) + ", dir_portal is " + dir_portal);
#endif
			continue;
		}

		// is it culled by the planes?
		LPortal::eClipResult overall_res = LPortal::eClipResult::CLIP_INSIDE;

		// for portals, we want to ignore the near clipping plane, as we might be right on the edge of a doorway
		// and still want to look through the portal.
		// So we are starting this loop from 1, ASSUMING that plane zero is the near clipping plane.
		// If it isn't we would need a different strategy
		for (int l=1; l<planes.size(); l++)
		{
			LPortal::eClipResult res = port.ClipWithPlane(planes[l]);

			switch (res)
			{
			case LPortal::eClipResult::CLIP_OUTSIDE:
				overall_res = res;
				break;
			case LPortal::eClipResult::CLIP_PARTIAL:
				overall_res = res;
				break;
			}

			if (overall_res == LPortal::eClipResult::CLIP_OUTSIDE)
				break;
		}

		// this portal is culled
		if (overall_res == LPortal::eClipResult::CLIP_OUTSIDE)
		{
#ifdef LROOM_VERBOSE
			print("\t\tportal culled (outside planes)");
#endif
			continue;
		}

		// else recurse into that portal
		unsigned int uiPoolMem = manager.m_Pool.Request();
		if (uiPoolMem != -1)
		{
			// get a vector of planes from the pool
			LVector<Plane> &new_planes = manager.m_Pool.Get(uiPoolMem);

			// copy the existing planes
			new_planes.copy_from(planes);

			// add the planes for the portal
			port.AddPlanes(cam.m_ptPos, new_planes);


			if (pLinkedRoom)
				pLinkedRoom->DetermineVisibility_Recursive(manager, depth + 1, cam, new_planes, port_id);

			// we no longer need these planes
			manager.m_Pool.Free(uiPoolMem);
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
	}
}


