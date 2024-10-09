#include "h/Terrain/WorldManager.h"
#include <vector>
#include <iostream>

WorldManager::WorldManager() :
	lightPos(0.0f, 2000.f, 0.0f),
	lightColor(0.9f, 0.8f, 0.7f),
	updatedRenderChunks(false),
	renderRadius(40),
	readyForPlayerUpdate(false)
{
	lastFrustumCheck = glfwGetTime();
	stopAsync.store(false);
}

bool WorldManager::initialize(ProcGen* pg) {

	if (!renderer.initialize()) {
		std::cout << "Renderer failed to initialize in World Manager\n";
		return false;
	}

	blockTextureArray = TextureArray({ "blocks/dirt.png", "blocks/grass_top.png", "blocks/grass_side.png", "blocks/stone.png", "blocks/bedrock.png", "blocks/sand.png", "blocks/water.png"}, false);

	proceduralAlgorithm = pg;

	return true;
}

void WorldManager::update() {
	glm::vec3 pos = camera->getCameraPos();
	lightPos = { 0, 500, 0 };

	if (updatedRenderChunks) {
		updateRenderBuffers();
	}

	double now = glfwGetTime();
	if ((now - lastFrustumCheck) >= 0.01) {
		lastFrustumCheck = now;

		std::vector<std::pair<int, int>> renderChunks = camera->getVisibleChunks(renderRadius);
		renderer.updateRenderChunks(renderChunks);
	}
}

void WorldManager::updateRenderBuffers() {
	std::lock(meshKeysMtx, renderBuffersMtx);
	std::lock_guard<std::mutex> meshLock(meshKeysMtx, std::adopt_lock);
	std::lock_guard<std::recursive_mutex> lock(renderBuffersMtx, std::adopt_lock);
	for (auto it = meshKeysToUpdate.begin(); it != meshKeysToUpdate.end(); /* no increment here */) {
		auto key = *it;
		renderer.updateVertexBuffer(verticesByChunk[key], key);
		renderer.updateIndexBuffer(indicesByChunk[key], key);
		it = meshKeysToUpdate.erase(it);
	}
	updatedRenderChunks = false;
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

	// Start new async task for loading chunks
	loadFuture = std::async(std::launch::async, [this, loadVector] {
		this->loadChunksAsync(loadVector);
	});
} 

void WorldManager::loadChunksAsync(const std::vector<std::pair<int, int>>& loadChunks) {
	for (const auto& key : loadChunks) {
		if (stopAsync.load()) {
			break;
		}

		int lod = calculateLevelOfDetail(key);
		if (worldKeysSet.find(key) == worldKeysSet.end()) {
			auto newChunk = std::make_unique<Chunk>();
			newChunk->setChunkCoords(key.first, key.second);
			newChunk->setProcGenReference(proceduralAlgorithm);
			newChunk->setWorldReference(this);
			newChunk->setLod(lod);
			newChunk->generateChunk();

			readyForPlayerUpdate = false;
			{
				std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
				worldMap[key] = std::move(newChunk);
			}
			readyForPlayerUpdate = true;
			worldKeysSet.insert({ key.first, key.second });
		}
	}

	for (const auto& key : loadChunks) {
		if (stopAsync.load()) {
			break;
		}

		int lod = calculateLevelOfDetail(key);
		if (worldKeysSet.find(key) != worldKeysSet.end()) {
			std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
			if (worldMap[key]->getCurrentLod() != lod) {
				worldMap[key]->convertLOD(lod);
				updateMesh(key);
			}
			else {
				genMeshForSingleChunk(key);
				markChunkReadyForRender(key);
				continue;
			}
		}
	}
}

int WorldManager::calculateLevelOfDetail(ChunkCoordPair ccp) {
	glm::vec3 cameraPos = camera->getCameraPos();
	ChunkCoordPair cameraKey = { 
		ChunkUtils::convertWorldCoordToChunkCoord(static_cast<int>(floor(cameraPos.x))),
		ChunkUtils::convertWorldCoordToChunkCoord(static_cast<int>(floor(cameraPos.z)))
	};
	
	float distance = (float)(sqrt(pow(abs(ccp.first - cameraKey.first), 2) + pow(abs(ccp.second - cameraKey.second), 2)));

	// These values are random af, need to put more thought into it
	if (distance <= 11) {
		return 0;
	}
	else if (distance <= 20) {
		return 1;
	}
	else if (distance <= 30) {
		return 2;
	}
	else if (distance <= 40) {
		return 3;
	}
	else if (distance <= 100) {
		return 4;
	}
	else if (distance <= 200) {
		return 5;
	}
	else {
		return 6;
	}
}

