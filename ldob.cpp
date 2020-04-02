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

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/

#include "ldob.h"
#include "scene/3d/mesh_instance.h"
#include "scene/3d/light.h"


bool LHidable::m_bDetach = true;

void LHidable::Hidable_Create(Node * pNode)
{
//	m_pNode = 0; m_pParent = 0; m_bShow = true;
	m_pNode = pNode;
	m_pParent = m_pNode->get_parent();
	m_bShow = true;
}


void LHidable::Show(bool bShow)
{
	// noop
	if (bShow == m_bShow)
		return;

	// new state
	m_bShow = bShow;

	assert (m_pParent);

	if (m_bDetach)
	{
		//String sz = "";
		if (bShow)
		{
			//sz = "show ";
			// add to tree
			m_pParent->add_child(m_pNode);
		}
		else
		{
			//sz = "hide ";
			// remove from tree
			m_pParent->remove_child(m_pNode);
		}
		//sz += m_pParent->get_name();
		//sz += "->";
		//sz += m_pNode->get_name();
		//print_line(sz);
	}
	else
	{
		if (bShow)
			((Spatial *) m_pNode)->show();
		else
			((Spatial *) m_pNode)->hide();
	}

}

/////////////////////////////////////////////////////////////////////

const char * LSource::m_szTypes[] = {"CAM", "DIR", "SPOT", "OMNI", };
const char * LSource::m_szClasses[] = {"STATIC", "ROOM", "DYN", };

void LSource::Source_SetDefaults()
{
	m_eType = ST_CAMERA;
	m_eClass = SC_STATIC;
	m_fSpread = 0.0f;
	m_fRange = FLT_MAX;
	m_RoomID = -1;

	// spotlight spread definition (light.cpp, line 146)
	// float size = Math::tan(Math::deg2rad(param[PARAM_SPOT_ANGLE])) * len;
}

String LSource::MakeDebugString() const
{
	String sz = "SOURCE : ";
	sz += String(m_szTypes[m_eType]) + " - ";
	sz += String(m_szClasses[m_eClass]) + "\tpos ";

	sz += String(Variant(m_ptPos)) + "\tdir ";
	sz += String(Variant(m_ptDir)) + "\n";

	return sz;
}


///////////////////////////////////////////////////


void LLight::Light_SetDefaults()
{
	m_Source.Source_SetDefaults();

	m_GodotID = 0;
	m_DOB_id = -1;
//	m_eType = LT_DIRECTIONAL;
//	m_eClass = LT_STATIC;
//	m_fSpread = 0.0f; // for spotlight
//	m_fMaxDist = 100.0f;
//	m_RoomID = -1;

	m_FirstCaster = 0;
	m_NumCasters = 0;

	m_NumAffectedRooms = 0;
	m_iArea = -1;
}

bool LLight::AddAffectedRoom(int room_id)
{
	if (m_NumAffectedRooms >= MAX_AFFECTED_ROOMS)
		return false;
	m_AffectedRooms[m_NumAffectedRooms++] = room_id;
	return true;
}



// dynamic light update
void LLight::Update()
{
	// firstly, has it crossed into another room like the dobs
}

String LLight::MakeDebugString() const
{
	String sz;
	sz += m_Source.MakeDebugString();
	return sz;
}


Light * LLight::GetGodotLight()
{
	Object * pObj = ObjectDB::get_instance(m_GodotID);
	Light * p = Object::cast_to<Light>(pObj);
	return p;
}


/////////////////////////////////////////////////////////////////////

Spatial * LSob::GetSpatial() const
{
	Object * pObj = ObjectDB::get_instance(m_ID);
	Spatial * pSpat = Object::cast_to<Spatial>(pObj);
	return pSpat;
}


bool LSob::IsShadowCaster() const
{
	Object * pObj = ObjectDB::get_instance(m_ID);
	GeometryInstance * pGI = Object::cast_to<GeometryInstance>(pObj);

	if (pGI)
	{
		if (pGI->get_cast_shadows_setting() == GeometryInstance::SHADOW_CASTING_SETTING_OFF)
			return false;

		return true;
	}

	// not sure yet, maybe this should be true, depends what the non geometry objects are
	return false;
}


GeometryInstance * LSob::GetGI() const
{
	Object * pObj = ObjectDB::get_instance(m_ID);
	GeometryInstance * pGI = Object::cast_to<GeometryInstance>(pObj);
	return pGI;
}

VisualInstance * LSob::GetVI() const
{
	Object * pObj = ObjectDB::get_instance(m_ID);
	VisualInstance * pVI = Object::cast_to<VisualInstance>(pObj);
	return pVI;
}



/*
void LSob::Show(bool bShow)
{
	// noop
	if (bShow == m_bShow)
		return;

	// new state
	m_bShow = bShow;

	Spatial * pS = GetSpatial();
	if (!pS)
		return;

	assert (m_pParent);

	if (bShow)
	{
		// add to tree
		m_pParent->add_child(m_pNode);
	}
	else
	{
		// remove from tree
		m_pParent->remove_child(m_pNode);
	}

	// noop
//	if (pS->is_visible() == bShow)
//		return;

//	if (bShow)
//		pS->show();
//	else
//		pS->hide();

//	GeometryInstance * pGI = Object::cast_to<GeometryInstance>(pS);
//	if (pGI)
//	{
//		// godot visible bug workaround
//		pGI->set_extra_cull_margin(0.0f);
//	}

}
*/


Spatial * LDob::GetSpatial() const
{
	Object * pObj = ObjectDB::get_instance(m_ID_Spatial);
	Spatial * pSpat = Object::cast_to<Spatial>(pObj);
	return pSpat;
}

VisualInstance * LDob::GetVI() const
{
	Object * pObj = ObjectDB::get_instance(m_ID_VI);
	VisualInstance * pVI = Object::cast_to<VisualInstance>(pObj);
	return pVI;
}

