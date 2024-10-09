#include "h/Terrain/Chunk.h"
#include "h/Terrain/WorldManager.h"
#include "GLFW/glfw3.h"
#include <iostream>

Chunk::Chunk() :
	chunkX(0),
	chunkZ(0),
	detailLevel(-1),
	blockResolution(0),
	resolutionXZ(64),
	resolutionY(256),
	highestOccupiedIndex(0)
{
	iFaces[0] = NEG_X;
	iFaces[1] = POS_X;
	iFaces[2] = NEG_Y;
	iFaces[3] = POS_Y;
	iFaces[4] = NEG_Z;
	iFaces[5] = POS_Z;

	for (int index = 0; index < 7; index++)
		hasBeenGenerated[index] = false;
}

void Chunk::generateChunk() {
	setChunkLodMapVectorSize();
	highestOccupiedIndex = proceduralAlgorithm->generateChunk(chunkLodMap[detailLevel], std::make_pair(chunkX, chunkZ), detailLevel);
	hasBeenGenerated[detailLevel] = true;
}

void Chunk::setChunkLodMapVectorSize() {
	switch (detailLevel) {
	case 0:
		chunkLodMap[0].resize(1048576, 0);
		break;
	case 1:
		chunkLodMap[1].resize(131072, 0);
		break;
	case 2:
		chunkLodMap[2].resize(16384, 0);
		break;
	case 3:
		chunkLodMap[3].resize(2048, 0);
		break;
	case 4:
		chunkLodMap[4].resize(256, 0);
		break;
	case 5:
		chunkLodMap[5].resize(32, 0);
		break;
	case 6:
		chunkLodMap[6].resize(4, 0);
		break;
	default:
		std::cerr << "Invalid detail level: " << detailLevel << ". Defaulting to 6.\n";
		chunkLodMap[6].resize(4, 0);
		detailLevel = 6;
		break;
	}
}

#define CHECK_PERFORMED_MASK(face) (1 << (face * 2))
#define CHECK_RESULT_MASK(face)    (1 << (face * 2 + 1))

void Chunk::generateChunkMeshes() { // Iterates through every face on every block, filling relevant vectors with face visibility status.
	std::vector<uint16_t> neighborCheckCache;
	neighborCheckCache.resize(resolutionXZ * resolutionXZ * resolutionY, 0);
	int resolution = ChunkUtils::WIDTH / blockResolution;
	for (int blockIndex = 0; blockIndex <= highestOccupiedIndex; blockIndex++) {
		if (chunkLodMap[detailLevel][blockIndex] != 0) { // If the block is not air, continue
			unsigned char neighbor_bitmask = checkNeighbors(blockIndex, neighborCheckCache);
			if (neighbor_bitmask != 0b000000) { // If at least one face of block is visible
				if (neighbor_bitmask & NEG_X) visByFaceType[NEG_X].push_back(blockIndex);
				if (neighbor_bitmask & POS_X) visByFaceType[POS_X].push_back(blockIndex);
				if (neighbor_bitmask & NEG_Y) visByFaceType[NEG_Y].push_back(blockIndex);
				if (neighbor_bitmask & POS_Y) visByFaceType[POS_Y].push_back(blockIndex);
				if (neighbor_bitmask & NEG_Z) visByFaceType[NEG_Z].push_back(blockIndex);
				if (neighbor_bitmask & POS_Z) visByFaceType[POS_Z].push_back(blockIndex);
			}
		}
	}

	greedyMesh();
}

