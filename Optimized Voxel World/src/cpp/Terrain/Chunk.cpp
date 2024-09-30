#include "h/Terrain/Chunk.h"
#include "h/Terrain/WorldManager.h"
#include "GLFW/glfw3.h"
#include <iostream>

#define CHUNK_WIDTH 64
#define CHUNK_DEPTH 64
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
	detailLevel = 0;
}

void Chunk::setProcGenReference(ProcGen* pg) {
	proceduralAlgorithm = pg;
}

void Chunk::generateChunk(int detailLvl) {
	detailLevel = detailLvl;

	switch (detailLevel) {
	case 0:
		chunk.resize(1048576, 0);
		break;
	case 1:
		chunk.resize(131072, 0);
		break;
	case 2:
		chunk.resize(16384, 0);
		break;
	case 3:
		chunk.resize(2048, 0);
		break;
	case 4:
		chunk.resize(256, 0);
		break;
	case 5:
		chunk.resize(32, 0);
		break;
	case 6:
		chunk.resize(4, 0);
		break;
	default:
		std::cerr << "Invalid detail level: " << detailLevel << ". Defaulting to 6.\n";
		chunk.resize(4, 0);
		detailLevel = 6;
		break;
	}

	proceduralAlgorithm->generateChunk(chunk, std::make_pair(chunkX, chunkZ), detailLevel);

	hasBeenGenerated = true;
}

