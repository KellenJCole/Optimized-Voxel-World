#include "h/Terrain/Chunk.h"
#include "h/Terrain/WorldManager.h"
#include <h/Terrain/GreedyGraph.h>
#include "GLFW/glfw3.h"
#include <iostream>

#define CHUNK_WIDTH 16
#define CHUNK_DEPTH 16
#define CHUNK_HEIGHT 256

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

	for (int i = 0; i < CHUNK_HEIGHT; i++) {
		for (int j = 0; j < CHUNK_HEIGHT; j++) {
			int flatIndex = i * CHUNK_HEIGHT + j;
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
	for (auto& flatIndex : visByFaceType[face]) {
		glm::vec3 localCoordinates = convertFlatIndexTo3DCoordinates(flatIndex);
		localCoordinates.x += chunkX * CHUNK_WIDTH;
		localCoordinates.z += chunkZ * CHUNK_DEPTH;
		returnVec.push_back(localCoordinates);
	}
	return returnVec;
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
		return world->getBlockAtGlobal(worldX, worldY, worldZ);
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
};

void Chunk::setChunkCoords(int cx, int cz) {
	chunkX = cx;
	chunkZ = cz;
}

void Chunk::unload() {
	world = nullptr;
	chunk.clear();

	for (auto& mesh : visByFaceType) {
		mesh.second.clear();
	}
	for (int i = 0; i < 6; i++) {
		Greedy_Graphs[i].clear();
	}
	planes.clear();
	visByFaceType.clear();
}

void Chunk::greedyMesh() {
	populatePlanes();

	BlockFace iFaces[6] = { NEG_X, POS_X, NEG_Y, POS_Y, NEG_Z, POS_Z };

	for (int i = 0; i < 6; i++) firstPassOn(iFaces[i]);

	planes.clear();
}

std::vector<std::pair<GreedyGraph, int>> Chunk::getGreedyMeshByFaceType(int faceType) {
	return Greedy_Graphs[faceType];
}

void Chunk::firstPassOn(BlockFace f) {
	int iMax, greedyGraphIndex;
	switch (f) {
	case NEG_X: iMax = 16; greedyGraphIndex = 0; break;
	case POS_X: iMax = 16; greedyGraphIndex = 1; break;
	case NEG_Y: iMax = 256; greedyGraphIndex = 2; break;
	case POS_Y: iMax = 256; greedyGraphIndex = 3; break;
	case NEG_Z: iMax = 16; greedyGraphIndex = 4; break;
	case POS_Z: iMax = 16; greedyGraphIndex = 5; break;
	}

	for (int i = 0; i < iMax; i++) {
		GreedyGraph gg;
		if (planes.find(f) != planes.end()) {
			std::vector<std::vector<unsigned char>> plane = planes[f][i];
			unsigned char focusBlock = 69;
			int startY, endY;
			std::pair<unsigned char, std::pair<std::pair<int, int>, int>> newVertex, oldVertex;
			std::pair<unsigned char, std::pair<std::pair<int, int>, int>> initial = { 0, {{0, 0}, 0 } };
			for (int firstAxis = 0; firstAxis < plane.size(); firstAxis++) {
				oldVertex = initial;
				for (int secondAxis = 0; secondAxis < plane[0].size(); secondAxis++) {
					if (focusBlock == 69) {
						focusBlock = plane[firstAxis][secondAxis];
						startY = firstAxis;
					}
					int blockHere = plane[firstAxis][secondAxis];
					if (blockHere == focusBlock) {
						endY = firstAxis;
					}
					else {
						newVertex = { focusBlock, {{startY, endY}, secondAxis }};
						gg.addVertex(newVertex);
						if (oldVertex != initial) {
							gg.addEdge(oldVertex, newVertex);
						}
						oldVertex = newVertex;
						startY = firstAxis;
						focusBlock = blockHere;
					}
				}
			}
			Greedy_Graphs[greedyGraphIndex].push_back({ gg, i});
		}
	}
};

void Chunk::populatePlanes() {
	planes[NEG_X].resize(CHUNK_WIDTH);
	planes[POS_X].resize(CHUNK_WIDTH);
	planes[NEG_Y].resize(CHUNK_HEIGHT);
	planes[POS_Y].resize(CHUNK_HEIGHT);
	planes[NEG_Z].resize(CHUNK_DEPTH);
	planes[POS_Z].resize(CHUNK_DEPTH);

	for (int j = 0; j < CHUNK_WIDTH; j++) {
		planes[NEG_X][j].resize(CHUNK_DEPTH);
		planes[POS_X][j].resize(CHUNK_DEPTH);
		planes[NEG_Z][j].resize(CHUNK_WIDTH);
		planes[POS_Z][j].resize(CHUNK_WIDTH);
		for (int k = 0; k < CHUNK_DEPTH; k++) {
			planes[NEG_X][j][k].resize(CHUNK_HEIGHT, 0);
			planes[POS_X][j][k].resize(CHUNK_HEIGHT, 0);
			planes[NEG_Z][j][k].resize(CHUNK_HEIGHT, 0);
			planes[POS_Z][j][k].resize(CHUNK_HEIGHT, 0);
		}
	}

	for (int j = 0; j < CHUNK_HEIGHT; j++) {
			planes[NEG_Y][j].resize(CHUNK_WIDTH);
			planes[POS_Y][j].resize(CHUNK_WIDTH);
			for (int k = 0; k < CHUNK_DEPTH; k++) {
				planes[NEG_Y][j][k].resize(CHUNK_DEPTH);
				planes[POS_Y][j][k].resize(CHUNK_DEPTH);
			}
	}

	for (const auto& blockLocation : visByFaceType[NEG_X]) {
		planes[NEG_X][blockLocation & 0x0F][(blockLocation & 0xFF) >> 4][blockLocation >> 8] = chunk[blockLocation];
	}
	for (const auto& blockLocation : visByFaceType[POS_X]) {
		planes[POS_X][blockLocation & 0x0F][(blockLocation & 0xFF) >> 4][blockLocation >> 8] = chunk[blockLocation];
	}
	for (const auto& blockLocation : visByFaceType[NEG_Y]) {
		planes[NEG_Y][blockLocation >> 8][blockLocation & 0x0F][(blockLocation & 0xFF) >> 4] = chunk[blockLocation];
	}
	for (const auto& blockLocation : visByFaceType[POS_Y]) {
		planes[POS_Y][blockLocation >> 8][blockLocation & 0x0F][(blockLocation & 0xFF) >> 4] = chunk[blockLocation];
	}
	for (const auto& blockLocation : visByFaceType[NEG_Z]) {
		planes[NEG_Z][(blockLocation & 0xFF) >> 4][blockLocation & 0x0F][blockLocation >> 8] = chunk[blockLocation];
	}
	for (const auto& blockLocation : visByFaceType[POS_Z]) {
		planes[POS_Z][(blockLocation & 0xFF) >> 4][blockLocation & 0x0F][blockLocation >> 8] = chunk[blockLocation];
	}
}