unsigned char Chunk::checkNeighbors(int blockIndex, std::vector<uint16_t>& neighborCache) {
	unsigned char faceVisibilityBitmask = 0;

	glm::vec3 coords = convertFlatIndexTo3DCoordinates(blockIndex);
	int localX = coords.x;
	int localZ = coords.z;

	coords.x += chunkX * ChunkUtils::WIDTH;
	coords.z += chunkZ * ChunkUtils::DEPTH;

	for (int i = 0; i < 6; i++) {
		int neighborIndex = blockIndex + neighborOffsets[i];

        if (hasNeighborCheckBeenPerformed(blockIndex, i, neighborCache)) {
            if (neighborCache[blockIndex] & CHECK_RESULT_MASK(i)) {
                faceVisibilityBitmask |= (1 << i);  // Neighbor is air, set visibility bit
            }
        }
		else {
			bool isNeighborAir = false;
			bool checkedNeighbor = false;
			if (localX == resolutionXZ - 1 && i == 1) {
				isNeighborAir = (world->getBlockAtGlobal((chunkX + 1) * ChunkUtils::WIDTH, coords.y, coords.z, detailLevel, i) == 0);
				checkedNeighbor = true;
			}
			if (localZ == resolutionXZ - 1 && i == 5) {
				isNeighborAir = (world->getBlockAtGlobal(coords.x, coords.y, (chunkZ + 1) * ChunkUtils::DEPTH, detailLevel, i) == 0);
				checkedNeighbor = true;
			}
			if (localX == 0 && i == 0) {
				isNeighborAir = (world->getBlockAtGlobal(((chunkX - 1) * ChunkUtils::WIDTH) + (resolutionXZ - 1), coords.y, coords.z, detailLevel, i) == 0);
				checkedNeighbor = true;
			}
			if (localZ == 0 && i == 4) {
				isNeighborAir = (world->getBlockAtGlobal(coords.x, coords.y, ((chunkZ - 1) * ChunkUtils::WIDTH) + (resolutionXZ - 1), detailLevel, i) == 0);
				checkedNeighbor = true;
			}
			if (!checkedNeighbor && neighborIndex >= 0 && neighborIndex < resolutionXZ * resolutionXZ * resolutionY) {
				isNeighborAir = (chunkLodMap[detailLevel][neighborIndex] == 0);
			}

			if (!isNeighborAir && !checkedNeighbor) {
				markNeighborsCheck(neighborIndex, i, neighborCache);
			}
			else {
				faceVisibilityBitmask |= (1 << i);  // Neighbor is air, set visibility bit
			}
		}
	}
	return faceVisibilityBitmask;
}

void Chunk::markNeighborsCheck(int neighborIndex, int face, std::vector<uint16_t>& neighborCache) {
	int oppositeFace = (face % 2 == 0) ? face + 1 : face - 1;  // Find the opposite face

	if (neighborIndex >= 0 && neighborIndex < resolutionXZ * resolutionXZ * resolutionY) {
		neighborCache[neighborIndex] |= CHECK_PERFORMED_MASK(oppositeFace);
		// Since the neighbor is adjacent to a solid block, and it's solid itself, it doesn't have a visible face here
		neighborCache[neighborIndex] &= ~CHECK_RESULT_MASK(oppositeFace); // Set visibility to false
	}
}

bool Chunk::hasNeighborCheckBeenPerformed(int blockIndex, int face, std::vector<uint16_t>& neighborCache) {
	return neighborCache[blockIndex] & CHECK_PERFORMED_MASK(face);
}

