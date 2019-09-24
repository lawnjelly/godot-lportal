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
#include "ldebug.h"


LRoom::LRoom() {
	m_RoomID = -1;
	m_uiFrameTouched = 0;
	m_iFirstPortal = 0;
	m_iNumPortals = 0;
	m_bVisible = true;

	m_iFirstSOB = 0;
	m_iNumSOBs = 0;
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
		if (m_DOBs[n].m_ID_Spatial == id)
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
	bool bCamera = pDOB->get_instance_id() == manager.m_ID_camera;
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
			LPRINT(0, "DOB at pos " + pt + " ahead of portal " + port.get_name() + " by " + String(Variant(dist)));

			// we want to move into the adjoining room
			return &manager.Portal_GetLinkedRoom(port);
		}
	}

	return 0;
}


// instead of directly showing and hiding objects we now set their layer,
// and the camera will hide them with a cull mask. This is so that
// objects can still be rendered outside immediate view for casting shadows.
// All objects in view (that are set to cast shadows) should cast shadows, so the actual
// shown objects are a superset of the softshown.
void LRoom::SoftShow(VisualInstance * pVI, bool bShow) const
{
	// hijack this layer number
	uint32_t mask = pVI->get_layer_mask();
	uint32_t orig_mask = mask;

	const int SOFT_SHOW_MASK = 1 << SOFT_SHOW_BIT;
	const int SOFT_HIDE_MASK = 1 << SOFT_HIDE_BIT;

	if (bShow)
	{
		// set
		mask |= SOFT_SHOW_MASK;
		// clear
		mask &= ~(1 | SOFT_HIDE_MASK);
	}
	else
	{
		// set
		mask |= SOFT_HIDE_MASK;
		// clear
		mask &= ~(1 | SOFT_SHOW_MASK);
	}


	// noop? don't touch the visual server if no change to mask
	if (mask == orig_mask)
		return;

	pVI->set_layer_mask(mask);

//	pVI->set_layer_mask_bit(0, false);
//	pVI->set_layer_mask_bit(SOFT_HIDE_BIT, bShow == false);
//	pVI->set_layer_mask_bit(SOFT_SHOW_BIT, bShow);
}


// hide all the objects not hit on this frame .. instead of calling godot hide without need
// (it might be expensive)
void LRoom::FinalizeVisibility(LRoomManager &manager)
{
	//print_line("FinalizeVisibility room " + get_name() + " NumSOBs " + itos(m_SOBs.size()) + ", NumDOBs " + itos(m_DOBs.size()));

	int last_sob = m_iFirstSOB + m_iNumSOBs;
	for (int n=m_iFirstSOB; n<last_sob; n++)
	{
		const LSob &sob = manager.m_SOBs[n];
		VisualInstance * pVI = sob.GetVI();

		if (pVI)
		{
			//SoftShow(pVI, sob.m_bSOBVisible);
			bool bVisible = manager.m_BF_visible_SOBs.GetBit(n) != 0;
			SoftShow(pVI, bVisible);
		}
	}

	for (int n=0; n<m_DOBs.size(); n++)
	{
		const LDob &dob = m_DOBs[n];

		// don't cull the main camera
		if (dob.m_ID_Spatial == manager.m_ID_camera)
			continue;

//		Spatial * pSpat = dob.GetSpatial();
//		if (pSpat)
//		{
//			// all should be showing .. this is mostly a no op
//			pSpat->show();
//		}

		VisualInstance * pVI = dob.GetVI();
		if (pVI)
		{
			SoftShow(pVI, dob.m_bVisible);
//			if (dob.m_bVisible)
//			{
//				//print("LRoom::FinalizeVisibility making visible dob " + pS->get_name());
//				pS->show();
//			}
//			else
//				pS->hide();
		}
	}
}

// allows us to show / hide all dobs as the room visibility changes
void LRoom::Room_MakeVisible(bool bVisible)
{
	// noop
	if (bVisible == m_bVisible)
		return;

	m_bVisible = bVisible;

	if (m_bVisible)
	{
		// show room
		GetGodotRoom()->show();

		// show all dobs
		for (int n=0; n<m_DOBs.size(); n++)
		{
			LDob &dob = m_DOBs[n];
			Spatial * pS = dob.GetSpatial();
			if (pS)
				pS->show();
		}
	}
	else
	{
		// hide room
		GetGodotRoom()->hide();

		// hide all dobs
		for (int n=0; n<m_DOBs.size(); n++)
		{
			LDob &dob = m_DOBs[n];
			Spatial * p = dob.GetSpatial();
			if (p)
				p->hide();
		}
	}
}


// hide godot room and all linked dobs
//void LRoom::Hide_All()
//{
//	GetGodotRoom()->hide();

//	for (int n=0; n<m_DOBs.size(); n++)
//	{
//		LDob &dob = m_DOBs[n];
//		Spatial * p = dob.GetSpatial();
//		if (p)
//			p->hide();
//	}
//}

