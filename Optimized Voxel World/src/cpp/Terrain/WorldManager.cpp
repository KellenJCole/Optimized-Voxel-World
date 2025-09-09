#include "h/Terrain/WorldManager.h"
#include <vector>
#include <iostream>

WorldManager::WorldManager()
	: vertexPool(nullptr)
	, proceduralGenerator(nullptr)
	, camera(nullptr)
	, updatedRenderChunks(false)
	, readyForPlayerUpdate(false)
	, renderRadius(std::numeric_limits<int>::min())

{
	lastFrustumCheck = glfwGetTime();
	stopAsync.store(false);
}

bool WorldManager::initialize(ProcGen* pg, VertexPool* vp) {

	if (!renderer.initialize()) {
		std::cerr << "Renderer failed to initialize in World Manager\n";
		return false;
	}

	vertexPool = vp;
	renderer.setVertexPoolPointer(vp);

	proceduralGenerator = pg;

	return true;
}

void WorldManager::update() {
	double now = glfwGetTime();
	if ((now - lastFrustumCheck) >= 0.01) {
		lastFrustumCheck = now;

		currentRenderChunks = camera->getVisibleChunks(renderRadius);
		renderer.updateRenderChunks(currentRenderChunks);
	}
}

void WorldManager::updateRenderChunks(int originX, int originZ, int renderRadius, bool unloadAll) {
	this->renderRadius = renderRadius;

	auto loadVector = chunkLoader.getLoadList(originX, originZ, renderRadius);

	if (loadFuture.valid()) {
		stopAsync.store(true);		// Signal to stop the async task
		loadFuture.get();			// Wait for the async task to finish
		stopAsync.store(false);		// Reset the stop flag
	}

	unloadChunks(loadVector, unloadAll);  // Unload before starting a new task

	loadFuture = std::async(std::launch::async, [this, loadVector] {
		this->loadChunksAsync(loadVector);
	});
} 

void WorldManager::loadChunksAsync(const std::vector<std::pair<int, int>>& loadChunks) {
	for (const auto& key : loadChunks) {
		if (stopAsync.load()) break;

		int lod = calculateLevelOfDetail(key);
		if (chunkKeys.find(key) == chunkKeys.end()) {
			auto newChunk = std::make_unique<Chunk>();
			newChunk->setChunkCoords(key.first, key.second);
			newChunk->setWorldReference(this);
			newChunk->setLodVariables(lod);
			newChunk->generateChunk(*proceduralGenerator);

			readyForPlayerUpdate = false;
			{
				std::unique_lock<std::shared_mutex> worldLock(worldMapMtx);
				worldMap[key] = std::move(newChunk);
			}
			readyForPlayerUpdate = true;
			chunkKeys.insert(key);
			insertUnmeshed(key);
		}
	}

	for (const auto& key : loadChunks) {
		if (stopAsync.load()) break;

		if (chunkKeys.find(key) != chunkKeys.end()) {
			std::unique_lock<std::shared_mutex> worldLock(worldMapMtx);
			int lod = calculateLevelOfDetail(key);
			if (worldMap[key]->getCurrentLod() != lod) {
				worldMap[key]->convertLOD(lod);
				worldMap[key]->generateChunk(*proceduralGenerator);
				insertUnmeshed(key);

				insertUnmeshed({ key.first - 1, key.second });
				insertUnmeshed({ key.first + 1, key.second });
				insertUnmeshed({ key.first, key.second - 1 });
				insertUnmeshed({ key.first, key.second + 1 });
			}
		}
	}

	std::unordered_set<ChunkCoordPair, PairHash> meshedKeys;
	for (const auto& key : unmeshedKeysOrder) {
		if (stopAsync.load()) break;
		if (unmeshedKeysSet.find(key) == unmeshedKeysSet.end()) continue;
		genChunkMesh(key);
		meshedKeys.insert(key);
	}

	for (const auto& key : meshedKeys) unmeshedKeysSet.erase(key);

	if (unmeshedKeysSet.empty())
		unmeshedKeysOrder.clear();
}

int WorldManager::calculateLevelOfDetail(ChunkCoordPair ccp) {
	glm::vec3 cameraPos = camera->getCameraPos();
	ChunkCoordPair cameraKey = { 
		ChunkUtils::worldToChunkCoord(static_cast<int>(floor(cameraPos.x))),
		ChunkUtils::worldToChunkCoord(static_cast<int>(floor(cameraPos.z)))
	};
	
	float distance = (float)(sqrt(pow(abs(ccp.first - cameraKey.first), 2) + pow(abs(ccp.second - cameraKey.second), 2)));

	// These values aren't final
	if (distance <= 10) return 0;
	else if (distance <= 25) return 1;
	else if (distance <= 50) return 2;
	else if (distance <= 70) return 3;
	else if (distance <= 100) return 4;
	else if (distance <= 512) return 5;
	else return 6;
}