void Chunk::setWholeChunkMeshes() {
	for (int i = 0; i < chunk.size(); i++) {
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
	int localX = coords.x;
	int localZ = coords.z;
	coords.x += chunkX * CHUNK_WIDTH;
	coords.z += chunkZ * CHUNK_DEPTH;

	int positionMult = pow(2, detailLevel);
	int resolution = 64 / positionMult;

	bool masksPerformed[6] = { false, false, false, false, false, false };
	for (int i = 0; i < 6; i++) {

		// If on a max boundary of a chunk...
		if (localX == resolution - 1 && i == 1) {
			masksPerformed[1] = true;
			if (getBlockAt((chunkX + 1) * 64, coords.y, coords.z, true, false, i, detailLevel, false) == 0) {
				faceVisibilityBitmask |= POS_X; // Neighbor is air
			}
		}
		if (localZ == resolution - 1 && i == 5) {
			masksPerformed[5] = true;
			if (getBlockAt(coords.x, coords.y, (chunkZ + 1) * 64, true, false, i, detailLevel, false) == 0) {
				faceVisibilityBitmask |= POS_Z;
			}
		}

		// If on a min boundary of a chunk...
		if (localX == 0 && i == 0) {
			masksPerformed[0] = true;
			if (getBlockAt(((chunkX - 1) * 64) + (resolution - 1), coords.y, coords.z, true, false, i, detailLevel, false) == 0) {
				faceVisibilityBitmask |= NEG_X; // Neighbor is air
			}
		}
		if (localZ == 0 && i == 4) {
			masksPerformed[4] = true;
			if (getBlockAt(coords.x, coords.y, ((chunkZ - 1) * 64) + (resolution - 1), true, false, i, detailLevel, false) == 0) {
				faceVisibilityBitmask |= NEG_Z;
			}
		}

		if (masksPerformed[i] == false) {
			if (getBlockAt(coords.x + (i == 0 ? -1 : 0) + (i == 1 ? 1 : 0), 
						   coords.y + (i == 2 ? -1 : 0) + (i == 3 ? 1 : 0), 
				           coords.z + (i == 4 ? -1 : 0) + (i == 5 ? 1 : 0), 
						   false, false, i, detailLevel, false) == 0) { // If neighbor is air
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
	}
	return faceVisibilityBitmask;
}

void Chunk::setWorldReference(WorldManager* wm) {
	world = wm;
}

int Chunk::convertWorldCoordToChunkCoord(int worldCoord) {
	int chunkCoord = (worldCoord >= 0) ? (worldCoord / 64) : ((worldCoord - 63) / 64);
	return chunkCoord;
}

int Chunk::getBlockAt(int worldX, int worldY, int worldZ, bool boundaryCall, bool calledFromGlobal, int face, int prevLod, bool recursion) {
	int positionMult = 1 << detailLevel;
	int resolutionXZ = 64 / positionMult;
	int resolutionY = 256 / positionMult;
	int lodDifference = pow(2, abs(prevLod - detailLevel));

	int localX = (((worldX % 64) + 64) % 64);
	int localY = worldY;
	int localZ = (((worldZ % 64) + 64) % 64);

	if (!hasBeenGenerated) {
		return 0;
	}

	int cx = convertWorldCoordToChunkCoord(worldX);
	int cz = convertWorldCoordToChunkCoord(worldZ);

	if ((cx != chunkX || cz != chunkZ)) {
		return world->getBlockAtGlobal(worldX, worldY, worldZ, true, boundaryCall, detailLevel, face);
	}


	int tempX = localX;
	int tempZ = localZ;
	if (localX / 2 >= resolutionXZ) {
		localX /= pow(2, detailLevel - 1);
	}
	else if (localX >= resolutionXZ) {
		localX /= lodDifference;
	}
	if (localZ / 2 >= resolutionXZ) {
		localZ /= pow(2, detailLevel - 1);
	}
	else if (localZ >= resolutionXZ) {
		localZ /= lodDifference;
	}

	worldX = cx * 64 + localX;
	worldY = localY;
	worldZ = cz * 64 + localZ;


	if (worldY >= CHUNK_HEIGHT / positionMult) {
		return 0;
	}
	else if (worldY < 0) {
		return 1;
	}

	int flatIndex = convert3DCoordinatesToFlatIndex(localX, localY, localZ);

	int maxIndex = 1048638 / (positionMult * positionMult * positionMult);

	if (prevLod != detailLevel && face != -1 && prevLod > detailLevel && !recursion) {
		int blocks[4];
		blocks[0] = static_cast<int>(chunk[flatIndex]);
		if (face == 0 || face == 1) { // X
			blocks[1] = static_cast<int>(chunk[convert3DCoordinatesToFlatIndex(localX + 1, localY, localZ)]);
			blocks[2] = static_cast<int>(chunk[convert3DCoordinatesToFlatIndex(localX, localY + 1, localZ)]);
			blocks[3] = static_cast<int>(chunk[convert3DCoordinatesToFlatIndex(localX + 1, localY + 1, localZ)]);

		}
		else if (face == 4 || face == 5) { // Z
			blocks[1] = static_cast<int>(chunk[convert3DCoordinatesToFlatIndex(localX, localY + 1, localZ)]);
			blocks[2] = static_cast<int>(chunk[convert3DCoordinatesToFlatIndex(localX, localY, localZ + 1)]);;
			blocks[3] = static_cast<int>(chunk[convert3DCoordinatesToFlatIndex(localX, localY + 1, localZ + 1)]);
		}
		for (int i = 0; i < 4; i++) {
			if (blocks[i] == 0) {
				return 0;
			}
		}
		return blocks[0];
	}

	if (flatIndex >= 1048638 / (int)(pow(positionMult, 3))) {
		std::cout << "Flatindex was " << flatIndex << " with a max of " << 1048638 / (int)(pow(positionMult, 3)) << "\n";
		return 69;
	}
	return static_cast<int>(chunk[flatIndex]);
}

int Chunk::convert3DCoordinatesToFlatIndex(int x, int y, int z) {
	int positionMult = pow(2, detailLevel);
	int width = CHUNK_WIDTH / positionMult;
	int depth = CHUNK_DEPTH / positionMult;
	int height = CHUNK_HEIGHT / positionMult;

	return x + (z * width) + (y * width * depth);
}

glm::ivec3 Chunk::convertFlatIndexTo3DCoordinates(int flatIndex) {
	int width = CHUNK_WIDTH / pow(2, detailLevel);
	int depth = CHUNK_DEPTH / pow(2, detailLevel);
	int height = CHUNK_HEIGHT / pow(2, detailLevel);

	glm::ivec3 returnVec;
	returnVec.y = flatIndex / (width * depth);
	int remainder = flatIndex % (width * depth);
	returnVec.z = remainder / width;
	returnVec.x = remainder % width;

	return returnVec;
}


void Chunk::setChunkCoords(int cx, int cz) {
	chunkX = cx;
	chunkZ = cz;
}

void Chunk::greedyMesh() {
	ga.populatePlanes(visByFaceType, chunk, detailLevel);
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

void Chunk::placeBlock(int localX, int localY, int localZ, unsigned char blockToPlace) {
	int flatIndex = convert3DCoordinatesToFlatIndex(localX, localY, localZ);
	chunk[flatIndex] = blockToPlace;
}

int Chunk::getLod() {
	return detailLevel;
}

bool Chunk::generated() {
	return hasBeenGenerated;
}

Chunk::~Chunk() {
	std::vector<unsigned char> temp;
	chunk.swap(temp);
}