unsigned char Chunk::getBlockAt(int worldX, int worldY, int worldZ, int face, int prevLod) {
	if (worldY >= resolutionY) {
		return 0;
	}
	else if (worldY < 0) {
		return 1;
	}

	int localX = ChunkUtils::convertWorldCoordToLocalCoord(worldX);
	int localZ = ChunkUtils::convertWorldCoordToLocalCoord(worldZ);

	if (!hasBeenGenerated[detailLevel]) {
		return 0;
	}

	int flatIndex = convert3DCoordinatesToFlatIndex(localX, worldY, localZ);

	if (prevLod != detailLevel && face != -1 && prevLod > detailLevel) {
		unsigned char blocks[4];
		blocks[0] = chunkLodMap[detailLevel][flatIndex];
		if (face == 0 || face == 1) { // X
			blocks[1] = chunkLodMap[detailLevel][convert3DCoordinatesToFlatIndex(localX + 1,	worldY,			localZ)];
			blocks[2] = chunkLodMap[detailLevel][convert3DCoordinatesToFlatIndex(localX,		worldY + 1,		localZ)];
			blocks[3] = chunkLodMap[detailLevel][convert3DCoordinatesToFlatIndex(localX + 1,	worldY + 1,		localZ)];

		}
		else if (face == 4 || face == 5) { // Z
			blocks[1] = chunkLodMap[detailLevel][convert3DCoordinatesToFlatIndex(localX,		worldY + 1,		localZ)];
			blocks[2] = chunkLodMap[detailLevel][convert3DCoordinatesToFlatIndex(localX,		worldY,			localZ + 1)];
			blocks[3] = chunkLodMap[detailLevel][convert3DCoordinatesToFlatIndex(localX,		worldY + 1,		localZ + 1)];
		}
		for (int i = 0; i < 4; i++) {
			if (blocks[i] == 0) {
				return 0;
			}
		}
		return 69;
	}

	return chunkLodMap[detailLevel][flatIndex];
}

void Chunk::convertLOD(int newLod) {
	std::vector<unsigned char> temp;
	chunkLodMap[detailLevel].swap(temp);
	setLod(newLod);
	generateChunk();
}

void Chunk::setProcGenReference(ProcGen* pg) {
	proceduralAlgorithm = pg;
}

void Chunk::setWorldReference(WorldManager* wm) {
	world = wm;
}

void Chunk::setChunkCoords(int cx, int cz) {
	chunkX = cx;
	chunkZ = cz;
}

void Chunk::setLod(int detailLvl) {
	detailLevel = detailLvl;
	blockResolution = 1 << detailLevel;
	resolutionXZ = ChunkUtils::WIDTH / blockResolution;
	resolutionY = ChunkUtils::HEIGHT / blockResolution;
	neighborOffsets[0] = -1;
	neighborOffsets[1] = 1;
	neighborOffsets[2] = -resolutionXZ * resolutionXZ;
	neighborOffsets[3] = resolutionXZ * resolutionXZ;
	neighborOffsets[4] = -resolutionXZ;
	neighborOffsets[5] = resolutionXZ;
	setChunkLodMapVectorSize();
}

int Chunk::getCurrentLod() {
	return detailLevel;
}

void Chunk::greedyMesh() {
	ga.populatePlanes(visByFaceType, chunkLodMap[detailLevel], detailLevel);
	for (int i = 0; i < 6; i++) ga.firstPassOn(iFaces[i]);
}

void Chunk::breakBlock(int localX, int localY, int localZ) {
	int flatIndex = convert3DCoordinatesToFlatIndex(localX, localY, localZ);

	chunkLodMap[detailLevel][flatIndex] = 0;
}

void Chunk::placeBlock(int localX, int localY, int localZ, unsigned char blockToPlace) {
	int flatIndex = convert3DCoordinatesToFlatIndex(localX, localY, localZ);
	if (flatIndex > highestOccupiedIndex) {
		highestOccupiedIndex = flatIndex;
	}
	chunkLodMap[detailLevel][flatIndex] = blockToPlace;
}

glm::ivec3 Chunk::convertFlatIndexTo3DCoordinates(int flatIndex) {
	glm::ivec3 returnVec;
	returnVec.y = flatIndex / (resolutionXZ * resolutionXZ);
	int remainder = flatIndex % (resolutionXZ * resolutionXZ);
	returnVec.z = remainder / resolutionXZ;
	returnVec.x = remainder % resolutionXZ;

	return returnVec;
}

std::vector<std::pair<std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>>, int>> Chunk::getGreedyMeshByFaceType(int faceType) {
	return ga.getAllGreedyGraphs(faceType);
}

void Chunk::unload() {
	visByFaceType.clear();
	ga.unload();
}

std::vector<unsigned char> Chunk::getCurrChunkVec() {
	if (hasBeenGenerated[detailLevel])
		return chunkLodMap[detailLevel];
	else
		return { ' '};
}

