#include "h/Terrain/Chunk.h"
#include "h/Terrain/WorldManager.h"
#include "GLFW/glfw3.h"
#include <iostream>

Chunk::Chunk() {

}

void Chunk::generateChunk() {
	chunk.resize(65536);
	// chunk is defined as an array of unsigned chars of length 65536 (16 * 256 * 16)
	noise.SetNoiseType(FastNoise::Perlin);
	noise.SetSeed(12);
	noise.SetFrequency(.01);
	noise.SetFractalType(FastNoise::FractalType::RigidMulti);
	noise.SetFractalOctaves(1);
	noise.SetFractalLacunarity(2.000);
	noise.SetFractalGain(0.59);

	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			int flatIndex = i * 256 + j;
			glm::ivec3 pos = convertFlatIndexTo3DCoordinates(flatIndex);

			float worldX = chunkX * 16.0f + pos.x; // Assuming each chunk has a width of 16 units
			float worldZ = chunkZ * 16.0f + pos.z;

			float noiseValue = noise.GetNoise(worldX, worldZ);

			int groundHeight = static_cast<int>((noiseValue + 1) * 64); // Increased range for height variation

			if (pos.y <= groundHeight) {
				chunk[flatIndex] = static_cast<unsigned char>(1);
			}
			else {
				chunk[flatIndex] = static_cast<unsigned char>(0);
			}
		}
	}
}


void Chunk::setWholeChunkMeshes() {
	for (int i = 0; i < 65536; i++) {
		if (chunk[i] != 0) { // If the block is not air, continue
			unsigned char neighbor_bitmask = checkNeighbors(i);
			if (neighbor_bitmask != 0b000000) { // If at least one face of block is visible
				if (neighbor_bitmask & NEG_X) meshByFaceType[NEG_X].push_back(i);
				if (neighbor_bitmask & POS_X) meshByFaceType[POS_X].push_back(i);
				if (neighbor_bitmask & NEG_Y) meshByFaceType[NEG_Y].push_back(i);
				if (neighbor_bitmask & POS_Y) meshByFaceType[POS_Y].push_back(i);
				if (neighbor_bitmask & NEG_Z) meshByFaceType[NEG_Z].push_back(i);
				if (neighbor_bitmask & POS_Z) meshByFaceType[POS_Z].push_back(i);
			}
		}
	}
}

std::vector<glm::vec3> Chunk::getMeshByFaceType(int faceType) {
	std::vector<glm::vec3> returnVec;
	BlockFace face;
	switch (faceType) {
	case 0: face = NEG_X; break;
	case 1: face = POS_X; break;
	case 2: face = NEG_Y; break;
	case 3: face = POS_Y; break;
	case 4: face = NEG_Z; break;
	case 5: face = POS_Z; break;
	}
	for (auto& flatIndex : meshByFaceType[face]) {
		glm::vec3 localCoordinates = convertFlatIndexTo3DCoordinates(flatIndex);
		localCoordinates.x += chunkX * 16;
		localCoordinates.z += chunkZ * 16;
		returnVec.push_back(localCoordinates);
	}
	return returnVec;
}



unsigned char Chunk::checkNeighbors(int blockIndex) {
	unsigned char faceVisibilityBitmask = 0;

	glm::vec3 coords = convertFlatIndexTo3DCoordinates(blockIndex);
	coords.x += chunkX * 16;
	coords.z += chunkZ * 16;

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
	if (worldY > 255) {
		return 0;
	}
	else if (worldY < 0) {
		return 1;
	}

	int cx = worldX >> 4;
	int cz = worldZ >> 4;

	// Ensure local coordinates are within [0, 15] range
	if (cx < chunkX || cx > chunkX || cz < chunkZ || cz > chunkZ) {
		return world->getBlockAtGlobal(worldX, worldY, worldZ);
	}

	int localX = (worldX % 16 + 16) % 16;
	int localZ = (worldZ % 16 + 16) % 16;
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
};

void Chunk::setChunkCoords(int cx, int cz) {
	chunkX = cx;
	chunkZ = cz;
}

void Chunk::unload() {
	world = nullptr;
	chunk.clear();

	for (auto& mesh : meshByFaceType) {
		mesh.second.clear();
	}
	meshByFaceType.clear();
}
