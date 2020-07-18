#include "lscene_saver.h"
#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"
#include "core/io/resource_saver.h"

void LSceneSaver::SetOwnerRecursive(Node * pNode, Node * pOwner)
{
	if (pNode != pOwner)
		pNode->set_owner(pOwner);

	for (int n=0; n<pNode->get_child_count(); n++)
	{
		SetOwnerRecursive(pNode->get_child(n), pOwner);
	}
}


bool LSceneSaver::SaveScene(Node * pNode, String szFilename)
{
	// godot needs owner to be set on nodes that are to be saved as part of a packed scene
	SetOwnerRecursive(pNode, pNode);

	//PackedScene ps;
	// reference should self delete on exiting func .. check!
	Ref<PackedScene> ps = memnew(PackedScene);
	//Ref<PackedScene> ref_ps = &ps;

	ps->pack(pNode);

	ResourceSaver rs;
	rs.save(szFilename, ps);

	// reimport
//	ResourceLoader::import(szFilename);

	return true;
}