void WorldManager::markChunkReadyForRender(std::pair<int, int> key) {
	std::lock_guard<std::mutex> meshLock(meshKeysMtx);
	if (meshKeysToUpdate.find(key) == meshKeysToUpdate.end()) {
		meshKeysToUpdate.insert(key);
		updatedRenderChunks = true;
	}
}

void WorldManager::genMeshForSingleChunk(ChunkCoordPair key) {
	int lod;
	// Find the chunk using the iterator
	std::vector<std::vector<std::pair<std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>>, int>>> greedyMeshes;
	if (worldKeysSet.find(key) != worldKeysSet.end()) {
		if (verticesByChunk.find(key) == verticesByChunk.end()) {
			{
				std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
				lod = worldMap[key]->getCurrentLod();
				worldMap[key]->generateChunkMeshes();
				for (int i = 0; i < 6; i++) {
					greedyMeshes.push_back(worldMap[key]->getGreedyMeshByFaceType(i));
				
				}
			}

			for (int i = 0; i < 6; i++) {
				for (const auto& meshData : greedyMeshes[i]) {
					for (const auto& quad : meshData.first) {
						addQuadVerticesAndIndices(quad, key, i, meshData.second, lod);
					}
				}
			}
		}

		{
			std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
			worldMap[key]->unload();
		}

		markChunkReadyForRender(key);
	}
	else {
		std::cerr << "genMeshForSingleChunk - Chunk not found or is null: (" << key.first << ", " << key.second << ")\n";
	}
}

void WorldManager::unloadChunks(const std::vector<std::pair<int, int>>& loadChunks, bool all) { // all - unload ALL for regeneration or unload those out of render distance
	std::unordered_set<std::pair<int, int>, PairHash> loadedChunksSet(loadChunks.begin(), loadChunks.end());

	// Unload chunks
	if (!all) {
		std::set<ChunkCoordPair> keysToDelete;

		std::lock(worldMapMtx, renderBuffersMtx);
		std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx, std::adopt_lock);
		std::lock_guard<std::recursive_mutex> renderLock(renderBuffersMtx, std::adopt_lock);

		for (const auto& chunkKey : worldMap) {
			if (loadedChunksSet.find(chunkKey.first) == loadedChunksSet.end()) {
				keysToDelete.insert(chunkKey.first);
			}
		}

		for (const auto& key : keysToDelete) {
			worldKeysSet.erase(key);
			verticesByChunk.erase(key);
			indicesByChunk.erase(key);
			renderer.eraseBuffers(key);
			worldMap.erase(key);
		}
	}
	else {
		std::lock(worldMapMtx, renderBuffersMtx);
		std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx, std::adopt_lock);
		std::lock_guard<std::recursive_mutex> renderLock(renderBuffersMtx, std::adopt_lock);

		for (const auto& key : worldMap) {
			worldKeysSet.erase(key.first);
			verticesByChunk.erase(key.first);
			indicesByChunk.erase(key.first);
			renderer.eraseBuffers(key.first);
		}
		worldMap.clear();
	}
}

void WorldManager::setCamAndShaderPointers(Shader* sha, Camera* cam) {
	shader = sha;
	camera = cam;
}

void WorldManager::cleanup() {
	renderer.cleanup();
}

