#include "h/Terrain/Chunk.h"

#include <chrono>
#include <iostream>

#include "GLFW/glfw3.h"
#include "h/Terrain/WorldManager.h"

Chunk::Chunk()
    : chunkX(0),
      chunkZ(0),
      detailLevel(-1),
      blockResolution(0),
      resolutionXZ(64),
      resolutionY(256),
      highestOccupiedIndex(0) {

    for (int index = 0; index < 7; index++) hasBeenGenerated[index] = false;
}

void Chunk::generateChunk() {
    setChunkLodMapVectorSize();
    highestOccupiedIndex =
        proceduralAlgorithm->generateChunk(chunkLodMap[detailLevel], std::make_pair(chunkX, chunkZ), detailLevel);
    hasBeenGenerated[detailLevel] = true;
}

void Chunk::setChunkLodMapVectorSize() {

    constexpr size_t MAX_SIZE = 1048576;  // Maximum size for the chunk lod map at detail level 0
    size_t newSize = MAX_SIZE >> (detailLevel * 3);  // Adjust size based on detail level

    chunkLodMap[detailLevel].resize(newSize, BlockID::AIR);
}

#define CHECK_PERFORMED_MASK(face) (1 << (face * 2))
#define CHECK_RESULT_MASK(face) (1 << (face * 2 + 1))

void Chunk::startMeshing() {
    std::vector<uint16_t> neighborCheckCache;
    neighborCheckCache.resize(resolutionXZ * resolutionXZ * resolutionY, 0);
    int resolution = ChunkUtils::WIDTH / blockResolution;
    for (int blockIndex = 0; blockIndex <= highestOccupiedIndex; blockIndex++) {
        if (chunkLodMap[detailLevel][blockIndex] != BlockID::AIR) {
            BlockFaceBitmask mask = cullFaces(blockIndex, neighborCheckCache);
            if (mask != BlockFaceBitmask::NONE) {  // If at least one face of block is visible
                if (has(mask, BlockFaceBitmask::NEG_X)) visByFaceType[BlockFace::NEG_X].push_back(blockIndex);
                if (has(mask, BlockFaceBitmask::POS_X)) visByFaceType[BlockFace::POS_X].push_back(blockIndex);
                if (has(mask, BlockFaceBitmask::NEG_Y)) visByFaceType[BlockFace::NEG_Y].push_back(blockIndex);
                if (has(mask, BlockFaceBitmask::POS_Y)) visByFaceType[BlockFace::POS_Y].push_back(blockIndex);
                if (has(mask, BlockFaceBitmask::NEG_Z)) visByFaceType[BlockFace::NEG_Z].push_back(blockIndex);
                if (has(mask, BlockFaceBitmask::POS_Z)) visByFaceType[BlockFace::POS_Z].push_back(blockIndex);
            }
        }
    }
}

BlockFaceBitmask Chunk::cullFaces(int blockIndex, std::vector<uint16_t>& neighborCache) {
    BlockFaceBitmask mask = BlockFaceBitmask::NONE;

    glm::ivec3 coords = expandChunkCoords(blockIndex);
    int localX = coords.x;
    int localY = coords.y;
    int localZ = coords.z;

    bool isBorderXNeg = (localX == 0);
    bool isBorderXPos = (localX == resolutionXZ - 1);
    bool isBorderZNeg = (localZ == 0);
    bool isBorderZPos = (localZ == resolutionXZ - 1);

    glm::ivec3 worldCoords = {localX + chunkX * ChunkUtils::WIDTH, localY, localZ + chunkZ * ChunkUtils::DEPTH};

    for (int i = 0; i < 6; ++i) {
        if (hasNeighborCheckBeenPerformed(blockIndex, i, neighborCache)) {
            if (neighborCache[blockIndex] & CHECK_RESULT_MASK(i)) {
                mask |= static_cast<BlockFaceBitmask>(1u << i);
            }
        } else {
            bool neighborIsAir = false;
            bool borderCheck = false;

            int neighborIndex = blockIndex + neighborOffsets[i];

            if ((i == 0 && isBorderXNeg) || (i == 1 && isBorderXPos) || (i == 4 && isBorderZNeg) ||
                (i == 5 && isBorderZPos)) {
                int neighborX = worldCoords.x + (i == 1) - (i == 0);
                int neighborY = worldCoords.y + (i == 3) - (i == 2);
                if (neighborY < 0 || neighborY >= resolutionY) {
                    continue;
                }
                int neighborZ = worldCoords.z + (i == 5) - (i == 4);
                neighborIsAir = (world->getBlockAtGlobal(neighborX, neighborY, neighborZ, detailLevel, i) == BlockID::AIR);
                borderCheck = true;
            } else if (neighborIndex >= 0 && neighborIndex < chunkLodMap[detailLevel].size()) {
                neighborIsAir = (chunkLodMap[detailLevel][neighborIndex] == BlockID::AIR);
            }

            if (neighborIsAir) {
                mask |= static_cast<BlockFaceBitmask>(1u << i);
            }
            if (!borderCheck) {
                markNeighborsCheck(neighborIndex, i, neighborCache);
            }
        }
    }

    return mask;
}

