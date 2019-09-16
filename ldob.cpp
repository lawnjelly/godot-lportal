#include "ldob.h"
#include "scene/3d/mesh_instance.h"


Spatial * LSob::GetSpatial() const
{
	Object * pObj = ObjectDB::get_instance(m_ID);
	Spatial * pSpat = Object::cast_to<Spatial>(pObj);
	return pSpat;
}



Spatial * LDob::GetSpatial() const
{
	Object * pObj = ObjectDB::get_instance(m_ID);
	Spatial * pSpat = Object::cast_to<Spatial>(pObj);
	return pSpat;
}

