#include "h/Terrain/ProcGen/ProcGen.h"
#include <time.h>
#include <iostream>

ProcGen::ProcGen() : 
	heightAmplitude(80),
	blockResolution(0),
	resolutionXZ(0),
	resolutionY(0)
{
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
}

int ProcGen::generateChunk(std::vector<BlockID>& chunkVec, std::pair<int, int> chunkCoordPair, int levelOfDetail) {
	std::lock_guard<std::mutex> procGenLock(procGenMutex);
	setLODVariables(levelOfDetail);
	std::vector<std::vector<float>> hm = getHeightMap(chunkCoordPair);

	const float globalMin = -1.0f; // Minimum possible Perlin noise value
	const float globalMax = 1.0f;  // Maximum possible Perlin noise value
	
	int highestOccupiedIndex = 0;

	for (int x = 0; x < resolutionXZ; x++) {
		for (int z = 0; z < resolutionXZ; z++) {
			float normalizedHeight = (hm[x][z] - globalMin) / (globalMax - globalMin);
			float convertHeight = normalizedHeight * heightAmplitude;
			int highestIndex = -1;

			for (int y = 0; y < resolutionY; y++) {
				int index = convert3DCoordinatesToFlatIndex(x, y, z);
				int worldY = y * blockResolution;

				if (worldY <= convertHeight) {
					if (worldY == 0) {
						chunkVec[index] = BlockID::BEDROCK;
					}
					if (worldY > 0) {
						chunkVec[index] = BlockID::STONE;
					}
					if (worldY >= convertHeight - 7) {
						chunkVec[index] = BlockID::DIRT;
					}
					highestIndex = index;
				}
				else {
					chunkVec[index] = BlockID::AIR;
				}
			}

			if (highestIndex != -1 && chunkVec[highestIndex] == BlockID::DIRT) {
				chunkVec[highestIndex] = BlockID::GRASS;
			}
			if (highestIndex > highestOccupiedIndex) {
				highestOccupiedIndex = highestIndex;
			}
		}
	}

	return highestOccupiedIndex;
}


std::vector<std::vector<float>> ProcGen::getHeightMap(std::pair<int, int> chunkCoordPair) {
	int resolution = ChunkUtils::WIDTH / blockResolution; // resolution is halved for each LOD
	std::vector<std::vector<float>> heightMap(resolution, std::vector<float>(resolution));

	int chunkX = chunkCoordPair.first;
	int chunkZ = chunkCoordPair.second;
	for (int x = 0; x < resolution; x++) {
		for (int z = 0; z < resolution; z++) {
			// Calculate world-space positions for this heightmap point.
			int x1 = (x * blockResolution) + (chunkX * ChunkUtils::WIDTH);
			int z1 = (z * blockResolution) + (chunkZ * ChunkUtils::DEPTH);

			float val;
			if (blockResolution == 1) {
				// Simple height calculation with full resolution
				val = static_cast<float>(heightMapWeights[0] * heightMapNoise[0].GetValue(x1, z1) +
					heightMapWeights[1] * heightMapNoise[1].GetValue(x1, z1) +
					heightMapWeights[2] * heightMapNoise[2].GetValue(x1, z1) +
					heightMapWeights[3] * heightMapNoise[3].GetValue(x1, z1));
			}
			else {
				// For higher LODs, sample multiple points and average them
				float sum = 0;
				int numSamples = blockResolution; // Number of samples to take in x and z
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

void ProcGen::setRandomNoiseState() {
	std::vector<float> state(21);

	for (int i = 0; i < 4; i++) {
		state[i] = (float)(rand() % 100) / 1000.0f;    // Random frequency between 0.0001 and 0.1
		state[i + 4] = rand() % 8 + 1;                   // Octaves between 1 and 8
		state[i + 8] = (float)(rand() % 200) / 100.0f;   // Lacunarity between 0.0 and 2.0
		state[i + 12] = (float)(rand() % 100) / 100.0f;  // Gain between 0.0 and 1.0
		state[i + 16] = (float)(rand() % 50) / 100.0f;   // Weight between 0.0 and 0.5
	}

	state[20] = (float)(rand() % 100) + 100;   // Height amplitude between 0 and 200

	setNoiseState(state);  // Apply the noise state

	for (int i = 0; i < 21; i++) {
		std::cout << i << ": " << state[i] << "\n";
	}
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

void ProcGen::setLODVariables(int levelOfDetail) {
	blockResolution = 1 << levelOfDetail;
	resolutionXZ = ChunkUtils::WIDTH / blockResolution;
	resolutionY = ChunkUtils::HEIGHT / blockResolution;
}

