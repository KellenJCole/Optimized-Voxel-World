#include "h/Terrain/Chunk.h"
#include "h/Terrain/WorldManager.h"
#include "GLFW/glfw3.h"
#include <iostream>

#define CHUNK_WIDTH 16
#define CHUNK_DEPTH 16
#define CHUNK_HEIGHT 256

Chunk::Chunk() {
	srand(time(NULL));
	iFaces[0] = NEG_X;
	iFaces[1] = POS_X;
	iFaces[2] = NEG_Y;
	iFaces[3] = POS_Y;
	iFaces[4] = NEG_Z;
	iFaces[5] = POS_Z;
	hasBeenGenerated = false;
}
/*
"dirt.png", "grass_top.png", "grass_side.png", "stone.png", "bedrock.png", "sand.png"
Dirt = 1
Grass = 2
Stone = 3
Bedrock = 4
Sand = 5
*/
void Chunk::generateChunk() {
	chunk.resize(65536, 0);
	// chunk is defined as an array of unsigned chars of length 65536 (16 * 256 * 16)

	FastNoise layer1, layer2, layer3, layer4;
	layer1.SetNoiseType(FastNoise::Perlin);
	layer1.SetSeed(12);
	layer1.SetFrequency(.01);
	layer1.SetFractalType(FastNoise::FractalType::RigidMulti);
	layer1.SetFractalOctaves(1);
	layer1.SetFractalLacunarity(2.000);
	layer1.SetFractalGain(0.59);

	layer2.SetNoiseType(FastNoise::Perlin);
	layer2.SetSeed(13);
	layer2.SetFrequency(.02);
	layer2.SetFractalType(FastNoise::FractalType::RigidMulti);
	layer2.SetFractalOctaves(1);
	layer2.SetFractalLacunarity(2.000);
	layer2.SetFractalGain(0.59);

	unsigned int biome = getBiome();


	if (biome == 0) {
		for (int i = 0; i < CHUNK_HEIGHT; i++) {
			for (int j = 0; j < CHUNK_HEIGHT; j++) {
				int flatIndex = i * CHUNK_HEIGHT + j;
				glm::ivec3 pos = convertFlatIndexTo3DCoordinates(flatIndex);

				float worldX = chunkX * 16.0f + pos.x; // Assuming each chunk has a width of 16 units
				float worldZ = chunkZ * 16.0f + pos.z;

				float noiseValue = layer1.GetNoise(worldX, worldZ) + (0.5 * layer2.GetNoise(worldX, worldZ));

				int groundHeight = static_cast<int>((noiseValue + 1) * 64) + 70; // Increased range for height variation

				if (pos.y == groundHeight) {
					chunk[flatIndex] = 2;
				}
				else if (pos.y == 0) {
					chunk[flatIndex] = 4;
				}
				else if (pos.y < groundHeight && groundHeight - pos.y <= 5) {
					chunk[flatIndex] = 1;
				}
				else if (pos.y < groundHeight && groundHeight - pos.y > 5) {
					chunk[flatIndex] = 3;
				}
				else {
					chunk[flatIndex] = 0;
				}
			}
		}
	}
	else {
		for (int i = 0; i < CHUNK_HEIGHT; i++) {
			for (int j = 0; j < CHUNK_HEIGHT; j++) {
				int flatIndex = i * CHUNK_HEIGHT + j;
				glm::ivec3 pos = convertFlatIndexTo3DCoordinates(flatIndex);

				float worldX = chunkX * 16.0f + pos.x; // Assuming each chunk has a width of 16 units
				float worldZ = chunkZ * 16.0f + pos.z;

				float noiseValue = 0.5 * layer1.GetNoise(worldX, worldZ) + layer2.GetNoise(worldX, worldZ);

				int groundHeight = static_cast<int>((noiseValue + 1) * 20) + 100; // Increased range for height variation

				if (pos.y == 0) {
					chunk[flatIndex] = 4;
				}
				else if (pos.y <= groundHeight) {
					chunk[flatIndex] = 5;
				}
				else {
					chunk[flatIndex] = 0;
				}
			}
		}
	}
	hasBeenGenerated = true;

}

unsigned int Chunk::getBiome() {
	FastNoise biomeNoise;
	biomeNoise.SetNoiseType(FastNoise::NoiseType::Cellular);
	biomeNoise.SetSeed(1337);
	biomeNoise.SetFrequency(0.011);
	biomeNoise.SetCellularDistanceFunction(FastNoise::CellularDistanceFunction::Euclidean);
	biomeNoise.SetCellularReturnType(FastNoise::CellularReturnType::Distance2Sub);
	biomeNoise.SetCellularJitter(1.120);

	float noiseValue = biomeNoise.GetNoise(chunkX, chunkZ);
	if (noiseValue < 0.5) {
		return 0;
	}
	else {
		return 1;
	}
}

