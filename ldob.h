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

	ObjectID m_ID_Spatial;
	ObjectID m_ID_VI;
	bool m_bVisible;
	float m_fRadius;
};


class LLight : public LHidable
{
public:
	enum eLightType
	{
		LT_DIRECTIONAL,
		LT_SPOTLIGHT,
		LT_OMNI,
	};
	void SetDefaults();
	Light * GetGodotLight();
	bool IsGlobal() const {return m_RoomID == -1;}

	Vector3 m_ptDir;
	Vector3 m_ptPos;
	ObjectID m_GodotID;
	eLightType m_eType;
	float m_fSpread; // for spotlight
	float m_fMaxDist; // shadow distance not light distance

	// source room
	int m_RoomID; // or -1 for global lights

	// shadow casters
	int m_FirstCaster;
	int m_NumCasters;
};