unsigned char WorldManager::getBlockAtGlobal(int worldX, int worldY, int worldZ, int prevLod, int face) {
	int chunkX = ChunkUtils::convertWorldCoordToChunkCoord(worldX);
	int chunkZ = ChunkUtils::convertWorldCoordToChunkCoord(worldZ);
	std::pair<int, int> key = { chunkX, chunkZ };

	if (worldKeysSet.find(key) != worldKeysSet.end()) {
		int currentLod;
		{
			std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
			currentLod = worldMap[key]->getCurrentLod();
		}
		if (prevLod != -1 && currentLod != prevLod) {
			int lodDifference = 1 << abs(prevLod - currentLod);

			// Wrap world coordinates to local chunk coordinates and apply the chunk position
			int localX = ChunkUtils::convertWorldCoordToLocalCoord(worldX);
			int localY = worldY;
			int localZ = ChunkUtils::convertWorldCoordToLocalCoord(worldZ);

			// If currentLod is higher (e.g., LOD 0), scale the coordinates up (multiply)
			if (currentLod < prevLod) {
				worldX = (chunkX * ChunkUtils::WIDTH) + (localX * lodDifference);
				worldY *= lodDifference;
				worldZ = (chunkZ * ChunkUtils::DEPTH) + (localZ * lodDifference);
			}
			// If currentLod is lower (e.g., LOD 1), scale the coordinates down (divide)
			else if (currentLod > prevLod) {
				worldX = (chunkX * ChunkUtils::WIDTH) + (localX / lodDifference);
				worldY /= lodDifference;
				worldZ = (chunkZ * ChunkUtils::DEPTH) + (localZ / lodDifference);
			}

			std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
			return worldMap[key]->getBlockAt(worldX, worldY, worldZ, face, prevLod);
		}
		else {
			std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
			return worldMap[key]->getBlockAt(worldX, worldY, worldZ, face, prevLod);
		}
	}
	else {
		return 69; // Arbitrary non-existent chunk block
	}
}

void WorldManager::breakBlock(int worldX, int worldY, int worldZ) {
	int chunkX = ChunkUtils::convertWorldCoordToChunkCoord(worldX);
	int chunkZ = ChunkUtils::convertWorldCoordToChunkCoord(worldZ);
	std::pair<int, int> key = { chunkX, chunkZ };
	{

		if (worldKeysSet.find(key) != worldKeysSet.end()) {
			int localX = ChunkUtils::convertWorldCoordToLocalCoord(worldX);
			int localZ = ChunkUtils::convertWorldCoordToLocalCoord(worldZ);
			{
				std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
				worldMap[key]->breakBlock(localX, worldY, localZ);
			}
			updateMesh(key);
			// Update neighboring chunks if necessary
			if (localX == 0) {
				updateMesh({ key.first - 1, key.second });
			}
			else if (localX == ChunkUtils::WIDTH - 1) {
				updateMesh({ key.first + 1, key.second });
			}
			if (localZ == 0) {
				updateMesh({ key.first, key.second - 1 });
			}
			else if (localZ == ChunkUtils::DEPTH - 1) {
				updateMesh({ key.first, key.second + 1 });
			}
		}
	}
}

void WorldManager::placeBlock(int worldX, int worldY, int worldZ, unsigned char blockToPlace) {
	int chunkX = ChunkUtils::convertWorldCoordToChunkCoord(worldX);
	int chunkZ = ChunkUtils::convertWorldCoordToChunkCoord(worldZ);
	std::pair<int, int> key = { chunkX, chunkZ };
	{
		if (worldKeysSet.find(key) != worldKeysSet.end()) {
			// Calculate local coordinates
			int localX = ChunkUtils::convertWorldCoordToLocalCoord(worldX);
			int localZ = ChunkUtils::convertWorldCoordToLocalCoord(worldZ);
			{
				std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
				worldMap[key]->placeBlock(localX, worldY, localZ, blockToPlace);
			}
			updateMesh(key);
			// Update neighboring chunks if necessary
			if (localX == 0) {
				updateMesh({ key.first - 1, key.second });
			}
			if (localX == ChunkUtils::WIDTH - 1) {
				updateMesh({ key.first + 1, key.second });
			}
			if (localZ == 0) {
				updateMesh({ key.first, key.second - 1 });
			}
			if (localZ == ChunkUtils::DEPTH - 1) {
				updateMesh({ key.first, key.second + 1 });
			}
		}
	}
}

void WorldManager::updateMesh(ChunkCoordPair key) {
	int lod;
	if (worldKeysSet.find(key) != worldKeysSet.end()) {
		{
			std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
			lod = worldMap[key]->getCurrentLod();
			worldMap[key]->generateChunkMeshes();
		}
		{
			std::lock_guard<std::recursive_mutex> renderLock(renderBuffersMtx);
			verticesByChunk.erase(key);
			indicesByChunk.erase(key);
		}
		std::vector<std::vector<std::pair<std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>>, int>>> greedyMeshes;
		{
			std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
			for (int i = 0; i < 6; i++) {
				greedyMeshes.push_back(worldMap[key]->getGreedyMeshByFaceType(i));

			}
		}

		for (int i = 0; i < 6; i++) {
			for (const auto& meshData : greedyMeshes[i]) {
				for (const auto& quad : meshData.first) {
					addQuadVerticesAndIndices(quad, key, i, meshData.second, lod);
				}
			}
		}

		{
			std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
			worldMap[key]->unload();
		}

		markChunkReadyForRender(key);
	}
	else {
		std::cerr << "updateMesh - Chunk not found or is null: (" << key.first << ", " << key.second << ")\n";
	}
}

