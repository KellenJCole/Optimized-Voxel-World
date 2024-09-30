#include "h/Terrain/ProcGen/ProcGen.h"
#include <time.h>
#include <iostream>

#define CHUNK_WIDTH 64
#define CHUNK_DEPTH 64
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
	
	heightMapNoise[0].SetFrequency((FN_DECIMAL)0.005);
	heightMapNoise[1].SetFrequency((FN_DECIMAL)0.01);
	heightMapNoise[2].SetFrequency((FN_DECIMAL)0.02);
	heightMapNoise[3].SetFrequency((FN_DECIMAL)0.04);

	heightMapWeights[0] = .5f;
	heightMapWeights[1] = .25f;
	heightMapWeights[2] = .15f;
	heightMapWeights[3] = .1f;

	heightAmplitude = 80;
}

void ProcGen::generateChunk(std::vector<unsigned char>& chunkVec, std::pair<int, int> chunkCoordPair, int levelOfDetail) {
	std::lock_guard<std::mutex> procGenLock(procGenMutex);
	int positionMult = pow(2, levelOfDetail);
	std::vector<std::vector<float>> hm = getHeightMap(chunkCoordPair, levelOfDetail);

	const float globalMin = -1.0f; // Minimum possible Perlin noise value
	const float globalMax = 1.0f;  // Maximum possible Perlin noise value

	int resolution = 64 / positionMult;
	int heightResolution = 256 / positionMult;

	for (int x = 0; x < resolution; x++) {
		for (int z = 0; z < resolution; z++) {
			float normalizedHeight = (hm[x][z] - globalMin) / (globalMax - globalMin);
			float convertHeight = normalizedHeight * heightAmplitude;
			int highestIndex = -1;

			for (int y = 0; y < heightResolution; y++) {
				int index = convert3DCoordinatesToFlatIndex(x, y, z, levelOfDetail);
				int worldY = y * positionMult;

				if (worldY <= convertHeight) {
					
					if (y <= convertHeight && normalizedHeight < 0.2f) {
						chunkVec[index] = 4; // Bedrock
					}
					else if (y <= convertHeight && normalizedHeight < 0.4f) {
						chunkVec[index] = 3; // Stone
					}
					else if (y <= convertHeight) {
						chunkVec[index] = 1; // Dirt
					}
					highestIndex = index;
				}
				else {
					chunkVec[index] = 0; // Air
				}
			}

			if (highestIndex != -1 && chunkVec[highestIndex] == 1) {
				chunkVec[highestIndex] = 2; // Grass
			}
		}
	}
}


std::vector<std::vector<float>> ProcGen::getHeightMap(std::pair<int, int> chunkCoordPair, int levelOfDetail) {

	int resolution = 64 / (pow(2, levelOfDetail)); // resolution is halved for each LOD
	std::vector<std::vector<float>> heightMap(resolution, std::vector<float>(resolution));

	int chunkX = chunkCoordPair.first;
	int chunkZ = chunkCoordPair.second;
	for (int x = 0; x < resolution; x++) {
		for (int z = 0; z < resolution; z++) {
			// Calculate world-space positions for this heightmap point.
			int x1 = (x * (int)pow(2, levelOfDetail)) + (chunkX * 64);
			int z1 = (z * (int)pow(2, levelOfDetail)) + (chunkZ * 64);

			float val;
			if (levelOfDetail == 0) {
				// Simple height calculation with full resolution
				val = static_cast<float>(heightMapWeights[0] * heightMapNoise[0].GetValue(x1, z1) +
					heightMapWeights[1] * heightMapNoise[1].GetValue(x1, z1) +
					heightMapWeights[2] * heightMapNoise[2].GetValue(x1, z1) +
					heightMapWeights[3] * heightMapNoise[3].GetValue(x1, z1));
			}
			else {
				// For higher LODs, sample multiple points and average them
				float sum = 0;
				int numSamples = pow(2, levelOfDetail); // Number of samples to take in x and z
				for (int dx = 0; dx < numSamples; dx++) {
					for (int dz = 0; dz < numSamples; dz++) {
						float sampleX = x1 + dx;
						float sampleZ = z1 + dz;
						sum += (float)(heightMapWeights[0] * heightMapNoise[0].GetValue(sampleX, sampleZ) +
							heightMapWeights[1] * heightMapNoise[1].GetValue(sampleX, sampleZ) +
							heightMapWeights[2] * heightMapNoise[2].GetValue(sampleX, sampleZ) +
							heightMapWeights[3] * heightMapNoise[3].GetValue(sampleX, sampleZ));
					}
				}
				val = sum / (numSamples * numSamples); // Average the samples
			}

			heightMap[x][z] = val;
		}
	}

	return heightMap;
}

void ProcGen::setNoiseState(std::vector<float> state) {

	for (int i = 0; i < 4; i++) {
		heightMapNoise[i].SetFrequency(state[i]);
		heightMapNoise[i].SetFractalOctaves((int)state[i + 4]);
		heightMapNoise[i].SetFractalLacunarity(state[i + 8]);
		heightMapNoise[i].SetFractalGain(state[i + 12]);
		heightMapWeights[i] = state[i + 16];
	}
	heightAmplitude = state[20];
}

int ProcGen::convert3DCoordinatesToFlatIndex(int x, int y, int z, int levelOfDetail) {
	int width = CHUNK_WIDTH / pow(2, levelOfDetail);
	int depth = CHUNK_DEPTH / pow(2, levelOfDetail);
	int height = CHUNK_HEIGHT / pow(2, levelOfDetail);

	return x + (z * width) + (y * width * depth);
}


