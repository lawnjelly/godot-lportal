#pragma once

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


#include "scene/3d/spatial.h"

class VisualInstance;
class GeometryInstance;
class Light;

class LHidable
{
public:
	void Hidable_Create(Node * pNode);
	void Show(bool bShow);

	// new .. can be separated from the scene tree to cull
	Node * m_pNode;
	Node * m_pParent;

	// separate flag so we don't have to touch the godot lookup
	bool m_bShow;

	// which method we are using to show and hide .. detaching from scene tree or just show / hide through godot
	static bool m_bDetach;
};

// static object
class LSob : public LHidable
{
public:
	Spatial * GetSpatial() const;
	VisualInstance * GetVI() const;
	GeometryInstance * GetGI() const;
	//void Show(bool bShow);
	bool IsShadowCaster() const;

	ObjectID m_ID; // godot object
	AABB m_aabb; // world space
};

// dynamic object
class LDob
{
public:
	Spatial * GetSpatial() const;
	VisualInstance * GetVI() const;

	bool m_bSlotTaken;
	bool m_bVisible;
	float m_fRadius;
	int m_iRoomID;

	ObjectID m_ID_Spatial;
	ObjectID m_ID_VI;
};


//class LCamera
// trace source can be camera or light
class LSource
{
public:
	enum eSourceType
	{
		ST_CAMERA, // frustum planes will have been added
		ST_DIRECTIONAL,
		ST_SPOTLIGHT, // trace should add back plane and cone planes
		ST_OMNI, // no planes, can go in any direction
	};

	enum eSourceClass
	{
		SC_STATIC, // non moving light
		SC_ROOM, // only moves within the room
		SC_DYNAMIC, // can move between rooms (register as a DOB)
	};


	void Source_SetDefaults();
	String MakeDebugString() const;

	// funcs
	bool IsGlobal() const {return m_RoomID == -1;}

	// all in world space, culling done in world space
	Vector3 m_ptPos;
	Vector3 m_ptDir;
	eSourceType m_eType;
	eSourceClass m_eClass;

	float m_fSpread; // for spotlight
	float m_fRange; // shadow distance not light distance

	// source room
	int m_RoomID; // or -1 for global lights

private:
	static const char * m_szTypes[];
	static const char * m_szClasses[];
};



class LLight : public LHidable
{
public:
	enum {MAX_AFFECTED_ROOMS=64};

	LSource m_Source;
	ObjectID m_GodotID;
	int m_DOB_id;

	// shadow casters
	int m_FirstCaster;
	int m_NumCasters;

	// funcs

	// for dynamic lights
	// move them light dobs across planes
	// and update the rooms that are affected by the light
	void Update();
	String MakeDebugString() const;

	void Light_SetDefaults();
	Light * GetGodotLight();


	bool AddAffectedRoom(int room_id);
	void ClearAffectedRooms() {m_NumAffectedRooms = 0;}

	// keep a list of the rooms affected by this light
	uint16_t m_AffectedRooms[MAX_AFFECTED_ROOMS];
	int m_NumAffectedRooms;

	// for global lights, this is the area or -1 if unset
	int m_iArea;
	String m_szArea; // set to the area string in the case of area lights, else ""
};
