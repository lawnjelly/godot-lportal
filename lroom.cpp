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


//#define SMOOTHCLASS Smooth
//#define SMOOTHNODE Spatial
//#include "smooth_body.inl"


LRoom::LRoom() {
//	m_Flags = 0;
//	SetFlags(SF_ENABLED | SF_TRANSLATE | SF_ROTATE);
}

void LRoom::DetermineVisibility_Recursive(LCamera &cam, const Vector<Plane> &planes, ObjectID portalID_from)
{
	// clip all objects in this room to the clipping planes
	// NYI

	// go through each portal out of here
	int nPortals = m_portal_IDs.size();

	ObjectID this_room_id = get_instance_id();

	for (int p=0; p<nPortals; p++)
	{
		ObjectID id = m_portal_IDs[p];

		// ignore if the portal we are looking in from
		if (id == portalID_from)
			continue;

		// get the portal
		Object *pObj = ObjectDB::get_instance(id);

		// assuming is a portal
		LPortal * pPortal = Object::cast_to<LPortal>(pObj);

		if (!pPortal)
		{
			WARN_PRINT_ONCE("LRoom::DetermineVisibility_Recursive : Not a portal");
			continue;
		}

		const Vector3 &portal_normal = pPortal->m_Plane.normal;

		// direction with the camera? (might not need to check)
		float dot = cam.m_ptDir.dot(portal_normal);
		if (dot <= 0.0f)
			continue;

		// is it culled by the planes?
		LPortal::eClipResult overall_res = LPortal::eClipResult::CLIP_INSIDE;

		for (int l=0; l<planes.size(); l++)
		{
			LPortal::eClipResult res = pPortal->ClipWithPlane(planes[l]);

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
			continue;

		// else recurse into that portal
		Vector<Plane> new_planes = planes;

		// add the planes for the portal
		// NYI

		DetermineVisibility_Recursive(cam, new_planes, id);
	}
}



// initial setup, allows importing portals as meshes from modelling program,
// which will be auto converted to LPortals with this method
void LRoom::DetectPortalMeshes()
{
	bool bFoundOne = true;

	while (bFoundOne)
	{
		bFoundOne = false;

		for (int n=0; n<get_child_count(); n++)
		{
			Node * pChild = get_child(n);

			MeshInstance * pMesh = Object::cast_to<MeshInstance>(pChild);
			if (pMesh)
			{
				// name must start with portal_
				if (LPortal::NameStartsWith(pMesh, "portal_"))
				{
					String szLinkRoom = LPortal::FindNameAfter(pMesh, 8);
					DetectedPortalMesh(pMesh, szLinkRoom);
					bFoundOne = true;
				}
			}

			if (bFoundOne)
				break;
		}
	}

}

void LRoom::DetectedPortalMesh(MeshInstance * pMeshInstance, String szLinkRoom)
{
	Ref<Mesh> rmesh = pMeshInstance->get_mesh();

	Array arrays = rmesh->surface_get_arrays(0);
	PoolVector<Vector3> p_vertices = arrays[VS::ARRAY_VERTEX];

	LPortal * pNew = memnew(LPortal);
	pNew->set_name("lportal");
	add_child(pNew);

	NodePath temppath = "../../" + szLinkRoom;
	pNew->AddRoom(szLinkRoom);

	// create the portal geometry
	pNew->CreateGeometry(p_vertices);

	// delete the original child
	pMeshInstance->get_parent()->remove_child(pMeshInstance);

	pMeshInstance->queue_delete();
}

void LRoom::MakePortalsTwoWay()
{
	for (int n=0; n<get_child_count(); n++)
	{
		Node * pChild = get_child(n);

		LPortal * pPortal = Object::cast_to<LPortal>(pChild);
		if (pPortal)
		{
			pPortal->Link(this);
		}
	}

}

// assuming that portals are a child of the room, detect these and make them 2 way
void LRoom::MakePortalQuickList()
{
	// this function could be called more than one time...
	m_portal_IDs.clear();

	for (int n=0; n<get_child_count(); n++)
	{
		Node * pChild = get_child(n);

		LPortal * pPortal = Object::cast_to<LPortal>(pChild);
		if (pPortal)
		{
			ObjectID id = pPortal->get_instance_id();
			m_portal_IDs.push_back(id);
		}
	}
}

// make sure there is a back (opposite) portal leading from the portal from to the roomto, from this room
void LRoom::MakeOppositePortal(LPortal * pPortalFrom, LRoom * pRoomTo)
{
	ObjectID room_to_id = pRoomTo->get_instance_id();

	// does an opposite portal exist already?
	for (int n=0; n<get_child_count(); n++)
	{
		Node * pChild = get_child(n);

		LPortal * pPortal = Object::cast_to<LPortal>(pChild);
		if (pPortal)
		{
			if (pPortal->GetLinkedRoomID() == room_to_id)
				// already linked
				return;
		}
	}

	// if we got to here it isn't linked...
	// add a new portal
	// NYI
	LPortal * pNew = memnew(LPortal);
	pNew->set_name("lportal");
	add_child(pNew);

	pNew->AddRoom(pRoomTo->get_path());
	pNew->CopyReversedGeometry(*pPortalFrom);
}


void LRoom::_bind_methods() {

//	BIND_ENUM_CONSTANT(MODE_LOCAL);
//	BIND_ENUM_CONSTANT(MODE_GLOBAL);


//	ClassDB::bind_method(D_METHOD("teleport"), &SMOOTHCLASS::teleport);

//	ClassDB::bind_method(D_METHOD("set_enabled"), &SMOOTHCLASS::set_enabled);
//	ClassDB::bind_method(D_METHOD("is_enabled"), &SMOOTHCLASS::is_enabled);
//	ClassDB::bind_method(D_METHOD("set_smooth_translate"), &SMOOTHCLASS::set_interpolate_translation);
//	ClassDB::bind_method(D_METHOD("get_smooth_translate"), &SMOOTHCLASS::get_interpolate_translation);
//	ClassDB::bind_method(D_METHOD("set_smooth_rotate"), &SMOOTHCLASS::set_interpolate_rotation);
//	ClassDB::bind_method(D_METHOD("get_smooth_rotate"), &SMOOTHCLASS::get_interpolate_rotation);
//	ClassDB::bind_method(D_METHOD("set_smooth_scale"), &SMOOTHCLASS::set_interpolate_scale);
//	ClassDB::bind_method(D_METHOD("get_smooth_scale"), &SMOOTHCLASS::get_interpolate_scale);

//	ClassDB::bind_method(D_METHOD("set_input_mode", "mode"), &SMOOTHCLASS::set_input_mode);
//	ClassDB::bind_method(D_METHOD("get_input_mode"), &SMOOTHCLASS::get_input_mode);
//	ClassDB::bind_method(D_METHOD("set_output_mode", "mode"), &SMOOTHCLASS::set_output_mode);
//	ClassDB::bind_method(D_METHOD("get_output_mode"), &SMOOTHCLASS::get_output_mode);

//	ClassDB::bind_method(D_METHOD("set_target", "target"), &SMOOTHCLASS::set_target);
//	ClassDB::bind_method(D_METHOD("set_target_path", "path"), &SMOOTHCLASS::set_target_path);
//	ClassDB::bind_method(D_METHOD("get_target_path"), &SMOOTHCLASS::get_target_path);


//	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

//	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target"), "set_target_path", "get_target_path");


//	ADD_GROUP("Components", "");
//	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "smooth_translate"), "set_smooth_translate", "get_smooth_translate");
//	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "smooth_rotate"), "set_smooth_rotate", "get_smooth_rotate");
//	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "smooth_scale"), "set_smooth_scale", "get_smooth_scale");
//	ADD_GROUP("Coords", "");
//	ADD_PROPERTY(PropertyInfo(Variant::INT, "input", PROPERTY_HINT_ENUM, "Local,Global"), "set_input_mode", "get_input_mode");
//	ADD_PROPERTY(PropertyInfo(Variant::INT, "output", PROPERTY_HINT_ENUM, "Local,Global"), "set_output_mode", "get_output_mode");

//	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lerp"), "set_lerp", "get_lerp");

// finish the bind with custom stuff
//BIND_ENUM_CONSTANT(METHOD_SLERP);
//BIND_ENUM_CONSTANT(METHOD_LERP);
//ClassDB::bind_method(D_METHOD("set_method", "method"), &LRoom::set_method);
//ClassDB::bind_method(D_METHOD("get_method"), &LRoom::get_method);

//ADD_GROUP("Misc", "");
//ADD_PROPERTY(PropertyInfo(Variant::INT, "method", PROPERTY_HINT_ENUM, "Slerp,Lerp"), "set_method", "get_method");
}



//void LRoom::SetupPortal(LPortal * pPortal)
//{
//	ObjectID id = pPortal->get_instance_id();
//	m_portal_IDs.push_back(id);

//}

//void LRoom::AddPortal(ObjectID id)
//{
//}

//bool LRoom::AddPortal(LPortal * pNode)
//{
//	if (has_node(path))
//	{
//		LPortal * pNode = Object::cast_to<LPortal>(get_node(path));
//		if (pNode)
//		{
//			ObjectID id = pNode->get_instance_id();

//			m_portal_paths.push_back(path);
//			m_portal_IDs.push_back(id);

//			// add the room to the portal automatically
//			NodePath self_path = get_path_to(this);
//			pNode->AddRoom(self_path);
//			return true;
//		}
//		else
//		{
//			WARN_PRINT("not a portal");
//			return false;
//		}
//	}

//	return false;
//}

//void Smooth::set_method(eMethod p_method)
//{
	//ChangeFlags(SF_LERP, p_method == METHOD_LERP);
//}

//Smooth::eMethod Smooth::get_method() const
//{
	//if (TestFlags(SF_LERP))
		//return METHOD_LERP;

	//return METHOD_SLERP;
//}






//bool Smooth::FindVisibility() const
//{
//	const Spatial *s = this;

//	int count = 0;
//	while (s) {

//		if (!s->data.visible)
//		{
//			print_line(itos(count++) + " hidden");
//			return false;
//		}
//		else
//		{
//			print_line(itos(count++) + " visible");
//		}
//		s = s->data.parent;
//	}

//	return true;
//}
