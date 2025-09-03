#pragma once
#ifndef CHUNKUTILS_H
#define CHUNKUTILS_H

namespace ChunkUtils {
    constexpr int WIDTH = 64;
    constexpr int HEIGHT = 256;
    constexpr int DEPTH = 64;

    inline int worldToChunkCoord(int worldCoord) {
        return (worldCoord >= 0) ? (worldCoord / ChunkUtils::WIDTH) : ((worldCoord - (ChunkUtils::WIDTH - 1)) / ChunkUtils::WIDTH);
    }

    inline int convertWorldCoordToLocalCoord(int worldCoord) {
        return (((worldCoord % ChunkUtils::WIDTH) + ChunkUtils::WIDTH) % ChunkUtils::WIDTH);
    }

    inline int flattenChunkCoords(int x, int y, int z, int detailLevel) {
        int w = ChunkUtils::WIDTH >> detailLevel;
        return x + (z * w) + (y * w * w);
    }

}

#endif
