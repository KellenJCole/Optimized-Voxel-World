#pragma once

#include <unordered_map>
#include <set>
#include <future>
#include <shared_mutex>

#include "h/Terrain/Chunk.h"
#include "h/Terrain/ChunkLoader.h"
#include "h/Rendering/Camera.h"
#include "h/Rendering/TerrainRenderer.h"

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

    std::shared_ptr<const std::vector<BlockID>> tryGetChunkSnapshot(ChunkUtils::ChunkCoordPair key);

private:
    void genChunkMesh(ChunkUtils::ChunkCoordPair key);

    void loadChunksAsync(const std::vector<std::pair<int, int>>& loadChunks);
    void unloadChunks(const std::vector<std::pair<int, int>>& loadChunks, bool all);

    int calculateLevelOfDetail(ChunkUtils::ChunkCoordPair ccp);

    VertexPool* vertexPool;
    TerrainRenderer renderer;
    ChunkLoader chunkLoader;
    ProcGen* proceduralGenerator;

    std::unordered_set<ChunkUtils::ChunkCoordPair, ChunkUtils::PairHash> chunkKeys;
    std::vector<ChunkUtils::ChunkCoordPair> unmeshedKeysOrder;
    std::unordered_set<ChunkUtils::ChunkCoordPair, ChunkUtils::PairHash> unmeshedKeysSet;

    inline void insertUnmeshed(const ChunkUtils::ChunkCoordPair& key) {
        if (unmeshedKeysSet.insert(key).second) {
            unmeshedKeysOrder.push_back(key);
        }
    }

    std::vector<ChunkUtils::ChunkCoordPair> currentRenderChunks;

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

    std::unordered_map<ChunkUtils::ChunkCoordPair, std::unique_ptr<Chunk>, ChunkUtils::PairHash> worldMap;
};