// show godot room and all linked dobs and all sobs
void LRoom::Debug_ShowAll()
{
	Room_MakeVisible(true);

	// NYI .. change layers to all be visible
//	for (int n=0; n<m_SOBs.size(); n++)
//	{
//		LSob &sob = m_SOBs[n];
//		Spatial * pS = sob.GetSpatial();
//		if (pS)
//			pS->show();
//	}

}


void LRoom::FirstTouch(LRoomManager &manager)
{
	// set the frame counter
	m_uiFrameTouched = manager.m_uiFrameCounter;

	// show this room and add to visible list of rooms
	Room_MakeVisible(true);

	manager.m_BF_visible_rooms.SetBit(m_RoomID, true);

	// keep track of which rooms are shown this frame
	manager.m_pCurr_VisibleRoomList->push_back(m_RoomID);

	// hide all objects
//	int last_sob = m_iFirstSOB + m_iNumSOBs;
//	for (int n=m_iFirstSOB; n<last_sob; n++)
//	{
//		LSob &sob = manager.m_SOBs[n];
//		sob.m_bSOBVisible = false;
//	}

	// hide all dobs
	for (int n=0; n<m_DOBs.size(); n++)
		m_DOBs[n].m_bVisible = false;
}


void LRoom::DetermineVisibility_Recursive(LRoomManager &manager, int depth, const LCamera &cam, const LVector<Plane> &planes, int portalID_from)
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
	LPRINT_RUN(2, "ROOM '" + get_name() + "' planes " + itos(planes.size()) + " portals " + itos(m_iNumPortals) );

	// only handle one touch per frame so far (one portal into room)
	//assert (manager.m_uiFrameCounter > m_uiFrameTouched);

	// first touch
	if (m_uiFrameTouched < manager.m_uiFrameCounter)
		FirstTouch(manager);



	// clip all objects in this room to the clipping planes
	int last_sob = m_iFirstSOB + m_iNumSOBs;
	for (int n=m_iFirstSOB; n<last_sob; n++)
	{
		LSob &sob = manager.m_SOBs[n];

		// already determined to be visible through another portal
		//if (sob.m_bVisible)
			//continue;


		//LPRINT_RUN(2, "sob " + itos(n) + " " + sob.GetSpatial()->get_name());
		// already determined to be visible through another portal
		if (manager.m_BF_visible_SOBs.GetBit(n))
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
				bShow = false;
				break;
			}
		}

		if (bShow)
		{
			//sob.m_bSOBVisible = true;
			// sob is renderable and visible (not shadow only)
			manager.m_BF_visible_SOBs.SetBit(n, true);
			//manager.m_BF_render_SOBs.SetBit(n, true);
			//manager.m_RenderList_SOBs.push_back(n);
		}

	} // for through sobs


	// cull DOBs
	for (int n=0; n<m_DOBs.size(); n++)
	{
		LDob &dob = m_DOBs[n];

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



	// look through portals
	for (int port_num=0; port_num<m_iNumPortals; port_num++)
	{
		int port_id = m_iFirstPortal + port_num;

		// ignore if the portal we are looking in from
		// is this needed? surely the portal we are looking in from is in another room?
//		if (port_id == portalID_from)
//		{
//			LPRINT_RUN(2, "\tIGNORING PORTAL LOOKING IN FROM");
//			continue;
//		}

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
		LPRINT_RUN(2, "\tPORTAL " + itos (port_num) + " (" + itos(port_id) + ") " + port.get_name() + " normal " + portal_normal);

		// we will dot the portal angle with a ray from the camera to the portal centre
		// (there might be an even better ray direction but this will do for now)
		Vector3 dir_portal = port.m_ptCentre - cam.m_ptPos;

		// doesn't actually need to be normalized?
		float dot = dir_portal.dot(portal_normal);

		if (dot <= -0.0f) // 0.0
		{
			//LPRINT_RUN(2, "\t\tCULLED (wrong direction) dot is " + String(Variant(dot)) + ", dir_portal is " + dir_portal);
			LPRINT_RUN(2, "\t\tCULLED (wrong direction)");
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
		unsigned int uiPoolMem = manager.m_Pool.Request();
		if (uiPoolMem != -1)
		{
			// get a vector of planes from the pool
			LVector<Plane> &new_planes = manager.m_Pool.Get(uiPoolMem);

			// copy the existing planes
			new_planes.copy_from(planes);

			// add the planes for the portal
			port.AddPlanes(manager, cam.m_ptPos, new_planes);


			if (pLinkedRoom)
			{
				pLinkedRoom->DetermineVisibility_Recursive(manager, depth + 1, cam, new_planes, port_id);
				// for debugging need to reset tab depth
				Lawn::LDebug::m_iTabDepth = depth;
			}

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

	} // for p through portals
}


