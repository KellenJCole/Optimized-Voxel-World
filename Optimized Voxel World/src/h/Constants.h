#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace ChunkUtils {
    constexpr int WIDTH = 64;
    constexpr int HEIGHT = 256;
    constexpr int DEPTH = 64;

    inline int convertWorldCoordToChunkCoord(int worldCoord) {
        return (worldCoord >= 0) ? (worldCoord / ChunkUtils::WIDTH) : ((worldCoord - (ChunkUtils::WIDTH - 1)) / ChunkUtils::WIDTH);
    }

    inline int convertWorldCoordToLocalCoord(int worldCoord) {
        return (((worldCoord % ChunkUtils::WIDTH) + ChunkUtils::WIDTH) % ChunkUtils::WIDTH);
    }
}

#endif