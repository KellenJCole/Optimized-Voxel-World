#include "h/Terrain/WorldManager.h"
#include <vector>
#include <iostream>

WorldManager::WorldManager() : lightPos(0.0f, 2000.f, 0.0f),
								lightColor(1.0f, 1.0f, 1.0f) {
	srand(glfwGetTime());
}

bool WorldManager::initialize() {
	currentlyLoadingChunks = false;

	if (!renderer.initialize()) {
		std::cout << "Renderer failed to initialize in World Manager\n";
		return false;
	}

	textureLibrary.setTextures();

	return true;
}

void WorldManager::update() {
	glm::vec3 pos = camera->getCameraPos();
	lightPos = { pos.x ,300 , pos.z };
	if (currentlyLoadingChunks && loadFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
		genAllFaces();
		currentlyLoadingChunks = false;
	}
}

void WorldManager::updateRenderChunks(int originX, int originZ) {
	if (!currentlyLoadingChunks) {
		auto unloadAndLoadVectors = chunkLoader.getUnloadAndLoadList(originX, originZ, 20);
		unloadChunks(unloadAndLoadVectors.first);

		if (std::get<1>(unloadAndLoadVectors).size() > 0) {
			currentlyLoadingChunks = true;
			loadFuture = std::async(std::launch::async, &WorldManager::loadChunksAsync, this, unloadAndLoadVectors.second);
		}
	}
}

void WorldManager::loadChunksAsync(const std::vector<std::pair<int, int>>& loadChunks) {
	std::vector<std::pair<int, int>> newChunks;
	for (const auto& key : loadChunks) {
		auto result = worldMap.emplace(key, Chunk());
		if (result.second) { // Chunk was newly inserted
			auto& newChunk = result.first->second;
			newChunk.setChunkCoords(key.first, key.second);
			newChunk.generateChunk();
			newChunk.setWorldReference(this);
		}
		newChunks.push_back(key);
		drawChunks.insert(key);
	}


	for (auto& key : newChunks) {
		worldMap[key].setWholeChunkMeshes();

		for (int i = 0; i < 6; i++) {
			meshesByFace[i][key] = worldMap[key].getMeshByFaceType(i);
		}
	}
}

void WorldManager::unloadChunks(const std::vector<std::pair<int, int>>& unloadChunks) {
	for (const auto& key : unloadChunks) {
		auto it = worldMap.find(key);
		if (it != worldMap.end()) {
			it->second.unload();
			worldMap.erase(it); 
		}
		drawChunks.erase(key);
		for (int i = 0; i < 6; i++) {
			meshesByFace[i].erase(key);
		}
	}
	for (const auto& key : createdForMeshes) {
		auto it = worldMap.find(key);
		if (it != worldMap.end()) {
			it->second.unload();
			worldMap.erase(it);
		}
	}
}

void WorldManager::genAllFaces() {
	for (int i = 0; i < 6; i++) {
		allFaces[i].clear();
	}

	for (const auto& chunkCoord : drawChunks) {
		for (int i = 0; i < 6; i++) {
			auto& chunkFaces = meshesByFace[i][chunkCoord];
			allFaces[i].insert(allFaces[i].end(), chunkFaces.begin(), chunkFaces.end());
		}
	}
}

void WorldManager::render() {
	Texture* blockTexture;
	blockTexture = textureLibrary.getTexture(3);

	GLCall(glActiveTexture(GL_TEXTURE0));
	blockTexture->Bind();

	shader->setUniform4fv("view", (glm::mat4&)camera->getView());
	shader->setUniform4fv("projection", (glm::mat4&)camera->getProjection());

	shader->setVec3("lightPos", lightPos);
	shader->setVec3("lightColor", lightColor);
	shader->setVec3("viewPos", (glm::vec3&)camera->getCameraPos());

	for (int i = 0; i < 6; i++) {
		renderer.instancedRenderByFace(allFaces[i], i, 1);
	}

}

void WorldManager::setCamAndShaderPointers(Shader* sha, Camera* cam) {
	shader = sha;
	camera = cam;
}

void WorldManager::cleanup() {
	renderer.cleanup();
}

int WorldManager::getBlockAtGlobal(int worldX, int worldY, int worldZ) {
	int chunkX = (worldX >> 4);
	int chunkZ = (worldZ >> 4);
	auto it = worldMap.find({ chunkX, chunkZ });
	if (it != worldMap.end()) {
		return it->second.getBlockAt(worldX, worldY, worldZ);
	}
	else {
		std::pair<int, int> key = { chunkX, chunkZ };
		worldMap[key] = Chunk();
		worldMap[key].setChunkCoords(key.first, key.second);
		worldMap[key].generateChunk();
		worldMap[key].setWorldReference(this);
		createdForMeshes.insert(key);
		return worldMap[key].getBlockAt(worldX, worldY, worldZ);
	}
}