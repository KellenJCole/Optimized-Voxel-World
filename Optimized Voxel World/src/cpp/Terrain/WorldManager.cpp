#include "h/Terrain/WorldManager.h"
#include <vector>
#include <iostream>

WorldManager::WorldManager() :
	lightPos(0.0f, 2000.f, 0.0f),
	lightColor(0.9f, 0.8f, 0.7f),
	updatedRenderChunks(false)
{
	lastFrustumCheck = (float)glfwGetTime();

	stopAsync.store(false);
	chunkUpdate.store(false);
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
	lightPos = { (sin(glfwGetTime()) * 1000) + pos.x, 500, (cos(glfwGetTime()) * 1000) + pos.z };

	if (updatedRenderChunks) {
		updateRenderBuffers();
	}

	float now = (float)glfwGetTime();
	if (now - lastFrustumCheck >= 0.01) {
		lastFrustumCheck = now;
		std::vector<std::pair<int, int>> renderChunks = camera->getVisibleChunks(renderRadius);
		renderer.updateRenderChunks(renderChunks);
	}
}

void WorldManager::updateRenderBuffers() {
	std::lock_guard<std::mutex> guard(mtx);
	for (auto it = preparedChunks.begin(); it != preparedChunks.end(); /* no increment here */) {
		auto key = *it;
		renderer.updateVertexBuffer(verticesByChunk[key], key);
		renderer.updateIndexBuffer(indicesByChunk[key], key);
		it = preparedChunks.erase(it);
	}
	updatedRenderChunks = false;
}

void WorldManager::updateRenderChunks(int originX, int originZ, int renderRadius, bool unloadAll) {
	this->renderRadius = renderRadius;

	auto loadVector = chunkLoader.getLoadList(originX, originZ, renderRadius);

	if (loadFuture.valid()) {
		stopAsync.store(true);

		loadFuture.get();

		if (!unloadAll) {
			if (updatedRenderChunks) {
				updateRenderBuffers();
			}
		}

		stopAsync.store(false);

		unloadChunks(loadVector, unloadAll);
	}

	loadFuture = std::async(std::launch::async, [this, loadVector] {
		this->loadChunksAsync(loadVector);
		});
}

void WorldManager::loadChunksAsync(const std::vector<std::pair<int, int>>& loadChunks) {
	for (const auto& key : loadChunks) {
		if (stopAsync) break;
		{
			std::unique_lock<std::mutex> lk(chunkUpdateMtx);
			chunkCondition.wait(lk, [this] { return !chunkUpdate.load();  });

			auto result = worldMap.emplace(key, Chunk());
			if (result.second) { // Chunk was newly inserted
				auto& newChunk = result.first->second;
				newChunk.setChunkCoords(key.first, key.second);
				newChunk.setProcGenReference(proceduralAlgorithm);
				newChunk.generateChunk();
				newChunk.setWorldReference(this);
				genMeshForSingleChunk(key);
			}
			else {
				genMeshForSingleChunk(key);
			}
		}
	}
}


void WorldManager::markChunkReadyForRender(std::pair<int, int> key) {
	std::lock_guard<std::mutex> guard(mtx);
	preparedChunks.insert(key);
	updatedRenderChunks = true;
}

void WorldManager::genMeshForSingleChunk(ChunkCoordPair key) {
	if (verticesByChunk.find(key) == verticesByChunk.end() && indicesByChunk.find(key) == indicesByChunk.end()) {
		worldMap[key].setWholeChunkMeshes();
		for (int i = 0; i < 6; i++) {
			auto pairVec = worldMap[key].getGreedyMeshByFaceType(i);
			for (const auto& meshData : pairVec) {
				for (const auto& quad : meshData.first) {
					addQuadVerticesAndIndices(quad, key, i, meshData.second);
				}
			}
		}
	}

	worldMap[key].unload();

	markChunkReadyForRender(key);
}

