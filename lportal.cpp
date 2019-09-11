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

#include "lportal.h"
#include "core/engine.h"
#include "lroom.h"


//#define SMOOTHCLASS Smooth
//#define SMOOTHNODE Spatial
//#include "smooth_body.inl"

bool LPortal::NameStartsWith(Node * pNode, String szSearch)
{
	int sl = szSearch.length();

	String name = pNode->get_name();
	int l = name.length();

	if (l < sl)
		return false;

	String szStart = name.substr(sl);

	if (szStart == szSearch)
		return true;

	return false;
}


String LPortal::FindNameAfter(Node * pNode, int CharsToMiss)
{
	String szRes;
	String name = pNode->get_name();
	szRes = name.substr(CharsToMiss);
	return szRes;
}

//////////////////////////////////////////////////////////

LPortal::eClipResult LPortal::ClipWithPlane(const Plane &p) const
{
	int nOutside = 0;
	int nPoints = m_ptsWorld.size();

	for (int n=0; n<nPoints; n++)
	{
		float d = p.distance_to(m_ptsWorld[n]);

		if  (d >= 0.0)
			nOutside++;
	}

	if (nOutside == nPoints)
		return CLIP_OUTSIDE;

	if (nOutside == 0)
		return CLIP_INSIDE;

	return CLIP_PARTIAL;
}




// use the name of the portal to find a room to link to
void LPortal::Link(LRoom * pParentRoom)
{
	// should start with 'portal_'
	if (!NameStartsWith(this, "portal_"))
	{
		WARN_PRINT("Portal name should begin with lportal_");
		return;
	}

	String szRoom = FindNameAfter(this, 8);

	// find the room group
	Spatial * pGroup = Object::cast_to<Spatial>(pParentRoom->get_parent());
	if (!pGroup)
	{
		WARN_PRINT("Room parent is not a spatial");
		return;
	}

	// attempt to find a child of the group that has the name specified
	int nChildren = pGroup->get_child_count();

	for (int n=0; n<nChildren; n++)
	{
		Node * pChild = pGroup->get_child(n);

		String szChildName = pChild->get_name();

		// is the name correct for the desired room?
		if (szRoom != szChildName)
			continue;

		LRoom * pTargetRoom = Object::cast_to<LRoom>(pChild);

		if (!pTargetRoom)
		{
			WARN_PRINT("Portal target is not a room");
			return;
		}

		// found! link
		pTargetRoom->MakeOppositePortal(this, pParentRoom);
		return;
	}
}


void LPortal::CreateGeometry(PoolVector<Vector3> p_vertices)
{
	int nPoints = p_vertices.size();
	ERR_FAIL_COND(nPoints < 3);

	m_ptsWorld.resize(nPoints);

	for (int n=0; n<nPoints; n++)
	{
		m_ptsWorld.set(n, p_vertices[n]);
	}

	PlaneFromPoints();
}

void LPortal::CopyReversedGeometry(const LPortal &source)
{
	// points are the same but reverse winding order
	int nPoints = source.m_ptsWorld.size();
	m_ptsWorld.resize(nPoints);

	for (int n=0; n<nPoints; n++)
	{
		m_ptsWorld.set(n, source.m_ptsWorld[nPoints - n - 1]);
	}

	PlaneFromPoints();
}

void LPortal::PlaneFromPoints()
{
	if (m_ptsWorld.size() < 3)
	{
		WARN_PRINT("Portal must have at least 3 vertices");
		return;
	}
	// create plane from points
	m_Plane = Plane(m_ptsWorld[0], m_ptsWorld[1], m_ptsWorld[2]);
}


bool LPortal::AddRoom(NodePath path)
{
	if (has_node(path))
	{
		LRoom * pNode = Object::cast_to<LRoom>(get_node(path));
		if (pNode)
		{
			ObjectID id = pNode->get_instance_id();

			m_room_path = path;
			m_room_ID = id;

			// make the portal name correct and feature the room name
			int num_names = path.get_name_count();
			if (num_names < 1)
			{
				WARN_PRINT("LPortal::AddRoom : Path too short");
				return false;
			}
			String szRoom = path.get_name(num_names-1);

			String szPortal = "lportal_" + szRoom;
			set_name(szPortal);

			return true;
		}
		else
		{
			WARN_PRINT("not a room");
			return false;
		}
	}

	return false;
}

LPortal::LPortal() {
	// unset
	m_room_ID = 0;
}


void LPortal::_bind_methods() {

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
//ClassDB::bind_method(D_METHOD("set_method", "method"), &Room::set_method);
//ClassDB::bind_method(D_METHOD("get_method"), &Room::get_method);

//ADD_GROUP("Misc", "");
//ADD_PROPERTY(PropertyInfo(Variant::INT, "method", PROPERTY_HINT_ENUM, "Slerp,Lerp"), "set_method", "get_method");
}



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