void WorldManager::unloadChunks(const std::vector<std::pair<int, int>>& loadChunks, bool all) { // all - unload ALL for regeneration or unload those out of render distance
	const std::unordered_set<std::pair<int, int>, PairHash> keep(loadChunks.begin(), loadChunks.end());

	std::vector<ChunkCoordPair> toDelete;
	{
		std::shared_lock<std::shared_mutex> mapRead(worldMapMtx);
		toDelete.reserve(worldMap.size());
		if (!all) {
			for (const auto& kv : worldMap) {
				if (!keep.count(kv.first)) toDelete.push_back(kv.first);
			}
		}
		else {
			for (const auto& kv : worldMap) toDelete.push_back(kv.first);
		}
	}

	{
		std::lock_guard<std::mutex> renderLock(renderBuffersMtx);
		for (const auto& key : toDelete) {
			vertexPool->freeBucket(key);
		}
	}

	{
		std::unique_lock<std::shared_mutex> mapWrite(worldMapMtx);
		if (all) {
			for (const auto& kv : worldMap) chunkKeys.erase(kv.first);
			worldMap.clear();
		}
		else {
			for (const auto& key : toDelete) {
				chunkKeys.erase(key);
				worldMap.erase(key);
			}
		}
	}
}

BlockID WorldManager::getBlockAtGlobal(int worldX, int worldY, int worldZ) {
	int chunkX = ChunkUtils::worldToChunkCoord(worldX);
	int chunkZ = ChunkUtils::worldToChunkCoord(worldZ);
	std::pair<int, int> chunkKey = { chunkX, chunkZ };

	if (chunkKeys.find(chunkKey) != chunkKeys.end()) {
		std::shared_lock<std::shared_mutex> worldLock(worldMapMtx);
		return worldMap[chunkKey]->getBlockAt(worldX, worldY, worldZ);
	}
	else return BlockID::NONE;
}

BlockID WorldManager::getBlockAtGlobal(int worldX, int worldY, int worldZ, BlockFace face, int sourceLod) {
	int chunkX = ChunkUtils::worldToChunkCoord(worldX);
	int chunkZ = ChunkUtils::worldToChunkCoord(worldZ);
	std::pair<int, int> chunkKey = { chunkX, chunkZ };

	if (chunkKeys.find(chunkKey) != chunkKeys.end()) {
		std::shared_lock<std::shared_mutex> worldLock(worldMapMtx);
		return worldMap[chunkKey]->getBlockAt(worldX, worldY, worldZ, face, sourceLod);
	}
	else return BlockID::NONE;
}

void WorldManager::breakBlock(int worldX, int worldY, int worldZ) {
	int chunkX = ChunkUtils::worldToChunkCoord(worldX);
	int chunkZ = ChunkUtils::worldToChunkCoord(worldZ);
	ChunkCoordPair key = { chunkX, chunkZ };
	{

		if (chunkKeys.find(key) != chunkKeys.end()) {
			int localX = ChunkUtils::convertWorldCoordToLocalCoord(worldX);
			int localZ = ChunkUtils::convertWorldCoordToLocalCoord(worldZ);

			{
				std::unique_lock<std::shared_mutex> worldLock(worldMapMtx);
				worldMap[key]->breakBlock(localX, worldY, localZ);
			}

			genChunkMesh(key);

			if (localX == 0) {
				genChunkMesh({ key.first - 1, key.second });
			}
			else if (localX == ChunkUtils::WIDTH - 1) {
				genChunkMesh({ key.first + 1, key.second });
			}
			if (localZ == 0) {
				genChunkMesh({ key.first, key.second - 1 });
			}
			else if (localZ == ChunkUtils::DEPTH - 1) {
				genChunkMesh({ key.first, key.second + 1 });
			}
		}
	}
}

void WorldManager::placeBlock(int worldX, int worldY, int worldZ, BlockID blockToPlace) {
	int chunkX = ChunkUtils::worldToChunkCoord(worldX);
	int chunkZ = ChunkUtils::worldToChunkCoord(worldZ);
	ChunkCoordPair key = { chunkX, chunkZ };

	if (chunkKeys.find(key) != chunkKeys.end()) {
		int localX = ChunkUtils::convertWorldCoordToLocalCoord(worldX);
		int localZ = ChunkUtils::convertWorldCoordToLocalCoord(worldZ);

		{
			std::unique_lock<std::shared_mutex> worldLock(worldMapMtx);
			worldMap[key]->placeBlock(localX, worldY, localZ, blockToPlace);
		}

		genChunkMesh(key);

		if (localX == 0) {
			genChunkMesh({ key.first - 1, key.second });
		}
		else if (localX == ChunkUtils::WIDTH - 1) {
			genChunkMesh({ key.first + 1, key.second });
		}

		if (localZ == 0) {
			genChunkMesh({ key.first, key.second - 1 });
		}
		else if (localZ == ChunkUtils::DEPTH - 1) {
			genChunkMesh({ key.first, key.second + 1 });
		}
	}
}