void WorldManager::addQuadVerticesAndIndices(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>> quad, ChunkCoordPair chunkCoords, int faceType, int offset, int levelOfDetail) {
	// Calculate positions for each corner of the quad
	glm::vec3 bottomLeft = calculatePosition(quad, 0, faceType, chunkCoords, offset, levelOfDetail);
	glm::vec3 bottomRight = calculatePosition(quad, 1, faceType, chunkCoords, offset, levelOfDetail);
	glm::vec3 topLeft = calculatePosition(quad, 2, faceType, chunkCoords, offset, levelOfDetail);
	glm::vec3 topRight = calculatePosition(quad, 3, faceType, chunkCoords, offset, levelOfDetail);

	// Calculate texture coordinates for each corner
	glm::vec2 texCoordsBL = calculateTexCoords(quad, 0);
	glm::vec2 texCoordsBR = calculateTexCoords(quad, 1);
	glm::vec2 texCoordsTL = calculateTexCoords(quad, 2);
	glm::vec2 texCoordsTR = calculateTexCoords(quad, 3);

	// Define normal based on face type
	int normal = faceType + 1;

	// Add vertices for the quad
	std::lock_guard<std::recursive_mutex> renderLock(renderBuffersMtx);
	unsigned int indexStart = verticesByChunk[chunkCoords].size();
	verticesByChunk[chunkCoords].push_back(Vertex{ bottomLeft, texCoordsBL, normal + (int)quad.first * 16 });
	verticesByChunk[chunkCoords].push_back(Vertex{ bottomRight, texCoordsBR, normal + (int)quad.first * 16 });
	verticesByChunk[chunkCoords].push_back(Vertex{ topRight, texCoordsTR, normal + (int)quad.first * 16 });
	verticesByChunk[chunkCoords].push_back(Vertex{ topLeft, texCoordsTL, normal + (int)quad.first * 16 });

	// Add indices for two triangles making up the quad
	switch (faceType) {
	case 0:
	case 2:
	case 5:
		indicesByChunk[chunkCoords].push_back(indexStart + 2);
		indicesByChunk[chunkCoords].push_back(indexStart + 1);
		indicesByChunk[chunkCoords].push_back(indexStart);

		indicesByChunk[chunkCoords].push_back(indexStart + 2);
		indicesByChunk[chunkCoords].push_back(indexStart + 3);
		indicesByChunk[chunkCoords].push_back(indexStart + 1);
		break;
	case 1:
	case 3:
	case 4:
	indicesByChunk[chunkCoords].push_back(indexStart);     
    indicesByChunk[chunkCoords].push_back(indexStart + 1); 
    indicesByChunk[chunkCoords].push_back(indexStart + 2); 

    indicesByChunk[chunkCoords].push_back(indexStart + 1);     
	indicesByChunk[chunkCoords].push_back(indexStart + 3); 
    indicesByChunk[chunkCoords].push_back(indexStart + 2); 
    break;
	}
}

