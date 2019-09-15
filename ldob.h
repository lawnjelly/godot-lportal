#pragma once

#include "scene/3d/spatial.h"


// static object
class LSob
{
public:
	ObjectID m_ID; // godot object
	AABB m_aabb; // world space
};

// dynamic object
class LDob
{
public:
	ObjectID m_ID;
};
