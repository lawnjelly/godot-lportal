#pragma once

#include "scene/3d/spatial.h"

class VisualInstance;

// static object
class LSob
{
public:
	Spatial * GetSpatial() const;

	ObjectID m_ID; // godot object
	AABB m_aabb; // world space
	bool m_bVisible;
};

// dynamic object
class LDob
{
public:
	Spatial * GetSpatial() const;

	ObjectID m_ID;
	bool m_bVisible;
	float m_fRadius;
};