glm::vec3 WorldManager::calculatePosition(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>& q, int corner, int faceType, ChunkCoordPair cxcz, int offset, int levelOfDetail) {
	int blockResolution = 1 << levelOfDetail;

	int startX = cxcz.first * ChunkUtils::WIDTH;
	int startZ = cxcz.second * ChunkUtils::DEPTH;
	int firstFirstConvert = q.second.first.first * blockResolution;
	int secondFirstConvert = q.second.second.first * blockResolution;
	int firstSecondConvert = q.second.first.second * blockResolution;
	int secondSecondConvert = q.second.second.second * blockResolution;
	int offsetConverted = offset * blockResolution;

	int xOffset = startX + offsetConverted;
	int xOffset1 = xOffset + blockResolution;

	int yOffset = offsetConverted;
	int yOffset1 = yOffset + blockResolution;

	int zOffset = startZ + offsetConverted;
	int zOffset1 = zOffset + blockResolution;

	// x face variables
	int xy = firstFirstConvert;
	int xy1 = firstSecondConvert + blockResolution;
	int xz = startZ + secondFirstConvert;
	int xz1 = startZ + secondSecondConvert + blockResolution;

	// y face variables

	int yx = startX + secondFirstConvert;
	int yx1 = startX + secondSecondConvert + blockResolution;
	int yz = startZ + firstFirstConvert;
	int yz1 = startZ + firstSecondConvert + blockResolution;

	// z face variables
	int zx = startX + secondFirstConvert;
	int zx1 = startX + secondSecondConvert + blockResolution;
	int zy = firstFirstConvert;
	int zy1 = firstSecondConvert + blockResolution;

	switch (faceType) {
	case 0:
		switch (corner) {
		case 0:
			return glm::vec3(xOffset,		xy,				xz);
		case 1:
			return glm::vec3(xOffset,		xy1,			xz);
		case 2:
			return glm::vec3(xOffset,		xy1,			xz1);
		case 3:
			return glm::vec3(xOffset,		xy,				xz1);
		}

	case 1:
		switch (corner) {
		case 0:
			return glm::vec3(xOffset1,		xy,				xz);
		case 1:
			return glm::vec3(xOffset1,		xy1,			xz);
		case 2:
			return glm::vec3(xOffset1,		xy1,			xz1);
		case 3:
			return glm::vec3(xOffset1,		xy,				xz1);
		}
	case 2:
		switch (corner) {
		case 0:
			return glm::vec3(yx,			yOffset,		yz);
		case 1:
			return glm::vec3(yx,			yOffset,		yz1);
		case 2:
			return glm::vec3(yx1,			yOffset,		yz1);
		case 3:
			return glm::vec3(yx1,			yOffset,		yz);
		}
	case 3:
		switch (corner) {
		case 0:
			return glm::vec3(yx,			yOffset1,		yz);
		case 1:
			return glm::vec3(yx,			yOffset1,		yz1);
		case 2:
			return glm::vec3(yx1,			yOffset1,		yz1);
		case 3:
			return glm::vec3(yx1,			yOffset1,		yz);
		}
	case 4:
		switch (corner) {
		case 0:
			return glm::vec3(zx,			zy,				zOffset);
		case 1:
			return glm::vec3(zx,			zy1,			zOffset);
		case 2:
			return glm::vec3(zx1,			zy1,			zOffset);
		case 3:
			return glm::vec3(zx1,			zy,				zOffset);
		}
	case 5:
		switch (corner) {
		case 0:
			return glm::vec3(zx,			zy,				zOffset1);
		case 1:
			return glm::vec3(zx,			zy1,			zOffset1);
		case 2:
			return glm::vec3(zx1,			zy1,			zOffset1);
		case 3:
			return glm::vec3(zx1,			zy,				zOffset1);
		}
	default:
		return glm::vec3(NULL, NULL, NULL);
	}
}

glm::vec2 WorldManager::calculateTexCoords(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>& q, int corner) {
	switch (corner) {
	case 0:
		return glm::vec2(0.0f, 0.0f);
	case 1:
		return glm::vec2((q.second.first.second - q.second.first.first + 1), 0.0f);
	case 2:
		return glm::vec2((q.second.first.second - q.second.first.first + 1), (q.second.second.second - q.second.second.first + 1));
	case 3:
		return glm::vec2(0.0f, (q.second.second.second - q.second.second.first + 1));
	}
}

void WorldManager::render() {

	blockTextureArray.Bind();

	shader->setUniform4fv("view", (glm::mat4&)camera->getView());
	shader->setUniform4fv("projection", (glm::mat4&)camera->getProjection());

	shader->setVec3("lightPos", lightPos);
	shader->setVec3("lightColor", lightColor);
	shader->setVec3("viewPos", (glm::vec3&)camera->getCameraPos());

	renderer.render();
}

void WorldManager::switchRenderMethod() {
	renderer.toggleFillLine();
}

void WorldManager::passWindowPointerToRenderer(GLFWwindow* window) {
	renderer.setWindowPointer(window);
}

std::vector<unsigned char> WorldManager::getChunkVector(ChunkCoordPair key) {
	std::vector<unsigned char> defaultReturn;
	defaultReturn.resize(1); 

	std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);

	if (worldMap.find(key) != worldMap.end()) {
		std::vector<unsigned char> result = worldMap[key]->getCurrChunkVec();
		return result;
	}
	else {
		return defaultReturn;
	}
}

bool WorldManager::getReadyForPlayerUpdate() {
	return readyForPlayerUpdate;
}