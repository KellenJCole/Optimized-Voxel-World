#include "h/Terrain/ProcGen/ProcGen.h"
#include <time.h>
#include <iostream>

#define CHUNK_WIDTH 16
#define CHUNK_DEPTH 16
#define CHUNK_HEIGHT 256

ProcGen::ProcGen() {
	srand(time(NULL));
	for (int i = 0; i < 4; i++) {
		heightMapNoise[i].SetFractalType(FastNoise::FractalType::FBM);
		heightMapNoise[i].SetNoiseType(FastNoise::NoiseType::Perlin);
		heightMapNoise[i].SetInterp(FastNoise::Interp::Quintic);
		heightMapNoise[i].SetFractalOctaves((FN_DECIMAL)(4 - i) * 2);
		heightMapNoise[i].SetFractalLacunarity((FN_DECIMAL)2);
		heightMapNoise[i].SetFractalGain((FN_DECIMAL)0.5);
	}
	
	heightMapNoise[0].SetFrequency((FN_DECIMAL)0.007);
	heightMapNoise[1].SetFrequency((FN_DECIMAL)0.058);
	heightMapNoise[2].SetFrequency((FN_DECIMAL)0.21);
	heightMapNoise[3].SetFrequency((FN_DECIMAL)0.67);

	heightMapWeights[0] = .5f;
	heightMapWeights[1] = .25f;
	heightMapWeights[2] = .15f;
	heightMapWeights[3] = .1f;

	heightAmplitude = 80;
}

/*
Dirt = 1
Grass = 2
Stone = 3
Bedrock = 4
Sand = 5
*/

void ProcGen::generateChunk(std::vector<unsigned char>& chunkVec, std::pair<int, int> chunkCoordPair) {
	std::vector<std::vector<float>> hm = getHeightMap(chunkCoordPair);
	std::pair<int, int> multiChunksKey = std::make_pair((8192 + chunkCoordPair.first) / 16384, (8192 + chunkCoordPair.second) / 16384);

	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			float min = multiChunksMap[multiChunksKey].first;
			float max = multiChunksMap[multiChunksKey].second;
			float height = (hm[x][z] - min) / (max - min);
			int convertHeight = height * heightAmplitude;
			for (int y = 0; y < 256; y++) {
				int index = convert3DCoordinatesToFlatIndex(x, y, z);
				if (y <= convertHeight && height < 0.2) {
					chunkVec[index] = 4;
				}
				else if (y <= convertHeight && height < 0.4) {
					chunkVec[index] = 3;
				}
				else if (y <= convertHeight && height < 0.5) {
					chunkVec[index] = 1;
				}
				else if (y <= convertHeight) {
					chunkVec[index] = 2;
				}
				else if (y == 30 && y > convertHeight) {
					chunkVec[index] = 6;
				}
				else {
					chunkVec[index] = 0;
				}
			}
		}
	}
}

std::vector<std::vector<float>> ProcGen::getHeightMap(std::pair<int, int> chunkCoordPair) {
	float minTheseChunks, maxTheseChunks;
	std::pair<int, int> multiChunksKey = std::make_pair((8192 + chunkCoordPair.first) / 16384, (8192 + chunkCoordPair.second) / 16384);
	if (multiChunksMap.find(multiChunksKey) != multiChunksMap.end()) {
		minTheseChunks = multiChunksMap[multiChunksKey].first;
		maxTheseChunks = multiChunksMap[multiChunksKey].second;
	}
	else {
		multiChunksMap[multiChunksKey].first = NULL;
		multiChunksMap[multiChunksKey].second = NULL;
		minTheseChunks = NULL;
		maxTheseChunks = NULL;
	}

	std::vector<std::vector<float>> heightMap(16, std::vector<float>(16));
	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			float x1 = (x + (chunkCoordPair.first * 16));
			float z1 = (z + (chunkCoordPair.second * 16));

			float val = (float)(heightMapWeights[0] * heightMapNoise[0].GetValue(x1, z1) + heightMapWeights[1] * heightMapNoise[1].GetValue(x1, z1) + heightMapWeights[2] * heightMapNoise[2].GetValue(x1, z1) + heightMapWeights[3] * heightMapNoise[3].GetNoise(x1, z1));

			if (minTheseChunks == NULL) {
				multiChunksMap[multiChunksKey].first = val;
				multiChunksMap[multiChunksKey].second = val;
				minTheseChunks = val;
				maxTheseChunks = val;
			}

			if (val > maxTheseChunks) {
				maxTheseChunks = val;
				multiChunksMap[multiChunksKey].second = val;
			}
			if (val < minTheseChunks) {
				minTheseChunks = val;
				multiChunksMap[multiChunksKey].first = val;
			}

			heightMap[x][z] = val;
		}
	}

	return heightMap;
}

void ProcGen::setNoiseState(std::vector<float> state) {
	multiChunksMap.clear();

	heightMapNoise[0].SetFrequency(state[0]);
	heightMapNoise[1].SetFrequency(state[1]);
	heightMapNoise[2].SetFrequency(state[2]);
	heightMapNoise[3].SetFrequency(state[3]);

	heightMapNoise[0].SetFractalOctaves((int)state[4]);
	heightMapNoise[1].SetFractalOctaves((int)state[5]);
	heightMapNoise[2].SetFractalOctaves((int)state[6]);
	heightMapNoise[3].SetFractalOctaves((int)state[7]);

	heightMapNoise[0].SetFractalLacunarity(state[8]);
	heightMapNoise[1].SetFractalLacunarity(state[9]);
	heightMapNoise[2].SetFractalLacunarity(state[10]);
	heightMapNoise[3].SetFractalLacunarity(state[11]);

	heightMapNoise[0].SetFractalGain(state[12]);
	heightMapNoise[1].SetFractalLacunarity(state[13]);
	heightMapNoise[2].SetFractalLacunarity(state[14]);
	heightMapNoise[3].SetFractalLacunarity(state[15]);

	heightMapWeights[0] = state[16];
	heightMapWeights[1] = state[17];
	heightMapWeights[2] = state[18];
	heightMapWeights[3] = state[19];

	heightAmplitude = state[20];
}

glm::ivec3 ProcGen::convertFlatIndexTo3DCoordinates(int flatIndex) {
	glm::ivec3 returnVec;
	returnVec.x = flatIndex & 0x0F; // Same as flatIndex % 16
	returnVec.y = flatIndex >> 8; // Same as flatIndex / 256
	returnVec.z = (flatIndex & 0xFF) >> 4; // Same as (flatIndex % 256) / 16
	return returnVec;
}

int ProcGen::convert3DCoordinatesToFlatIndex(int x, int y, int z) {
	return (y << 8) | (z << 4) | x;
}
