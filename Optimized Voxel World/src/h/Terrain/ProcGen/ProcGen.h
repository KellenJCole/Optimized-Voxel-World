#pragma once

#include "h/Terrain/Utility/ChunkUtils.h"
#include "h/Terrain/Utility/BlockID.h"
#include "h/external/FastNoise-master/FastNoise.h"
#include "h/external/glm/glm.hpp"

#include <vector>
#include <string>
#include <set>
#include <map>
#include <mutex>
#include <fstream>

class ProcGen {
public:
	ProcGen();
	int generateChunk(std::vector<BlockID>& chunkVector, ChunkUtils::ChunkCoordPair chunkCoordPair, int levelOfDetail);
	void setNoiseState(std::vector<float> state);
	void setRandomNoiseState();
private:
	std::vector<std::vector<float>> getHeightMap(ChunkUtils::ChunkCoordPair chunkCoordPair);
	FastNoise heightMapNoise[4];
	float heightMapWeights[4];
	int heightAmplitude;

	void setLodVariables(int levelOfDetail);
	inline int convert3DCoordinatesToFlatIndex(int x, int y, int z) {
		return x + (z * resolutionXZ) + (y * resolutionXZ * resolutionXZ);
	}

	std::mutex procGenMutex;

	int blockResolution;
	int resolutionXZ, resolutionY;
};