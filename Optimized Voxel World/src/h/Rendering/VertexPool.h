#pragma once

#include <unordered_map>
#include <vector>
#include <cstddef>
#include <glad/glad.h>
#include <mutex>
#include "h/Rendering/Utility/GLErrorCatcher.h"
#include "h/Rendering/Utility/BlockGeometry.h"

using ChunkCoordPair = std::pair<int, int>;

struct PairHash {
    size_t operator()(const ChunkCoordPair& p) const {
        return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
    }
};

struct BucketInfo {
    size_t vertexOffsetBytes;
    size_t vertexSizeBytes;
    size_t indexOffsetBytes;
    size_t indexCount;
};

struct DrawElementsIndirectCommand {
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLuint baseVertex;
    GLuint baseInstance;
};

class VertexPool {
public:
    VertexPool(size_t totalPoolBytes);
    ~VertexPool();

    bool initialize();

    bool allocateBucket(const ChunkCoordPair& chunkKey, size_t vertexBytes, size_t indexCount);

    void updateVertices(const ChunkCoordPair& chunkKey, const void* data, size_t bytes);
    void updateIndices(const ChunkCoordPair& chunkKey, const GLuint* data, size_t count);

    void freeBucket(const ChunkCoordPair& chunkKey);

    bool containsBucket(const ChunkCoordPair& chunkKey) const;

    void buildIndirectCommands(const std::vector<ChunkCoordPair>& visibleChunks);
    void renderIndirect() const;

    GLuint getVBO() const { return _vbo; }
    GLuint getEBO() const { return _ebo; }
    GLuint getIndirectBuf() const { return _indirectBuf; }

private:
    GLuint _vbo = 0;
    GLuint _ebo = 0;
    GLuint _indirectBuf = 0;

    void* _mapV = nullptr;
    void* _mapI = nullptr;

    size_t _poolBytes;
    size_t _vertRegion;
    size_t _idxRegion;

    std::vector<std::pair<size_t, size_t>> _freeV, _freeI;

    mutable std::mutex _bucketMtx;
    std::unordered_map<ChunkCoordPair, BucketInfo, PairHash> _buckets;

    std::vector<DrawElementsIndirectCommand> _commands;
};