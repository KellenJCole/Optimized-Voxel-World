#pragma once

#include <vector>
#include <map>
#include <set>
#include <shared_mutex>

#include "h/Rendering/Utility/BlockFaceBitmask.h"
#include <h/Terrain/GreedyAlgorithm.h>
#include <h/external/glm/glm.hpp>
#include "h/Terrain/ProcGen/ProcGen.h"
#include "h/Rendering/Shader.h"
#include "h/Rendering/Utility/GLErrorCatcher.h"

class WorldManager;
class Chunk {
public:
	// Constructor
	Chunk();

	// Setters
	inline void setProcGenReference(ProcGen* pg) { proceduralAlgorithm = pg; }
	inline void setWorldReference(WorldManager* wm) { world = wm; }
	inline void setChunkCoords(int cx, int cz) { chunkX = cx; chunkZ = cz; }
	void setLod(int detailLvl);

	// Getters
	const MeshUtils::MeshGraph& getMeshGraph(BlockFace faceType) { return greedyAlgorithm.getMeshGraph(faceType); }
	inline int getChunkX() const { return chunkX; }
	inline int getChunkZ() const { return chunkZ; }
	inline int getCurrentLod() const { return detailLevel; }
	BlockID getBlockAt(int worldX, int worldY, int worldZ);
	BlockID getBlockAt(int worldX, int worldY, int worldZ, BlockFace face, int sourceLod);
	std::vector<BlockID> getCurrChunkVec();

	// Procedurally generate chunk and form meshes
	void generateChunk();
	void startMeshing();
	BlockFaceBitmask cullFaces(int blockIndex, std::vector<uint16_t>& neighborCache);
	void markNeighborsCheck(int neighborIndex, BlockFace face, std::vector<uint16_t>& neighborCache);
	bool hasNeighborCheckBeenPerformed(int blockIndex, BlockFace face, std::vector<uint16_t>& neighborCache);
	void greedyMesh();
	std::map<BlockFace, std::vector<unsigned int>>* getVisByFaceType() { return &visByFaceType; }

	// Chunk modification
	bool breakBlock(int localX, int localY, int localZ);
	bool placeBlock(int localX, int localY, int localZ, BlockID blockToPlace);

	// Change LOD
	void convertLOD(int newLod);

	// Clear VisByFaceType (might need to do other things, this should be thought about harder)
	void unload();

	bool meshExists = false;
	bool meshDirty = false;
	int resolutionXZ, resolutionY;
	int highestOccupiedIndex;
	int detailLevel;

private:
	glm::ivec3 expandChunkCoords(int flatIndex);

	int neighborOffsets[6];

	void setChunkLodMapVectorSize();

	ProcGen* proceduralAlgorithm;
	GreedyAlgorithm greedyAlgorithm;
	std::map<int, std::vector<BlockID>> chunkLodMap;
	std::map<BlockFace, std::vector<unsigned int>> visByFaceType;
	WorldManager* world;

	bool hasBeenGenerated[7];

	// Basic variables related to level of detail
	int blockResolution;
	int chunkX, chunkZ;
};