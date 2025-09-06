#pragma once

namespace ChunkUtils {
    constexpr int WIDTH = 64;
    constexpr int HEIGHT = 256;
    constexpr int DEPTH = 64;

    constexpr int worldToChunkCoord(int worldCoord) {
        return (worldCoord >= 0) ? (worldCoord / ChunkUtils::WIDTH) : ((worldCoord - (ChunkUtils::WIDTH - 1)) / ChunkUtils::WIDTH);
    }

    constexpr int convertWorldCoordToLocalCoord(int worldCoord) {
        return (((worldCoord % ChunkUtils::WIDTH) + ChunkUtils::WIDTH) % ChunkUtils::WIDTH);
    }

    constexpr int flattenChunkCoords(int x, int y, int z, int detailLevel) {
        int w = ChunkUtils::WIDTH >> detailLevel;
        return x + (z * w) + (y * w * w);
    }

    constexpr int getChunkLength(int detailLevel) {
        int w = ChunkUtils::WIDTH >> detailLevel;
        int h = ChunkUtils::HEIGHT >> detailLevel;
        int d = ChunkUtils::DEPTH >> detailLevel;

        return w * h * d;
    }

}