void WorldManager::unloadChunks(const std::vector<std::pair<int, int>>& loadChunks, bool all) { // all - unload ALL for regeneration or unload those out of render distance
	std::unordered_set<std::pair<int, int>, PairHash> loadedChunksSet(loadChunks.begin(), loadChunks.end());

	// Unload chunks
	if (!all) {
		std::set<ChunkCoordPair> keysToDelete;
		for (const auto& chunkKey : worldMap) {
			if (loadedChunksSet.find(chunkKey.first) == loadedChunksSet.end()) {
				keysToDelete.insert(chunkKey.first);
			}
		}

		for (const auto& key : keysToDelete) {
			verticesByChunk.erase(key);
			indicesByChunk.erase(key);
			renderer.eraseBuffers(key);
			worldMap.erase(key);
		}
	}
	else {
		for (const auto& key : worldMap) {
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

int WorldManager::getBlockAtGlobal(int worldX, int worldY, int worldZ, bool fromSelf) {
	std::lock_guard<std::mutex> guard(mtx);
	int chunkX = (worldX >> 4);
	int chunkZ = (worldZ >> 4);
	std::pair<int, int> key = { chunkX, chunkZ };
	auto it = worldMap.find(key);
	if (it != worldMap.end()) {
		return it->second.getBlockAt(worldX, worldY, worldZ);
	}
	else {
		if (fromSelf) {
			worldMap.emplace(key, Chunk());
			worldMap[key].setChunkCoords(key.first, key.second);
			worldMap[key].setProcGenReference(proceduralAlgorithm);
			worldMap[key].generateChunk();
			worldMap[key].setWorldReference(this);
			return static_cast<int>(worldMap[key].getBlockAt(worldX, worldY, worldZ));
		}
	}

	return 69;
}

void WorldManager::breakBlock(int worldX, int worldY, int worldZ) {
	int chunkX = (worldX >> 4);
	int chunkZ = (worldZ >> 4);
	std::pair<int, int> key = { chunkX, chunkZ };
	{
		chunkUpdate.store(true);
		std::lock_guard<std::mutex> lk(chunkUpdateMtx);
		auto it = worldMap.find(key);
		if (it != worldMap.end()) {
			worldMap[key].breakBlock(((worldX % 16) + 16) % 16, worldY, ((worldZ % 16) + 16) % 16);
			updateMesh(key);
			if ((((worldX % 16) + 16) % 16) == 0) {
				updateMesh({ key.first - 1, key.second });
			}
			if ((((worldX % 16) + 16) % 16) == 15) {
				updateMesh({ key.first + 1, key.second });
			}
			if ((((worldZ % 16) + 16) % 16) == 0) {
				updateMesh({ key.first, key.second - 1 });
			}
			if ((((worldZ % 16) + 16) % 16) == 15) {
				updateMesh({ key.first, key.second + 1 });
			}
		}
	}
	chunkCondition.notify_one();
}

void WorldManager::placeBlock(int worldX, int worldY, int worldZ, unsigned char blockToPlace) {
	int chunkX = (worldX >> 4);
	int chunkZ = (worldZ >> 4);
	std::pair<int, int> key = { chunkX, chunkZ };
	{
		chunkUpdate.store(true);
		std::lock_guard<std::mutex> lk(chunkUpdateMtx);
		auto it = worldMap.find(key);
		if (it != worldMap.end()) {
			worldMap[key].placeBlock(((worldX % 16) + 16) % 16, worldY, ((worldZ % 16) + 16) % 16, blockToPlace);
			updateMesh(key);
			if ((((worldX % 16) + 16) % 16) == 0) {
				updateMesh({ key.first - 1, key.second });
			}
			if ((((worldX % 16) + 16) % 16) == 15) {
				updateMesh({ key.first + 1, key.second });
			}
			if ((((worldZ % 16) + 16) % 16) == 0) {
				updateMesh({ key.first, key.second - 1 });
			}
			if ((((worldZ % 16) + 16) % 16) == 15) {
				updateMesh({ key.first, key.second + 1 });
			}
		}
	}
	chunkCondition.notify_one();
}

void WorldManager::updateMesh(ChunkCoordPair key) {
	worldMap[key].setWholeChunkMeshes();
	verticesByChunk.erase(key);
	indicesByChunk.erase(key);
	for (int i = 0; i < 6; i++) {
		auto pairVec = worldMap[key].getGreedyMeshByFaceType(i);
		for (const auto& meshData : pairVec) {
			for (const auto& quad : meshData.first) {
				addQuadVerticesAndIndices(quad, key, i, meshData.second);
			}
		}
	}

	worldMap[key].unload();

	markChunkReadyForRender(key);

	chunkUpdate.store(false);
}

void WorldManager::addQuadVerticesAndIndices(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>> quad, ChunkCoordPair chunkCoords, int faceType, int offset) {
	// Calculate positions for each corner of the quad
	glm::vec3 bottomLeft = calculatePosition(quad, 0, faceType, chunkCoords, offset);
	glm::vec3 bottomRight = calculatePosition(quad, 1, faceType, chunkCoords, offset);
	glm::vec3 topLeft = calculatePosition(quad, 2, faceType, chunkCoords, offset);
	glm::vec3 topRight = calculatePosition(quad, 3, faceType, chunkCoords, offset);

	// Calculate texture coordinates for each corner
	glm::vec2 texCoordsBL = calculateTexCoords(quad, 0);
	glm::vec2 texCoordsBR = calculateTexCoords(quad, 1);
	glm::vec2 texCoordsTL = calculateTexCoords(quad, 2);
	glm::vec2 texCoordsTR = calculateTexCoords(quad, 3);

	// Define normal based on face type
	int normal; // Implement this based on faceType
	switch (faceType) {
	case 0: normal = 1; break;
	case 1: normal = 2; break;
	case 2: normal = 3; break;
	case 3: normal = 4; break;
	case 4: normal = 5; break;
	case 5: normal = 6; break;
	}

	// Add vertices for the quad
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

glm::vec3 WorldManager::calculatePosition(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>& q, int corner, int faceType, ChunkCoordPair cxcz, int offset) {
	switch (faceType) {
	case 0:
		switch (corner) {
		case 0:
			return glm::vec3(cxcz.first * 16 + offset, q.second.first.first, q.second.second.first + cxcz.second * 16);
		case 1:
			return glm::vec3(cxcz.first * 16 + offset, q.second.first.second + 1, q.second.second.first + cxcz.second * 16);
		case 2:
			return glm::vec3(cxcz.first * 16 + offset, q.second.first.second + 1, q.second.second.second + 1 + cxcz.second * 16);
		case 3:
			return glm::vec3(cxcz.first * 16 + offset, q.second.first.first, q.second.second.second + 1 + cxcz.second * 16);
		}
	case 1:
		switch (corner) {
		case 0:
			return glm::vec3(cxcz.first * 16 + offset + 1, q.second.first.first, q.second.second.first + cxcz.second * 16);
		case 1:
			return glm::vec3(cxcz.first * 16 + offset + 1, q.second.first.second + 1, q.second.second.first + cxcz.second * 16);
		case 2:
			return glm::vec3(cxcz.first * 16 + offset + 1, q.second.first.second + 1, q.second.second.second + 1 + cxcz.second * 16);
		case 3:
			return glm::vec3(cxcz.first * 16 + offset + 1, q.second.first.first, q.second.second.second + 1 + cxcz.second * 16);
		}
	case 2:
		switch (corner) {
		case 0:
			return glm::vec3(cxcz.first * 16 + q.second.second.first, offset, cxcz.second * 16 + q.second.first.first);
		case 1:
			return glm::vec3(cxcz.first * 16 + q.second.second.first, offset, cxcz.second * 16 + q.second.first.second + 1);
		case 2:
			return glm::vec3(cxcz.first * 16 + q.second.second.second + 1, offset, cxcz.second * 16 + q.second.first.second + 1);
		case 3:
			return glm::vec3(cxcz.first * 16 + q.second.second.second + 1, offset, cxcz.second * 16 + q.second.first.first);
		}
	case 3:
		switch (corner) {
		case 0:
			return glm::vec3(cxcz.first * 16 + q.second.second.first, offset + 1, cxcz.second * 16 + q.second.first.first);
		case 1:
			return glm::vec3(cxcz.first * 16 + q.second.second.first, offset + 1, cxcz.second * 16 + q.second.first.second + 1);
		case 2:
			return glm::vec3(cxcz.first * 16 + q.second.second.second + 1, offset + 1, cxcz.second * 16 + q.second.first.second + 1);
		case 3:
			return glm::vec3(cxcz.first * 16 + q.second.second.second + 1, offset + 1, cxcz.second * 16 + q.second.first.first);
		}
	case 4:
		switch (corner) {
		case 0:
			return glm::vec3(cxcz.first * 16 + q.second.second.first, q.second.first.first, offset + cxcz.second * 16);
		case 1:
			return glm::vec3(cxcz.first * 16 + q.second.second.first, q.second.first.second + 1, offset + cxcz.second * 16);
		case 2:
			return glm::vec3(cxcz.first * 16 + q.second.second.second + 1, q.second.first.second + 1, offset + cxcz.second * 16);
		case 3:
			return glm::vec3(cxcz.first * 16 + q.second.second.second + 1, q.second.first.first, offset + cxcz.second * 16);
		}
	case 5:
		switch (corner) {
		case 0:
			return glm::vec3(cxcz.first * 16 + q.second.second.first, q.second.first.first, offset + 1 + cxcz.second * 16);
		case 1:
			return glm::vec3(cxcz.first * 16 + q.second.second.first, q.second.first.second + 1, offset + 1 + cxcz.second * 16);
		case 2:
			return glm::vec3(cxcz.first * 16 + q.second.second.second + 1, q.second.first.second + 1, offset + 1 + cxcz.second * 16);
		case 3:
			return glm::vec3(cxcz.first * 16 + q.second.second.second + 1, q.second.first.first, offset + 1 + cxcz.second * 16);
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
		return glm::vec2(q.second.first.second - q.second.first.first + 1, 0.0f);
	case 2:
		return glm::vec2(q.second.first.second - q.second.first.first + 1, q.second.second.second - q.second.second.first + 1);
	case 3:
		return glm::vec2(0.0f, q.second.second.second - q.second.second.first + 1);
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