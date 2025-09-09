#pragma once

#include <unordered_map>
#include <set>
#include <future>
#include <shared_mutex>

#include "h/Terrain/Chunk.h"
#include "h/Terrain/ChunkLoader.h"
#include "h/Rendering/Camera.h"
#include "h/Rendering/TerrainRenderer.h"

using ChunkCoordPair = std::pair<int, int>;

class WorldManager {
public:
    WorldManager();
    WorldManager(const WorldManager&) = delete;
    WorldManager& operator=(const WorldManager&) = delete;

    bool initialize(ProcGen* pg, VertexPool* vp);

    void update();
    void render();
    void cleanup() { renderer.cleanup(); }

    void updateRenderChunks(int originX, int originZ, int renderRadius, bool unloadAll);

    BlockID getBlockAtGlobal(int worldX, int worldY, int worldZ);
    BlockID getBlockAtGlobal(int worldX, int worldY, int worldZ, BlockFace face, int sourceLod);
    void breakBlock(int worldX, int worldY, int worldZ);
    void placeBlock(int worldX, int worldY, int worldZ, BlockID blockToPlace);

    void setWindowPointer(GLFWwindow* window) { renderer.setWindowPointer(window); }
    void passCameraPointer(Camera* cam) { camera = cam; }
    bool getReadyForPlayerUpdate() { return readyForPlayerUpdate; }
    void switchRenderMethod() { renderer.toggleFillLine(); }

    std::shared_ptr<const std::vector<BlockID>> tryGetChunkSnapshot(ChunkCoordPair key);

private:
    void addVerticesForQuad(std::vector<Vertex>& verts, std::vector<unsigned int>& inds, MeshUtils::Quad quad, ChunkCoordPair chunkCoords,
        int faceType, int sliceIndex, int levelOfDetail, unsigned int baseIndex);
    glm::vec3 calculatePosition(MeshUtils::Quad& quad, int corner, size_t faceType, ChunkCoordPair cxcz, int sliceIndex, int levelOfDetail);
    glm::vec2 calculateTexCoords(MeshUtils::Quad& quad, int corner);

    void genChunkMesh(ChunkCoordPair key);

    void loadChunksAsync(const std::vector<std::pair<int, int>>& loadChunks);
    void unloadChunks(const std::vector<std::pair<int, int>>& loadChunks, bool all);

    int calculateLevelOfDetail(ChunkCoordPair ccp);

    VertexPool* vertexPool;
    TerrainRenderer renderer;
    ChunkLoader chunkLoader;
    ProcGen* proceduralGenerator;

    std::unordered_set<ChunkCoordPair, PairHash> chunkKeys;
    std::vector<ChunkCoordPair> unmeshedKeysOrder;
    std::unordered_set<ChunkCoordPair, PairHash> unmeshedKeysSet;

    inline void insertUnmeshed(const ChunkCoordPair& key) {
        if (unmeshedKeysSet.insert(key).second) {
            unmeshedKeysOrder.push_back(key);
        }
    }

    std::vector<ChunkCoordPair> currentRenderChunks;

    Camera* camera;

    // MULTITHREAD
    std::future<void> loadFuture;
    std::shared_mutex worldMapMtx;
    std::mutex renderBuffersMtx;
    std::atomic<bool> updatedRenderChunks;
    std::atomic<bool> stopAsync;

    bool readyForPlayerUpdate;
    double lastFrustumCheck;
    int renderRadius;

    std::unordered_map<ChunkCoordPair, std::unique_ptr<Chunk>, PairHash> worldMap;
};