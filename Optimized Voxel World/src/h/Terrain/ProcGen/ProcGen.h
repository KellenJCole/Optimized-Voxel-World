#pragma once
#include <vector>
#include <string>
#include "h/FastNoise-master/FastNoise.h"
#include "h/glm/glm.hpp"
#include <set>
#include <map>
#include <mutex>
#include <fstream>

class ProcGen {
public:
	ProcGen();
	void generateChunk(std::vector<unsigned char>& chunkVec, std::pair<int, int> chunkCoordPair, int levelOfDetail);
	void setNoiseState(std::vector<float> state);
	void setRandomNoiseState();
private:
	std::vector<std::vector<float>> getHeightMap(std::pair<int, int> chunkCoordPair);
	void setBlockResolution(int levelOfDetail);
	FastNoise heightMapNoise[4];
	float heightMapWeights[4];

	int heightAmplitude;
	int convert3DCoordinatesToFlatIndex(int x, int y, int z);

	std::mutex procGenMutex;

	int blockResolution;
};