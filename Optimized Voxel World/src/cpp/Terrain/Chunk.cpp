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
    int localX = coords.x, localY = coords.y, localZ = coords.z;

    for (int f = 0; f < toInt(BlockFace::Count); ++f) {
        BlockFace face = static_cast<BlockFace>(f);

        if (hasNeighborCheckBeenPerformed(blockIndex, face, neighborCache)) { // neighbor is cached
            if (neighborCache[blockIndex] & CHECK_RESULT_MASK(toInt(face))) {
                mask |= static_cast<BlockFaceBitmask>(1u << toInt(face));
            }
        } 
        else {
            bool neighborIsAir = false;

            bool isBorderXNeg = (localX == 0)                   && (face == BlockFace::NEG_X);
            bool isBorderXPos = (localX == resolutionXZ - 1)    && (face == BlockFace::POS_X);
            bool isBorderZNeg = (localZ == 0)                   && (face == BlockFace::NEG_Z);
            bool isBorderZPos = (localZ == resolutionXZ - 1)    && (face == BlockFace::POS_Z);

            bool borderCheck = isBorderXNeg || isBorderXPos || isBorderZNeg || isBorderZPos;

            if (!borderCheck) { // Neighbor is within local chunk
                if (localY == 0 && face == BlockFace::NEG_Y) {
                    neighborIsAir = false;
                }
                else if (localY == (resolutionY - 1) && face == BlockFace::POS_Y) {
                    neighborIsAir = true;
                }
                else {
                    int neighborIndex = blockIndex + neighborOffsets[toInt(face)];
                    neighborIsAir = (chunkLodMap[detailLevel][neighborIndex] == BlockID::AIR);
                    markNeighborsCheck(neighborIndex, face, neighborCache);
                }
            }
            else {
                int scale = 1 << detailLevel;
                int baseX = chunkX * ChunkUtils::WIDTH;
                int baseZ = chunkZ * ChunkUtils::DEPTH;

                int neighborX, neighborY, neighborZ;
                neighborY = localY * scale;

                if (isBorderXNeg) {
                    neighborX = baseX - 1;
                    neighborZ = baseZ + (localZ * scale);
                }
                else if (isBorderZNeg) {
                    neighborX = baseX + (localX * scale);
                    neighborZ = baseZ - 1;
                }
                else if (isBorderXPos) {
                    neighborX = baseX + ChunkUtils::WIDTH;
                    neighborZ = baseZ + (localZ * scale);
                }
                else if (isBorderZPos) {
                    neighborX = baseX + (localX * scale);
                    neighborZ = baseZ + ((localZ + 1) * scale);
                }

                neighborIsAir = (world->getBlockAtGlobal(neighborX, neighborY, neighborZ, face, detailLevel) == BlockID::AIR);
            }

            if (neighborIsAir) {
                mask |= static_cast<BlockFaceBitmask>(1u << toInt(face));
            }
        }
    }

    return mask;
}

void Chunk::markNeighborsCheck(int neighborIndex, BlockFace face, std::vector<uint16_t>& neighborCache) {
    BlockFace oppositeFace = opposite(face);

    if (neighborIndex >= 0 && neighborIndex < resolutionXZ * resolutionXZ * resolutionY) {
        neighborCache[neighborIndex] |= CHECK_PERFORMED_MASK(toInt(oppositeFace));
        neighborCache[neighborIndex] &= ~CHECK_RESULT_MASK(toInt(oppositeFace));
    }
}

bool Chunk::hasNeighborCheckBeenPerformed(int blockIndex, BlockFace face, std::vector<uint16_t>& neighborCache) {
    return neighborCache[blockIndex] & CHECK_PERFORMED_MASK(toInt(face));
}

BlockID Chunk::getBlockAt(int worldX, int worldY, int worldZ) {

    int localX = ChunkUtils::convertWorldCoordToLocalCoord(worldX);
    int localY = worldY;
    int localZ = ChunkUtils::convertWorldCoordToLocalCoord(worldZ);

    int flatIndex = ChunkUtils::flattenChunkCoords(localX, localY, localZ, detailLevel);
    return chunkLodMap[detailLevel][flatIndex];
}

BlockID Chunk::getBlockAt(int worldX, int worldY, int worldZ, BlockFace face, int sourceLod) {
    int localX = ChunkUtils::convertWorldCoordToLocalCoord(worldX);
    int localY = worldY;
    int localZ = ChunkUtils::convertWorldCoordToLocalCoord(worldZ);
    
    int scale = 1 << detailLevel;

    localX /= scale;
    localY /= scale;
    localZ /= scale;

    int flatIndex = ChunkUtils::flattenChunkCoords(localX, localY, localZ, detailLevel);
    if (sourceLod > detailLevel) { // Meshing - a block of low detail is looking at a block of higher detail (smaller)
        BlockID blocks[4];
        blocks[0] = chunkLodMap[detailLevel][flatIndex];
        if (face == BlockFace::NEG_X || face == BlockFace::POS_X) {
            blocks[1] = chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX,         localY,         localZ + 1,     detailLevel)];
            blocks[2] = chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX,         localY + 1,     localZ,         detailLevel)];
            blocks[3] = chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX,         localY + 1,     localZ + 1,     detailLevel)];

        }
        else if (face == BlockFace::NEG_Z || face == BlockFace::POS_Z) {
            blocks[1] = chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX,         localY + 1,     localZ,         detailLevel)];
            blocks[2] = chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX + 1,     localY,         localZ,         detailLevel)];
            blocks[3] = chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX + 1,     localY + 1,     localZ,         detailLevel)];
        }

        for (int i = 0; i < 4; i++) {
            if (blocks[i] == BlockID::AIR) {
                return BlockID::AIR;
            }
        }
        return BlockID::BEDROCK;  // arbitrary - solid
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

bool Chunk::breakBlock(int localX, int localY, int localZ) {
    int flatIndex = ChunkUtils::flattenChunkCoords(localX, localY, localZ, detailLevel);

    if (chunkLodMap[detailLevel][flatIndex] != BlockID::BEDROCK) {
        chunkLodMap[detailLevel][flatIndex] = BlockID::AIR;
        return true;
    }
    return false;
}

bool Chunk::placeBlock(int localX, int localY, int localZ, BlockID blockToPlace) {
    int flatIndex = ChunkUtils::flattenChunkCoords(localX, localY, localZ, detailLevel);
    if (flatIndex > highestOccupiedIndex) {
        highestOccupiedIndex = flatIndex;
    }
    chunkLodMap[detailLevel][flatIndex] = blockToPlace;

    return true;
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
