#pragma once

#include <vector>

#include <glm/glm.hpp>

#include <assimp/scene.h>

#include "Mesh.h"


class MeshModel
{
public:
	MeshModel();
	MeshModel(std::vector<Mesh> newMeshList);
	size_t getMeshCount();
	Mesh* getMesh(size_t index);
	glm::mat4 getModel();
	void setModel(glm::mat4 newModel);
	void destroyMeshModel();
	~MeshModel();

public:
	static std::vector<std::string> loadMaterials(const aiScene* scene);

private:
	std::vector<Mesh> meshList;
	glm::mat4 model;

};
