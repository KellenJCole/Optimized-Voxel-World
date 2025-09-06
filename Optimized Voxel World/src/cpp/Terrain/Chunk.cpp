#include "h/Terrain/Chunk.h"

#include <chrono>

#include "GLFW/glfw3.h"
#include "h/Terrain/WorldManager.h"

Chunk::Chunk()
    :
    chunkX(std::numeric_limits<int>::min()),
    chunkZ(std::numeric_limits<int>::min()),
    resolutionXZ(64),
    resolutionY(256),
    detailLevel(std::numeric_limits<int>::min()),
    highestOccupiedIndex(std::numeric_limits<int>::min()),
    world(nullptr)
{
    for (int index = 0; index < 6; index++) neighborOffsets[index] = std::numeric_limits<int>::min();
}

void Chunk::generateChunk(ProcGen& proceduralGenerator) {
    chunkLodMap[detailLevel].resize(ChunkUtils::getChunkLength(detailLevel), BlockID::AIR);
    highestOccupiedIndex = proceduralGenerator.generateChunk(chunkLodMap[detailLevel], std::make_pair(chunkX, chunkZ), detailLevel);
}

#define CHECK_PERFORMED_MASK(face) (1 << (face * 2))
#define CHECK_RESULT_MASK(face) (1 << (face * 2 + 1))

void Chunk::startMeshing() {
    std::vector<uint16_t> neighborCheckCache;
    neighborCheckCache.resize(resolutionXZ * resolutionXZ * resolutionY, 0);
    int resolution = ChunkUtils::WIDTH >> detailLevel;
    for (int blockIndex = 0; blockIndex <= highestOccupiedIndex; blockIndex++) {
        if (chunkLodMap[detailLevel][blockIndex] != BlockID::AIR) {
            BlockFaceBitmask mask = cullFaces(blockIndex, neighborCheckCache);
            if (mask != BlockFaceBitmask::NONE) {  // If at least one face of block is visible
                for (int f = 0; f < toInt(BlockFace::Count); f++) {
                    BlockFace face = static_cast<BlockFace>(f);
                    BlockFaceBitmask bitmask = static_cast<BlockFaceBitmask>(1u << f);
                    if (has(mask, bitmask)) visByFaceType[face].push_back(blockIndex);
                }
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

    neighborCache[neighborIndex] |= CHECK_PERFORMED_MASK(toInt(oppositeFace));
    neighborCache[neighborIndex] &= ~CHECK_RESULT_MASK(toInt(oppositeFace));
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
        blocks[1] = chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX, localY + 1, localZ, detailLevel)];
        if (face == BlockFace::NEG_X || face == BlockFace::POS_X) {
            blocks[2] = chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX,         localY,         localZ + 1,     detailLevel)];
            blocks[3] = chunkLodMap[detailLevel][ChunkUtils::flattenChunkCoords(localX,         localY + 1,     localZ + 1,     detailLevel)];

        }
        else if (face == BlockFace::NEG_Z || face == BlockFace::POS_Z) {
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
    setLodVariables(newLod);
}

void Chunk::setLodVariables(int lod) {
    detailLevel = lod;

    resolutionXZ = ChunkUtils::WIDTH >> detailLevel;
    resolutionY = ChunkUtils::HEIGHT >> detailLevel;

    neighborOffsets[0] = -1;
    neighborOffsets[1] = 1;
    neighborOffsets[2] = -resolutionXZ * resolutionXZ;
    neighborOffsets[3] = resolutionXZ * resolutionXZ;
    neighborOffsets[4] = -resolutionXZ;
    neighborOffsets[5] = resolutionXZ;
}

void Chunk::greedyMesh() {
    for (auto& kv : visByFaceType) {
        auto& list = kv.second;
        std::sort(list.begin(), list.end());
    }
    greedyAlgorithm.populatePlanes(visByFaceType, chunkLodMap[detailLevel], detailLevel);

    for (int f = 0; f < toInt(BlockFace::Count); f++) {
        BlockFace face = static_cast<BlockFace>(f);
        greedyAlgorithm.firstPassOn(face);

    }
}

bool Chunk::breakBlock(int localX, int localY, int localZ) {
    int flatIndex = ChunkUtils::flattenChunkCoords(localX, localY, localZ, detailLevel);

    chunkLodMap[detailLevel][flatIndex] = BlockID::AIR;
    return true;
}

bool Chunk::placeBlock(int localX, int localY, int localZ, BlockID blockToPlace) {
    int flatIndex = ChunkUtils::flattenChunkCoords(localX, localY, localZ, detailLevel);
    if (flatIndex > highestOccupiedIndex) {
        highestOccupiedIndex = flatIndex;
    }
    chunkLodMap[detailLevel][flatIndex] = blockToPlace;

    return true;
}

glm::ivec3 Chunk::expandChunkCoords(int flatIndex) const {
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
    return chunkLodMap[detailLevel];
}
