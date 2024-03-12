#pragma once
#include <vector>
#include "h/FastNoise-master/FastNoise.h"
#include "h/glm\glm.hpp"
#include <set>
#include <map>

class ProcGen {
public:
	ProcGen();
	void generateChunk(std::vector<unsigned char>& chunkVec, std::pair<int, int> chunkCoordPair);
	void setNoiseState(std::vector<float> state);
private:
	std::vector<std::vector<float>> getHeightMap(std::pair<int, int> chunkCoordPair);
	FastNoise heightMapNoise[4];
	float heightMapWeights[4];

	int heightAmplitude;
	glm::ivec3 convertFlatIndexTo3DCoordinates(int flatIndex);
	int convert3DCoordinatesToFlatIndex(int x, int y, int z);
	std::map<std::pair<int, int>, std::pair<float, float>> multiChunksMap;
};