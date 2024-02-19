#pragma once
#include <vector>
#include <map>
#include <set>
#include <h/Terrain/GreedyGraph.h>
#include <h/glm/glm.hpp>
#include <h/FastNoise-master/FastNoise.h>

enum BlockFace {
	NEG_X = 1 << 0, // 0b000001
	POS_X = 1 << 1, // 0b000010
	NEG_Y = 1 << 2, // 0b000100
	POS_Y = 1 << 3, // 0b001000
	NEG_Z = 1 << 4, // 0b010000
	POS_Z = 1 << 5  // 0b100000
};

class WorldManager;
class Chunk {
public:
	Chunk();
	void generateChunk();
	void setWholeChunkMeshes();
	std::vector<glm::vec3> getMeshByFaceType(int faceType);
	std::vector<std::pair<GreedyGraph, int>> getGreedyMeshByFaceType(int faceType);
	void setChunkCoords(int cx, int cz);
	int getChunkX() { return chunkX; }
	int getChunkZ() { return chunkZ; }
	int getBlockAt(int worldX, int worldY, int worldZ);
	void setWorldReference(WorldManager* wm);
	void unload();
private:
	int convert3DCoordinatesToFlatIndex(int x, int y, int z);
	glm::ivec3 convertFlatIndexTo3DCoordinates(int flatIndex);
	void greedyMesh();
	void firstPassOn(BlockFace f);
	void populatePlanes();

	unsigned char checkNeighbors(int blockIndex); // Returns a bitmask representing which faces are visible and which are not

	std::vector<unsigned char> chunk; // delete this upon unload
	std::map<BlockFace, std::vector<unsigned int>> visByFaceType; // keep before greedy meshing
	std::map<BlockFace, std::vector<std::vector<std::vector<unsigned char>>>> planes;
	int chunkX, chunkZ;
	WorldManager* world;
	FastNoise noise;

	std::vector<std::pair<GreedyGraph, int>> Greedy_Graphs[6];
};