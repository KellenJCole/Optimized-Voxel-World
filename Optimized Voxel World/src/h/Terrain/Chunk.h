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
	void setWorldReference(WorldManager* wm) { world = wm; }
	void setChunkCoords(int cx, int cz) { chunkX = cx; chunkZ = cz; }

	// Getters
	const MeshUtils::MeshGraph& getMeshGraph(BlockFace faceType) { return greedyAlgorithm.getMeshGraph(faceType); }
	int getChunkX() const { return chunkX; }
	int getChunkZ() const { return chunkZ; }
	int getCurrentLod() const { return detailLevel; }
	BlockID getBlockAt(int worldX, int worldY, int worldZ);
	BlockID getBlockAt(int worldX, int worldY, int worldZ, BlockFace face, int sourceLod);
	std::vector<BlockID> getCurrChunkVec() { return chunkLodMap[detailLevel]; }

	// Procedurally generate chunk and form meshes
	void generateChunk(ProcGen& proceduralGenerator);
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
	void setLodVariables(int detailLvl); // sets variables related to level of detail
	void convertLOD(int lod); // calls setLod & regenerates chunk

	// Clear VisByFaceType (might need to do other things, this should be thought about harder)
	void unload();

private:
	glm::ivec3 expandChunkCoords(int flatIndex) const;

	int neighborOffsets[6];

	GreedyAlgorithm greedyAlgorithm;
	std::map<int, std::vector<BlockID>> chunkLodMap;
	std::map<BlockFace, std::vector<unsigned int>> visByFaceType;
	WorldManager* world;

	int chunkX, chunkZ;
	int resolutionXZ, resolutionY;
	int detailLevel;
	int highestOccupiedIndex;
};