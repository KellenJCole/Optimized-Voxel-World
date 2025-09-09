#pragma once

#include "h/Rendering/Utility/GLErrorCatcher.h"
#include "h/Rendering/Utility/BlockGeometry.h"
#include "h/Terrain/Utility/ChunkUtils.h"
#include <glad/glad.h>

#include <unordered_map>
#include <vector>
#include <cstddef>
#include <mutex>

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

    bool allocateBucket(const ChunkUtils::ChunkCoordPair& chunkKey, size_t vertexBytes, size_t indexCount);

    void updateVertices(const ChunkUtils::ChunkCoordPair& chunkKey, const void* data, size_t bytes);
    void updateIndices(const ChunkUtils::ChunkCoordPair& chunkKey, const GLuint* data, size_t count);

    void freeBucket(const ChunkUtils::ChunkCoordPair& chunkKey);

    bool containsBucket(const ChunkUtils::ChunkCoordPair& chunkKey) const;

    void buildIndirectCommands(const std::vector<ChunkUtils::ChunkCoordPair>& visibleChunks);
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
    std::unordered_map<ChunkUtils::ChunkCoordPair, BucketInfo, ChunkUtils::PairHash> _buckets;

    std::vector<DrawElementsIndirectCommand> _commands;
};