void Chunk::setWholeChunkMeshes() {
	for (int i = 0; i < CHUNK_WIDTH * CHUNK_DEPTH * CHUNK_HEIGHT; i++) {
		if (chunk[i] != 0) { // If the block is not air, continue
			unsigned char neighbor_bitmask = checkNeighbors(i);
			if (neighbor_bitmask != 0b000000) { // If at least one face of block is visible
				if (neighbor_bitmask & NEG_X) visByFaceType[NEG_X].push_back(i);
				if (neighbor_bitmask & POS_X) visByFaceType[POS_X].push_back(i);
				if (neighbor_bitmask & NEG_Y) visByFaceType[NEG_Y].push_back(i);
				if (neighbor_bitmask & POS_Y) visByFaceType[POS_Y].push_back(i);
				if (neighbor_bitmask & NEG_Z) visByFaceType[NEG_Z].push_back(i);
				if (neighbor_bitmask & POS_Z) visByFaceType[POS_Z].push_back(i);
			}
		}
	}
	greedyMesh();
}

unsigned char Chunk::checkNeighbors(int blockIndex) {
	unsigned char faceVisibilityBitmask = 0;

	glm::vec3 coords = convertFlatIndexTo3DCoordinates(blockIndex);
	coords.x += chunkX * CHUNK_WIDTH;
	coords.z += chunkZ * CHUNK_DEPTH;

	for (int i = 0; i < 6; i++) {
		if (getBlockAt(coords.x + (i == 0 ? -1 : 0) + (i == 1 ? 1 : 0), coords.y + (i == 2 ? -1 : 0) + (i == 3 ? 1 : 0), coords.z + (i == 4 ? -1 : 0) + (i == 5 ? 1 : 0)) == 0) { // If neighbor is air
			switch (i) {
				case 0: faceVisibilityBitmask |= NEG_X; break;
				case 1: faceVisibilityBitmask |= POS_X; break;
				case 2: faceVisibilityBitmask |= NEG_Y; break;
				case 3: faceVisibilityBitmask |= POS_Y; break;
				case 4: faceVisibilityBitmask |= NEG_Z; break;
				case 5: faceVisibilityBitmask |= POS_Z; break;
			}
		}
	}
	return faceVisibilityBitmask;
}

void Chunk::setWorldReference(WorldManager* wm) {
	world = wm;
}

int Chunk::getBlockAt(int worldX, int worldY, int worldZ) {
	if (worldY >= CHUNK_HEIGHT) {
		return 0;
	}
	else if (worldY < 0) {
		return 1;
	}

	int cx = worldX >> 4;
	int cz = worldZ >> 4;

	// Ensure local coordinates are within [0, 15] range
	if (cx < chunkX || cx > chunkX || cz < chunkZ || cz > chunkZ) {
		return world->getBlockAtGlobal(worldX, worldY, worldZ, true);
	}

	int localX = (worldX % CHUNK_WIDTH + CHUNK_WIDTH) % CHUNK_WIDTH;
	int localZ = (worldZ % CHUNK_DEPTH + CHUNK_DEPTH) % CHUNK_DEPTH;
	int flatIndex = convert3DCoordinatesToFlatIndex(localX, worldY, localZ);
	return static_cast<int>(chunk[flatIndex]);
}

int Chunk::convert3DCoordinatesToFlatIndex( int x,  int y,  int z) {
	return (y << 8) | (z << 4) | x;
}

glm::ivec3 Chunk::convertFlatIndexTo3DCoordinates(int flatIndex) {
	glm::ivec3 returnVec;
	returnVec.x = flatIndex & 0x0F; // Same as flatIndex % 16
	returnVec.y = flatIndex >> 8; // Same as flatIndex / 256
	returnVec.z = (flatIndex & 0xFF) >> 4; // Same as (flatIndex % 256) / 16
	return returnVec;
}

void Chunk::setChunkCoords(int cx, int cz) {
	chunkX = cx;
	chunkZ = cz;
}

void Chunk::greedyMesh() {
	ga.populatePlanes(visByFaceType, chunk);
	for (int i = 0; i < 6; i++) ga.firstPassOn(iFaces[i]);
}

void Chunk::unload() {
	visByFaceType.clear();
	ga.unload();
}

std::vector<std::pair<std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>>, int>> Chunk::getGreedyMeshByFaceType(int faceType) {
	return ga.getAllGreedyGraphs(faceType);
}

void Chunk::breakBlock(int localX, int localY, int localZ) {
	int flatIndex = convert3DCoordinatesToFlatIndex(localX, localY, localZ);
	chunk[flatIndex] = 0;
}

Chunk::~Chunk() {
	chunk.clear();
}