void Chunk::markNeighborsCheck(int neighborIndex, int face, std::vector<uint16_t>& neighborCache) {
    int oppositeFace = (face % 2 == 0) ? face + 1 : face - 1;

    if (neighborIndex >= 0 && neighborIndex < resolutionXZ * resolutionXZ * resolutionY) {
        neighborCache[neighborIndex] |= CHECK_PERFORMED_MASK(oppositeFace);
        neighborCache[neighborIndex] &= ~CHECK_RESULT_MASK(oppositeFace);
    }
}

bool Chunk::hasNeighborCheckBeenPerformed(int blockIndex, int face, std::vector<uint16_t>& neighborCache) {
    return neighborCache[blockIndex] & CHECK_PERFORMED_MASK(face);
}

BlockID Chunk::getBlockAt(int worldX, int worldY, int worldZ, int face, int prevLod) {
    if (worldY >= resolutionY) {
        return BlockID::AIR;
    } else if (worldY < 0) {
        return BlockID::BEDROCK;
    }

    int localX = ChunkUtils::convertWorldCoordToLocalCoord(worldX);
    int localZ = ChunkUtils::convertWorldCoordToLocalCoord(worldZ);

    int flatIndex = ChunkUtils::flattenChunkCoords(localX, worldY, localZ, detailLevel);

    if (prevLod > detailLevel && face != -1) {
        BlockID blocks[4];
        blocks[0] = chunkLodMap[detailLevel][flatIndex];
        if (face == 0 || face == 1) {  // X
            blocks[1] =
                chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX + 1, worldY, localZ, detailLevel)];
            blocks[2] =
                chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX, worldY + 1, localZ, detailLevel)];
            blocks[3] =
                chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX + 1, worldY + 1, localZ, detailLevel)];

        } else if (face == 4 || face == 5) {  // Z
            blocks[1] =
                chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX, worldY + 1, localZ, detailLevel)];
            blocks[2] =
                chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX, worldY, localZ + 1, detailLevel)];
            blocks[3] =
                chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX, worldY + 1, localZ + 1, detailLevel)];
        }
        for (int i = 0; i < 4; i++) {
            if (blocks[i] == BlockID::AIR) {
                return BlockID::AIR;
            }
        }
        return BlockID::BEDROCK;
    }

    return chunkLodMap[detailLevel][flatIndex];
}

void Chunk::convertLOD(int newLod) {
    std::vector<BlockID> temp;
    chunkLodMap[detailLevel].swap(temp);
    setLod(newLod);
    generateChunk();
    meshDirty = true;
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

void Chunk::greedyMesh() {
    for (auto& kv : visByFaceType) {
        auto& list = kv.second;
        std::sort(list.begin(), list.end());
    }
    greedyAlgorithm.populatePlanes(visByFaceType, chunkLodMap[detailLevel], detailLevel);

    greedyAlgorithm.firstPassOn(BlockFace::NEG_X);
    greedyAlgorithm.firstPassOn(BlockFace::POS_X);
    greedyAlgorithm.firstPassOn(BlockFace::NEG_Y);
    greedyAlgorithm.firstPassOn(BlockFace::POS_Y);
    greedyAlgorithm.firstPassOn(BlockFace::NEG_Z);
    greedyAlgorithm.firstPassOn(BlockFace::POS_Z);
}

void Chunk::breakBlock(int localX, int localY, int localZ) {
    int flatIndex = ChunkUtils::flattenChunkCoords(localX, localY, localZ, detailLevel);

    chunkLodMap[detailLevel][flatIndex] = BlockID::AIR;
}

void Chunk::placeBlock(int localX, int localY, int localZ, BlockID blockToPlace) {
    int flatIndex = ChunkUtils::flattenChunkCoords(localX, localY, localZ, detailLevel);
    if (flatIndex > highestOccupiedIndex) {
        highestOccupiedIndex = flatIndex;
    }
    chunkLodMap[detailLevel][flatIndex] = blockToPlace;
}

glm::ivec3 Chunk::expandChunkCoords(int flatIndex) {
    glm::ivec3 coords3D;
    coords3D.y = flatIndex / (resolutionXZ * resolutionXZ);
    int remainder = flatIndex % (resolutionXZ * resolutionXZ);
    coords3D.z = remainder / resolutionXZ;
    coords3D.x = remainder % resolutionXZ;

    return coords3D;
}

void Chunk::unload() {
    visByFaceType.clear();
    greedyAlgorithm.unload();
}

std::vector<BlockID> Chunk::getCurrChunkVec() {
    if (hasBeenGenerated[detailLevel])
        return chunkLodMap[detailLevel];
    else {
        std::vector<BlockID> emptyVec;
        return emptyVec;
    }
}
