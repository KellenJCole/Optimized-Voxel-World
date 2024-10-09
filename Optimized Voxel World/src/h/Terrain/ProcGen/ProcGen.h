#pragma once
#include <vector>
#include <string>
#include "h/Constants.h"
#include "h/FastNoise-master/FastNoise.h"
#include "h/glm/glm.hpp"
#include <set>
#include <map>
#include <mutex>
#include <fstream>

class ProcGen {
public:
	ProcGen();
	int generateChunk(std::vector<unsigned char>& chunkVec, std::pair<int, int> chunkCoordPair, int levelOfDetail);
	void setNoiseState(std::vector<float> state);
	void setRandomNoiseState();
private:
	std::vector<std::vector<float>> getHeightMap(std::pair<int, int> chunkCoordPair);
	FastNoise heightMapNoise[4];
	float heightMapWeights[4];
	int heightAmplitude;

	void setLODVariables(int levelOfDetail);
	inline int convert3DCoordinatesToFlatIndex(int x, int y, int z) {
		return x + (z * resolutionXZ) + (y * resolutionXZ * resolutionXZ);
	}

	std::mutex procGenMutex;

	int blockResolution;
	int resolutionXZ, resolutionY;
};