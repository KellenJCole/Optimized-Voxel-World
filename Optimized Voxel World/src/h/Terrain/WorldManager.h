#pragma once

#include <unordered_map>
#include <set>
#include <future>
#include <mutex>

#include "h/Terrain/Chunk.h"
#include "h/Terrain/ChunkLoader.h"
#include "h/Rendering/Camera.h"
#include "h/Rendering/Renderer.h"
#include "h/Rendering/Buffering/TextureArray.h"

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
    std::vector<BlockID> getChunkVector(ChunkCoordPair key);

    void setWindowPointer(GLFWwindow* window) { renderer.setWindowPointer(window); }
    void passObjectPointers(Shader* sha, Camera* cam) { shader = sha; camera = cam; }
    bool getReadyForPlayerUpdate() { return readyForPlayerUpdate; }
    void switchRenderMethod() { renderer.toggleFillLine(); }

    std::unordered_map<ChunkCoordPair, std::unique_ptr<Chunk>, PairHash> worldMap;

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
    Renderer renderer;
    ChunkLoader chunkLoader;
    ProcGen* proceduralAlgorithm;
    TextureArray blockTextureArray;

    // Store unique_ptr to manage Chunk lifetimes
    std::unordered_set<ChunkCoordPair, PairHash> worldKeysSet;
    std::vector<ChunkCoordPair> currentRenderChunks;

    // SHADER CAMERA LIGHTS
    Shader* shader;
    Camera* camera;
    glm::vec3 lightPos;
    glm::vec3 lightColor;

    // MULTITHREAD
    std::future<void> loadFuture;
    std::recursive_mutex chunkUpdateMtx;
    std::recursive_mutex worldMapMtx;
    std::recursive_mutex renderBuffersMtx;
    std::atomic<bool> updatedRenderChunks;
    std::atomic<bool> stopAsync;

    bool readyForPlayerUpdate;
    double lastFrustumCheck;
    int renderRadius;
};