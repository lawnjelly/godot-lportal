#pragma once

class LSceneSaver
{
public:
	bool SaveScene(Node * pNode, String szFilename);

private:
	void SetOwnerRecursive(Node * pNode, Node * pOwner);
};