void WorldManager::genChunkMesh(ChunkCoordPair key) {
	Chunk* chunk = nullptr;
	int lod = -1;

	{
		std::shared_lock<std::shared_mutex> worldLock(worldMapMtx);
		auto it = worldMap.find(key);
		if (it == worldMap.end()) return;	// this happens sometimes... How? I'll find out another time...

		chunk = worldMap[key].get();
		lod = chunk->getCurrentLod();
	}

	{
		std::unique_lock<std::shared_mutex> worldLock(worldMapMtx);
		auto it = worldMap.find(key);
	}

	chunk->startMeshing();
	chunk->greedyMesh();

	// Release mesh from GPU if it exists
	vertexPool->freeBucket(key);

	MeshUtils::FaceMeshGraphs greedyMeshes;

	{
		std::shared_lock<std::shared_mutex> worldLock(worldMapMtx);
		
		for (int f = 0; f < toInt(BlockFace::Count); ++f) {
			greedyMeshes[f] = worldMap[key]->getMeshGraph(static_cast<BlockFace>(f));
        }
	}

	std::vector<Vertex> verts;
	std::vector<unsigned int> inds;
	unsigned int baseIndex = 0;

	for (int f = 0; f < toInt(BlockFace::Count); ++f) {
		for (const auto& slice : greedyMeshes[f]) {
            int sliceIdx = slice.sliceIndex;
			for (const auto& quad : slice.quads) {
				addVerticesForQuad(verts, inds, quad, key, f, sliceIdx, lod, baseIndex);
				baseIndex += 4;
			}
		}
	}

	vertexPool->allocateBucket(key, verts.size() * sizeof(Vertex), inds.size());
	vertexPool->updateVertices(key, verts.data(), verts.size() * sizeof(Vertex));
	vertexPool->updateIndices(key, inds.data(), inds.size());

	{
		std::unique_lock<std::shared_mutex> worldLock(worldMapMtx);
		worldMap[key]->unload();
	}
}

void WorldManager::addVerticesForQuad(std::vector<Vertex>& verts, std::vector<unsigned int>& inds, MeshUtils::Quad quad, ChunkCoordPair chunkCoords, int faceType, int sliceIndex, int levelOfDetail, unsigned int baseIndex) {
	// Calculate positions for each corner of the quad
	glm::vec3 bl = calculatePosition(quad, 0, faceType, chunkCoords, sliceIndex, levelOfDetail);
    glm::vec3 br = calculatePosition(quad, 1, faceType, chunkCoords, sliceIndex, levelOfDetail);
    glm::vec3 tl = calculatePosition(quad, 2, faceType, chunkCoords, sliceIndex, levelOfDetail);
    glm::vec3 tr = calculatePosition(quad, 3, faceType, chunkCoords, sliceIndex, levelOfDetail);

	// Calculate texture coordinates for each corner
	glm::vec2 texBL = calculateTexCoords(quad, 0);
	glm::vec2 texBR = calculateTexCoords(quad, 1);
	glm::vec2 texTL = calculateTexCoords(quad, 2);
	glm::vec2 texTR = calculateTexCoords(quad, 3);

	// Define normal based on face type
	int normal = faceType;
	BlockTextureID tex = quad.tex;

	// Add vertices for the quad
    verts.push_back(Vertex{bl, texBL, normal + static_cast<int>(tex) * 16});
    verts.push_back(Vertex{br, texBR, normal + static_cast<int>(tex) * 16});
    verts.push_back(Vertex{tr, texTR, normal + static_cast<int>(tex) * 16});
    verts.push_back(Vertex{tl, texTL, normal + static_cast<int>(tex) * 16});

	// Add indices for two triangles making up the quad
	switch (faceType) {
	case 0: case 2: case 5:
		inds.insert(inds.end(), { baseIndex + 2, baseIndex + 1, baseIndex + 0,
								  baseIndex + 2, baseIndex + 3, baseIndex + 1 });
		break;
	case 1: case 3: case 4:
		inds.insert(inds.end(), { baseIndex + 0, baseIndex + 1, baseIndex + 2,
								  baseIndex + 1, baseIndex + 3, baseIndex + 2 });
		break;
	}
}

