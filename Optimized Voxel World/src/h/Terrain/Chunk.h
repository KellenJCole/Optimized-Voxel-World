#pragma once
#include <vector>
#include <map>
#include <set>
#include <h/Terrain/GreedyAlgorithm.h>
#include <h/glm/glm.hpp>
#include "h/Terrain/ProcGen/ProcGen.h"
#include <shared_mutex>
#include <stack>

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
	// Constructor
	Chunk();

	// Setters
	void setWorldReference(WorldManager* wm);
	void setProcGenReference(ProcGen* pg);
	void setLod(int detailLvl);
	void setChunkCoords(int cx, int cz);

	// Getters
	std::vector<std::pair<std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>>, int>> getGreedyMeshByFaceType(int faceType);
	int getChunkX() { return chunkX; }
	int getChunkZ() { return chunkZ; }
	int getCurrentLod();
	unsigned char getBlockAt(int worldX, int worldY, int worldZ, int face, int prevLod);
	std::vector<unsigned char> getCurrChunkVec();
	// Procedurally generate chunk and form meshes
	void generateChunk();
	void generateChunkMeshes();

	// Chunk modification
	void breakBlock(int localX, int localY, int localZ);
	void placeBlock(int localX, int localY, int localZ, unsigned char blockToPlace);

	// Switch LOD
	void convertLOD(int newLod);

	// Clear VisByFaceType (might need to do other things, this should be thought about harder)
	void unload();
private:
	inline int convert3DCoordinatesToFlatIndex(int x, int y, int z) {
		return x + (z * resolutionXZ) + (y * resolutionXZ * resolutionXZ);
	}
	glm::ivec3 convertFlatIndexTo3DCoordinates(int flatIndex);

	// Face culling
	unsigned char checkNeighbors(int blockIndex, std::vector<uint16_t>& neighborCache); // Returns a bitmask representing which faces are visible and which are not
	int neighborOffsets[6];
	void markNeighborsCheck(int neighborIndex, int face, std::vector<uint16_t>& neighborCache);
	bool hasNeighborCheckBeenPerformed(int blockIndex, int face, std::vector<uint16_t>& neighborCache);


	void greedyMesh();

	void setChunkLodMapVectorSize();

	ProcGen* proceduralAlgorithm;
	GreedyAlgorithm ga;
	std::map<int, std::vector<unsigned char>> chunkLodMap; // delete this upon unload
	std::map<BlockFace, std::vector<unsigned int>> visByFaceType; // keep before greedy meshing
	WorldManager* world;
	BlockFace iFaces[6];

	bool hasBeenGenerated[7];
	std::stack<std::pair<unsigned int, unsigned char>> chunkEditsStack; // Keeps track of the edits made to chunks for saving purposes so they never have to have the entire chunk saved.

	// Basic variables related to level of detail
	int detailLevel;
	int blockResolution; // 1 >> detailLevel
	int chunkX, chunkZ;
	int resolutionXZ, resolutionY;
	int highestOccupiedIndex;
};