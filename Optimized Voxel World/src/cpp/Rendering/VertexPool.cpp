#include "h/Rendering/VertexPool.h"
#include <cstring>
#include <iostream>

VertexPool::VertexPool(size_t totalPoolBytes)
    : _poolBytes(totalPoolBytes)
{
}

VertexPool::~VertexPool() {
    if (_mapV) {
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    if (_mapI) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    }
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
    glDeleteBuffers(1, &_indirectBuf);
}

bool VertexPool::initialize() {
    // reserve 80% of bytes for verts, 20% for indices
    _vertRegion = _poolBytes * 4 / 5;
    _idxRegion = _poolBytes - _vertRegion;

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferStorage(GL_ARRAY_BUFFER, _vertRegion, nullptr,
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    _mapV = glMapBufferRange(
        GL_ARRAY_BUFFER, 0, _vertRegion,
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
    );
    if (!_mapV) {
        std::cerr << "VertexPool ERROR: failed to map VBO\n";
        return false;
    }

    glGenBuffers(1, &_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, _idxRegion, nullptr,
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    _mapI = glMapBufferRange(
        GL_ELEMENT_ARRAY_BUFFER, 0, _idxRegion,
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
    );
    if (!_mapI) {
        std::cerr << "VertexPool ERROR: failed to map EBO\n";
        return false;
    }

    glGenBuffers(1, &_indirectBuf);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _indirectBuf);
    glBufferData(GL_DRAW_INDIRECT_BUFFER,
        sizeof(DrawElementsIndirectCommand) * 16384,
        nullptr, GL_DYNAMIC_DRAW);

    _freeV.emplace_back(0, _vertRegion);
    _freeI.emplace_back(0, _idxRegion);

    return true;
}

bool VertexPool::allocateBucket(const ChunkUtils::ChunkCoordPair& key,
    size_t vertexBytes,
    size_t indexCount)
{
    size_t neededVb = vertexBytes;
    size_t vb = ((neededVb + sizeof(Vertex) - 1)
        / sizeof(Vertex))
        * sizeof(Vertex);

    size_t neededIb = indexCount * sizeof(GLuint);
    size_t ib = ((neededIb + sizeof(GLuint) - 1)
        / sizeof(GLuint))
        * sizeof(GLuint);

    size_t offV = SIZE_MAX, offI = SIZE_MAX;

    for (auto it = _freeV.begin(); it != _freeV.end(); ++it) {
        if (it->second >= vb) {
            offV = it->first;
            if (it->second > vb) {
                *it = { it->first + vb, it->second - vb };
            }
            else {
                _freeV.erase(it);
            }
            break;
        }
    }

    for (auto it = _freeI.begin(); it != _freeI.end(); ++it) {
        if (it->second >= ib) {
            offI = it->first;
            if (it->second > ib) {
                *it = { it->first + ib, it->second - ib };
            }
            else {
                _freeI.erase(it);
            }
            break;
        }
    }

    if (offV == SIZE_MAX || offI == SIZE_MAX) {
        std::cerr << "VertexPool ERROR: out of "
            << (offV == SIZE_MAX ? "vertex" : "")
            << ((offV == SIZE_MAX && offI == SIZE_MAX) ? " & " : "")
            << (offI == SIZE_MAX ? "index" : "")
            << " space for chunk (" << key.first
            << "," << key.second << ")\n";
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(_bucketMtx);
        _buckets[key] = {offV, vb, offI, indexCount};
    }

    return true;
}

void VertexPool::updateVertices(const ChunkUtils::ChunkCoordPair& key, const void* data, size_t bytes) {
    std::lock_guard<std::mutex> lock(_bucketMtx);
    auto it = _buckets.find(key);
    if (it == _buckets.end() || bytes > it->second.vertexSizeBytes) return;
    std::memcpy((char*)_mapV + it->second.vertexOffsetBytes, data, bytes);
}

void VertexPool::updateIndices(const ChunkUtils::ChunkCoordPair& key, const GLuint* data, size_t count) {
    std::lock_guard<std::mutex> lock(_bucketMtx);
    auto it = _buckets.find(key);
    if (it == _buckets.end()) {
        std::cerr << "VertexPool ERROR: updateIndices missing bucket for ("
            << key.first << "," << key.second << ")\n";
        return;
    }
    if (count > it->second.indexCount) {
        std::cerr << "VertexPool ERROR: index count " << count
            << " exceeds allocated " << it->second.indexCount
            << " for chunk (" << key.first << "," << key.second << ")\n";
        return;
    }
    std::memcpy((char*)_mapI + it->second.indexOffsetBytes,
        data, count * sizeof(GLuint));
}

void VertexPool::freeBucket(const ChunkUtils::ChunkCoordPair& key) {
    std::lock_guard<std::mutex> lock(_bucketMtx);
    auto it = _buckets.find(key);
    if (it == _buckets.end()) return;
    const auto& b = it->second;
    _freeV.emplace_back(b.vertexOffsetBytes, b.vertexSizeBytes);
    _freeI.emplace_back(b.indexOffsetBytes, b.indexCount * sizeof(GLuint));
    _buckets.erase(it);
}

bool VertexPool::containsBucket(const ChunkUtils::ChunkCoordPair& key) const {
    std::lock_guard<std::mutex> lock(_bucketMtx);
    return _buckets.count(key) != 0;
}

void VertexPool::buildIndirectCommands(const std::vector<ChunkUtils::ChunkCoordPair>& visible) {
    _commands.clear();
    for (auto& c : visible) {
        {
            std::lock_guard<std::mutex> lock(_bucketMtx);
            auto it = _buckets.find(c);
            if (it == _buckets.end()) continue;
            const auto& b = it->second;
            DrawElementsIndirectCommand cmd = {};
            cmd.count = (GLuint)b.indexCount;
            cmd.instanceCount = 1;
            cmd.firstIndex = (GLuint)(b.indexOffsetBytes / sizeof(GLuint));
            cmd.baseVertex = (GLint)(b.vertexOffsetBytes / sizeof(Vertex));
            cmd.baseInstance = 0;
            _commands.push_back(cmd);
        }
    }
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _indirectBuf);
    glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0,
        _commands.size() * sizeof(DrawElementsIndirectCommand),
        _commands.data());
}

void VertexPool::renderIndirect() const {
    if (_commands.empty()) return;
    glMultiDrawElementsIndirect(
        GL_TRIANGLES,
        GL_UNSIGNED_INT,
        nullptr,
        (GLsizei)_commands.size(),
        0
    );
}