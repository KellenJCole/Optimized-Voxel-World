#pragma once
#include <unordered_map>
#include <set>
#include <future>
#include <mutex>
#include "h/Rendering/Renderer.h"
#include "h/Terrain/ChunkLoader.h"
#include "h/Terrain/Chunk.h"
#include "h/Rendering/Texture.h"
#include "h/Rendering/Camera.h"

struct PairHash {
	size_t operator()(const std::pair<int, int>& pair) const {
		auto hash1 = std::hash<int>{}(pair.first);
		auto hash2 = std::hash<int>{}(pair.second);
		return hash1 ^ hash2;
	}
};


using ChunkCoordPair = std::pair<int, int>;
class WorldManager {
public:
	WorldManager();
	bool initialize();
	void update();
	void updateRenderChunks(int originX, int originZ, int renderRadius);
	void render();
	void cleanup();
	void setCamAndShaderPointers(Shader* sha, Camera* cam);
	int getBlockAtGlobal(int worldX, int worldY, int worldZ, bool fromSelf);
	void switchRenderMethod();
	void breakBlock(int worldX, int worldY, int worldZ);
private:
	void addQuadVerticesAndIndices(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>> quad, ChunkCoordPair chunkCoords, int faceType, int offset);
	void genMeshForSingleChunk(ChunkCoordPair key);
	void markChunkReadyForRender(std::pair<int, int> key);
	void updateMesh(ChunkCoordPair key);

	void loadChunksAsync(const std::vector<std::pair<int, int>>& loadChunks);
	void unloadChunks(const std::vector<std::pair<int, int>>& loadChunks);

	void updateRenderBuffers();

	glm::vec3 calculatePosition(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>& q, int corner, int faceType, ChunkCoordPair cxcz, int offset);
	glm::vec2 calculateTexCoords(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>& q, int corner);

	Renderer renderer;
	ChunkLoader chunkLoader;
	Texture blockTextureArray;

	std::unordered_map<ChunkCoordPair, Chunk, PairHash> worldMap;
	std::unordered_map<ChunkCoordPair, std::vector<Vertex>, PairHash> verticesByChunk;
	std::unordered_map<ChunkCoordPair, std::vector<unsigned int>, PairHash> indicesByChunk;
	std::unordered_set<ChunkCoordPair,  PairHash> preparedChunks;

	// SHADER CAMERA LIGHTS
	Shader* shader;
	Camera* camera;
	glm::vec3 lightPos;
	glm::vec3 lightColor;

	// MULTITHREAD
	std::future<void> loadFuture;
	std::mutex mtx;
	std::mutex chunkUpdateMtx;
	std::condition_variable chunkCondition;

	float lastFrustumCheck;
	std::atomic<bool> updatedRenderChunks;
	std::atomic<bool> chunkUpdate;
	int renderRadius;
	std::atomic<bool> stopAsync;

};