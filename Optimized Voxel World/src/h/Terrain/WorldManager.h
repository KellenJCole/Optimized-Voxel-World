#pragma once
#include <unordered_map>
#include <set>
#include <list>
#include <future>
#include "h/Rendering/Renderer.h"
#include "h/Terrain/ChunkLoader.h"
#include "h/Terrain/Chunk.h"
#include "h/Rendering/TextureLibrary.h"
#include "h/Rendering/Camera.h"
#include "h/Rendering/Shader.h"

struct PairHash {
	size_t operator()(const std::pair<int, int>& pair) const {
		auto hash1 = std::hash<int>{}(pair.first);
		auto hash2 = std::hash<int>{}(pair.second);
		return hash1 ^ hash2;
	}
};

class WorldManager {
public:
	WorldManager();
	bool initialize();
	void update();
	void updateRenderChunks(int originX, int originZ);
	void render();
	void cleanup();
	void setCamAndShaderPointers(Shader* sha, Camera* cam);
	int getBlockAtGlobal(int worldX, int worldY, int worldZ);
private:
	void genAllFaces();
	void loadChunksAsync(const std::vector<std::pair<int, int>>& loadChunks);
	void unloadChunks(const std::vector<std::pair<int, int>>& unloadChunks);
	Renderer renderer;
	ChunkLoader chunkLoader;
	TextureLibrary textureLibrary;
	std::unordered_set<std::pair<int, int>, PairHash> drawChunks;
	std::unordered_set <std::pair<int, int>, PairHash> createdForMeshes;
	std::unordered_map<std::pair<int, int>, Chunk, PairHash> worldMap;
	std::map<std::pair<int, int>, std::vector<glm::vec3>> meshesByFace[6];
	std::vector<glm::vec3> allFaces[6];
	Shader* shader;
	Camera* camera;
	glm::vec3 lightPos;
	glm::vec3 lightColor;

	std::future<void> loadFuture;
	bool currentlyLoadingChunks;

};