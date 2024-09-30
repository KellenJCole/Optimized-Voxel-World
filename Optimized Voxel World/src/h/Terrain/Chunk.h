#pragma once
#include <vector>
#include <map>
#include <set>
#include <h/Terrain/GreedyAlgorithm.h>
#include <h/glm/glm.hpp>
#include "h/Terrain/ProcGen/ProcGen.h"
#include <shared_mutex>

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
	// Setup
	Chunk();
	void generateChunk(int detailLvl);
	void setWorldReference(WorldManager* wm);
	void setProcGenReference(ProcGen* pg);
	void unload();

	bool generated();

	// Coordinate functions
	void setChunkCoords(int cx, int cz);
	int getChunkX() { return chunkX; }
	int getChunkZ() { return chunkZ; }
	void setWholeChunkMeshes();
	std::vector<std::pair<std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>>, int>> getGreedyMeshByFaceType(int faceType);
	int getBlockAt(int worldX, int worldY, int worldZ, bool boundaryCall, bool calledFromGlobal, int face, int prevLod, bool recursion);
	void breakBlock(int localX, int localY, int localZ);
	void placeBlock(int localX, int localY, int localZ, unsigned char blockToPlace);
	int getLod();
	~Chunk();
private:
	ProcGen* proceduralAlgorithm;
	int convert3DCoordinatesToFlatIndex(int x, int y, int z);
	glm::ivec3 convertFlatIndexTo3DCoordinates(int flatIndex);
	void greedyMesh();
	unsigned char checkNeighbors(int blockIndex); // Returns a bitmask representing which faces are visible and which are not
	int convertWorldCoordToChunkCoord(int worldCoord);
	int detailLevel;
	/*
	For LODS
	At detail level 0, each block is represented as is, at 1x1x1 scale
	At detail level 1, average block of each 2x2x2 chunk of blocks is averaged to form one big block
	*/

	GreedyAlgorithm ga;
	std::vector<unsigned char> chunk; // delete this upon unload
	std::map<BlockFace, std::vector<unsigned int>> visByFaceType; // keep before greedy meshing
	int chunkX, chunkZ;
	WorldManager* world;
	bool hasBeenGenerated;
	BlockFace iFaces[6];
};