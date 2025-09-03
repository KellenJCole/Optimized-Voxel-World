#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>
#include "h/Terrain/Utility/ChunkUtils.h"
#include "h/Terrain/Utility/BlockID.h"
#include "h/Rendering/Utility/MeshUtils.h"
#include "h/Rendering/Utility/BlockTextureLUT.h"

class GreedyAlgorithm {
public:
	GreedyAlgorithm();

	void populatePlanes(
		std::map<BlockFace, std::vector<unsigned int>>& visibleBlockIndexes, 
		std::vector<BlockID>& chunkData, 
		int levelOfDetail
	);

	void firstPassOn(BlockFace f);

	const MeshUtils::MeshGraph& getMeshGraph(BlockFace f) const;

	void unload();
private:

	std::array<std::vector<MeshUtils::Plane>, MeshUtils::FACE_COUNT> _planes;
	MeshUtils::FaceMeshGraphs _greedyGraphs;

	void assignCoordinates(BlockFace face, 
		int localX, int localY, int localZ, 
		int& u, int& v, int& sliceIndex);
};