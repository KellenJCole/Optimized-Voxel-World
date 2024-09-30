#pragma once
#include <unordered_map>
#include <set>
#include <future>
#include <mutex>
#include "h/Rendering/Renderer.h"
#include "h/Terrain/ChunkLoader.h"
#include "h/Terrain/Chunk.h"
#include "h/Rendering/TextureArray.h"
#include "h/Rendering/Camera.h"

struct PairHash {
    size_t operator()(const std::pair<int, int>& pair) const {
        auto hash1 = std::hash<int>{}(pair.first);
        auto hash2 = std::hash<int>{}(pair.second);
        return hash1 ^ (hash2 << 1);
    }
};

using ChunkCoordPair = std::pair<int, int>;

class WorldManager {
public:
    WorldManager();
    bool initialize(ProcGen* pg);
    void update();
    void updateRenderChunks(int originX, int originZ, int renderRadius, bool unloadAll);
    void render();
    void cleanup();
    void setCamAndShaderPointers(Shader* sha, Camera* cam);
    int getBlockAtGlobal(int worldX, int worldY, int worldZ, bool fromSelf, bool boundaryCheck, int prevLod, int face);
    void switchRenderMethod();
    void breakBlock(int worldX, int worldY, int worldZ);
    void placeBlock(int worldX, int worldY, int worldZ, unsigned char blockToPlace);
    void passWindowPointerToRenderer(GLFWwindow* window);

    // Delete copy constructor and copy assignment operator
    WorldManager(const WorldManager&) = delete;
    WorldManager& operator=(const WorldManager&) = delete;

    std::unordered_map<ChunkCoordPair, std::unique_ptr<Chunk>, PairHash> worldMap;

private:
    void addQuadVerticesAndIndices(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>> quad, ChunkCoordPair chunkCoords, int faceType, int offset, int levelOfDetail);
    void genMeshForSingleChunk(ChunkCoordPair key);
    void markChunkReadyForRender(std::pair<int, int> key);
    void updateMesh(ChunkCoordPair key);

    void loadChunksAsync(const std::vector<std::pair<int, int>>& loadChunks);
    void unloadChunks(const std::vector<std::pair<int, int>>& loadChunks, bool all);

    void updateRenderBuffers();

    int calculateLevelOfDetail(ChunkCoordPair ccp);

    int convertWorldCoordToChunkCoord(int worldCoord);

    glm::vec3 calculatePosition(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>& q, int corner, int faceType, ChunkCoordPair cxcz, int offset, int levelOfDetail);
    glm::vec2 calculateTexCoords(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>& q, int corner);

    Renderer renderer;
    ChunkLoader chunkLoader;
    ProcGen* proceduralAlgorithm;
    TextureArray blockTextureArray;

    // Store unique_ptr to manage Chunk lifetimes
    std::unordered_map<ChunkCoordPair, std::vector<Vertex>, PairHash> verticesByChunk;
    std::unordered_map<ChunkCoordPair, std::vector<unsigned int>, PairHash> indicesByChunk;
    std::unordered_set<ChunkCoordPair, PairHash> preparedChunks;

    // SHADER CAMERA LIGHTS
    Shader* shader;
    Camera* camera;
    glm::vec3 lightPos;
    glm::vec3 lightColor;

    // MULTITHREAD
    std::future<void> loadFuture;
    std::mutex preparedChunksMtx;
    std::recursive_mutex chunkUpdateMtx;
    std::recursive_mutex worldMapMtx;
    std::recursive_mutex renderBuffersMtx;
    std::condition_variable chunkCondition;
    std::atomic<bool> updatedRenderChunks;
    std::atomic<bool> chunkUpdate;
    std::atomic<bool> stopAsync;


    double lastFrustumCheck;
    int renderRadius;
};