glm::vec3 WorldManager::calculatePosition(MeshUtils::Quad& quad, int corner, size_t faceType, ChunkCoordPair cxcz, int sliceIndex, int levelOfDetail) {
	int blockResolution = 1 << levelOfDetail;

	int startX = cxcz.first * ChunkUtils::WIDTH;
	int startZ = cxcz.second * ChunkUtils::DEPTH;

	int u0 = quad.bounds.u0 * blockResolution;
    int v0 = quad.bounds.v0 * blockResolution;
	int u1 = quad.bounds.u1 * blockResolution;
	int v1 = quad.bounds.v1 * blockResolution;

	int sliceConverted = sliceIndex * blockResolution;

	int sliceXNeg = startX + sliceConverted;
    int sliceXPos = sliceXNeg + blockResolution;

	int sliceYNeg = sliceConverted;
    int sliceYPos = sliceYNeg + blockResolution;

	int sliceZNeg = startZ + sliceConverted;
    int sliceZPos = sliceZNeg + blockResolution;

	// x face variables
	int xy = u0;
	int xy1 = u1 + blockResolution;
	int xz = startZ + v0;
	int xz1 = startZ + v1 + blockResolution;

	// y face variables

	int yx = startX + v0;
	int yx1 = startX + v1 + blockResolution;
	int yz = startZ + u0;
	int yz1 = startZ + u1 + blockResolution;

	// z face variables
	int zx = startX + v0;
	int zx1 = startX + v1 + blockResolution;
	int zy = u0;
	int zy1 = u1 + blockResolution;

	std::unordered_map<int, int> sliceMap;

    sliceMap[0] = sliceXNeg;
    sliceMap[1] = sliceXPos;
    sliceMap[2] = sliceYNeg;
    sliceMap[3] = sliceYPos;
    sliceMap[4] = sliceZNeg;
    sliceMap[5] = sliceZPos;

	switch (faceType) {
    case 0:
    case 1:
		switch (corner) {
		case 0:
             return glm::vec3(sliceMap[faceType],	xy,						xz);
		case 1:
             return glm::vec3(sliceMap[faceType],	xy1,					xz);
		case 2:
             return glm::vec3(sliceMap[faceType],	xy1,					xz1);
		case 3:
             return glm::vec3(sliceMap[faceType],	xy,						xz1);
		}
	case 2:
    case 3:
		switch (corner) {
		case 0:
             return glm::vec3(yx,					sliceMap[faceType],		yz);
		case 1:
             return glm::vec3(yx,					sliceMap[faceType],		yz1);
		case 2:
             return glm::vec3(yx1,					sliceMap[faceType],		yz1);
		case 3:
             return glm::vec3(yx1,					sliceMap[faceType],		yz);
		}
	case 4:
    case 5:
		switch (corner) {
		case 0:
			return glm::vec3(zx,					zy,						sliceMap[faceType]);
		case 1:
			return glm::vec3(zx,					zy1,					sliceMap[faceType]);
		case 2:
			return glm::vec3(zx1,					zy1,					sliceMap[faceType]);
		case 3:
			return glm::vec3(zx1,					zy,						sliceMap[faceType]);
		}
	default:
		return glm::vec3(NULL,						NULL,					NULL);
	}
}

glm::vec2 WorldManager::calculateTexCoords(MeshUtils::Quad& quad, int corner) {
	switch (corner) {
	case 0:
		return glm::vec2(0.0f, 0.0f);
	case 1:
		return glm::vec2((quad.bounds.u1 - quad.bounds.u0 + 1), 0.0f);
	case 2:
        return glm::vec2((quad.bounds.u1 - quad.bounds.u0 + 1),  (quad.bounds.v1 - quad.bounds.v0 + 1));
	case 3:
		return glm::vec2(0.0f, (quad.bounds.v1 - quad.bounds.v0 + 1));
    default:
        break;
	}
}

void WorldManager::render() {
	renderer.updateShaderUniforms((glm::mat4&)camera->getView(), (glm::mat4&)camera->getProjection(), (glm::vec3&)camera->getCameraPos());
	renderer.render();
}

std::shared_ptr<const std::vector<BlockID>> WorldManager::tryGetChunkSnapshot(ChunkCoordPair key) {
	std::shared_lock<std::shared_mutex> lock(worldMapMtx, std::try_to_lock);
	if (!lock.owns_lock()) return {};
	auto it = worldMap.find(key);
	if (it == worldMap.end()) return {};
	return it->second->getSnapshot();
}