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
	try {
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
	catch (const std::system_error& e) {
		std::cerr << "System error in worldManager::update(): " << e.what() << "\n";
	}
	catch (const std::exception& e) {
		std::cerr << "Exception in worldManager::update(): " << e.what() << "\n";
	}
	catch (...) {
		std::cerr << "Unknown error in worldManager::update().\n";
	}
}

void WorldManager::updateRenderBuffers() {
	try {
		std::lock(preparedChunksMtx, renderBuffersMtx);
		std::lock_guard<std::mutex> prepLock(preparedChunksMtx, std::adopt_lock);
		std::lock_guard<std::recursive_mutex> lock(renderBuffersMtx, std::adopt_lock);
		for (auto it = preparedChunks.begin(); it != preparedChunks.end(); /* no increment here */) {
			auto key = *it;
			renderer.updateVertexBuffer(verticesByChunk[key], key);
			renderer.updateIndexBuffer(indicesByChunk[key], key);
			it = preparedChunks.erase(it);
		}
		updatedRenderChunks = false;
	}
	catch (const std::system_error& e) {
		std::cerr << "System error in worldManager::updateRenderBuffers(): " << e.what() << "\n";
	}
	catch (const std::exception& e) {
		std::cerr << "Exception in worldManager::updateRenderBuffers(): " << e.what() << "\n";
	}
	catch (...) {
		std::cerr << "Unknown error in worldManager::updateRenderBuffers().\n";
	}
}

void WorldManager::updateRenderChunks(int originX, int originZ, int renderRadius, bool unloadAll) {
	try {
		this->renderRadius = renderRadius;

		auto loadVector = chunkLoader.getLoadList(originX, originZ, renderRadius);

		if (loadFuture.valid()) {
			stopAsync.store(true);  // Signal to stop the async task
			loadFuture.get();  // Wait for the async task to finish

			stopAsync.store(false);  // Reset the stop flag
		}

		if (unloadAll) {
			unloadChunks(loadVector, unloadAll);  // Unload before starting a new task
		}

		// Start new async task for loading chunks
		loadFuture = std::async(std::launch::async, [this, loadVector] {
			this->loadChunksAsync(loadVector);
			});
	}
	catch (const std::system_error& e) {
		std::cerr << "System error in worldManager::updateRenderChunks(): " << e.what() << "\n";
	}
	catch (const std::exception& e) {
		std::cerr << "Exception in worldManager::updateRenderChunks(): " << e.what() << "\n";
	}
	catch (...) {
		std::cerr << "Unknown error in worldManager::updateRenderChunks().\n";
	}
} 

void WorldManager::loadChunksAsync(const std::vector<std::pair<int, int>>& loadChunks) {
	for (const auto& key : loadChunks) {
		if (stopAsync.load()) {
			break;
		}

		{
			std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
			if (worldMap.find(key) != worldMap.end()) {
				genMeshForSingleChunk(key);

				// Notify that the chunk is ready
				markChunkReadyForRender(key);
				continue;  // Skip to the next chunk
			}

			auto newChunk = std::make_unique<Chunk>();
			newChunk->setChunkCoords(key.first, key.second);
			newChunk->setProcGenReference(proceduralAlgorithm);
			newChunk->setWorldReference(this);
			newChunk->generateChunk(calculateLevelOfDetail(key));
				
			worldMap[key] = std::move(newChunk); // Insert without a second lock
		}

		genMeshForSingleChunk(key);

		// Notify that the chunk is ready
		markChunkReadyForRender(key);
	}
}

int WorldManager::calculateLevelOfDetail(ChunkCoordPair ccp) {
	glm::vec3 cameraPos = camera->getCameraPos();
	ChunkCoordPair cameraKey = { 
		cameraPos.x >= 0 ? cameraPos.x / 64 : (cameraPos.x - 63) / 64, 
		cameraPos.z >= 0 ? cameraPos.z / 64 : (cameraPos.z - 63) / 64 
	};
	
	float distance = (float)(sqrt(pow(abs(ccp.first - cameraKey.first), 2) + pow(abs(ccp.second - cameraKey.second), 2)));

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
	else if (distance <= 50) {
		return 4;
	}
	else if (distance <= 60) {
		return 5;
	}
	else {
		return 6;
	}
}

void WorldManager::markChunkReadyForRender(std::pair<int, int> key) {
	std::lock_guard<std::mutex> lock(preparedChunksMtx);
	if (preparedChunks.find(key) == preparedChunks.end()) {
		preparedChunks.insert(key);
		updatedRenderChunks = true;
	}
}

void WorldManager::genMeshForSingleChunk(ChunkCoordPair key) {

	std::lock(worldMapMtx, chunkUpdateMtx);
	std::lock_guard<std::recursive_mutex> lock(chunkUpdateMtx, std::adopt_lock);
	std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx, std::adopt_lock);

	// Find the chunk using the iterator
	auto it = worldMap.find(key);
	if (it != worldMap.end()) {
		if (verticesByChunk.find(key) == verticesByChunk.end()) {
			it->second->setWholeChunkMeshes();
			for (int i = 0; i < 6; i++) {
				auto pairVec = it->second->getGreedyMeshByFaceType(i);
				for (const auto& meshData : pairVec) {
					for (const auto& quad : meshData.first) {
						addQuadVerticesAndIndices(quad, key, i, meshData.second, it->second->getLod());
					}
				}
			}
		}

		it->second->unload();

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

int WorldManager::convertWorldCoordToChunkCoord(int worldCoord) {
	return (worldCoord >= 0) ? (worldCoord / 64) : ((worldCoord - 63) / 64);
}

int WorldManager::getBlockAtGlobal(int worldX, int worldY, int worldZ, bool fromSelf, bool boundaryCheck, int prevLod, int face) {
	try {
		std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx);
		int chunkX = convertWorldCoordToChunkCoord(worldX);
		int chunkZ = convertWorldCoordToChunkCoord(worldZ);
		std::pair<int, int> key = { chunkX, chunkZ };

		auto it = worldMap.find(key);
		if (it != worldMap.end()) {
			int currentLod = worldMap[key]->getLod();

			if (boundaryCheck && prevLod != -1 && currentLod != prevLod) {
				int lodDifference = std::pow(2, std::abs(prevLod - currentLod));

				// Wrap world coordinates to local chunk coordinates and apply the chunk position
				int localX = (((worldX % 64) + 64) % 64);
				int localY = worldY;
				int localZ = (((worldZ % 64) + 64) % 64);

				// If currentLod is higher (e.g., LOD 0), scale the coordinates up (multiply)
				if (currentLod < prevLod) {
					//std::cout << "Higher\n";
					worldX = (chunkX * 64) + (localX * lodDifference);
					worldY *= lodDifference; // Assuming vertical scaling also follows LOD
					worldZ = (chunkZ * 64) + (localZ * lodDifference);
				}
				// If currentLod is lower (e.g., LOD 1), scale the coordinates down (divide)
				else if (currentLod > prevLod) {
					//std::cout << "Lower\n";
					worldX = (chunkX * 64) + (localX / lodDifference);
					worldY /= lodDifference; // Assuming vertical scaling also follows LOD
					worldZ = (chunkZ * 64) + (localZ / lodDifference);
				}

				return it->second->getBlockAt(worldX, worldY, worldZ, false, boundaryCheck, face, prevLod, false);
			}
			else {
				return it->second->getBlockAt(worldX, worldY, worldZ, false, boundaryCheck, face, prevLod, false);
			}
		}
		else {
			if (fromSelf) {
				// Insert a new chunk safely using std::make_unique
				auto newChunk = std::make_unique<Chunk>();
				newChunk->setChunkCoords(key.first, key.second);
				newChunk->setProcGenReference(proceduralAlgorithm);
				newChunk->setWorldReference(this);
				newChunk->generateChunk(calculateLevelOfDetail(key));

				// Attempt to insert the new chunk
				auto insertResult = worldMap.emplace(key, std::move(newChunk));
				if (insertResult.second) { // Newly inserted
					auto& insertedChunk = insertResult.first->second;
					return insertedChunk->getBlockAt(worldX, worldY, worldZ, false, boundaryCheck, -1, prevLod, false);
				}
				else { // Already exists (unlikely due to emplace)
					std::cout << "Chunk (" << key.first << ", " << key.second << ") already exists in getBlockAtGlobal.\n";
					return it->second->getBlockAt(worldX, worldY, worldZ, false, boundaryCheck, -1, prevLod, false);
				}
			}
		}

		std::cerr << "getBlockAtGlobal - Chunk not found and fromSelf is false: (" << key.first << ", " << key.second << ")\n";
		return 69; // Arbitrary error value
	}
	catch (const std::system_error& e) {
		std::cerr << "System error in getBlockAtGlobal: " << e.what() << "\n";
	}
	catch (const std::exception& e) {
		std::cerr << "Exception in getBlockAtGlobal: " << e.what() << "\n";
	}
	catch (...) {
		std::cerr << "Unknown error in getBlockAtGlobal.\n";
	}

	return 69; // Arbitrary error value if exception occurs
}

void WorldManager::breakBlock(int worldX, int worldY, int worldZ) {
	int chunkX = convertWorldCoordToChunkCoord(worldX);
	int chunkZ = convertWorldCoordToChunkCoord(worldZ);
	std::pair<int, int> key = { chunkX, chunkZ };
	{
		std::lock(worldMapMtx, chunkUpdateMtx);
		std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx, std::adopt_lock);
		std::lock_guard<std::recursive_mutex> chunkLock(chunkUpdateMtx, std::adopt_lock);
		chunkUpdate.store(true);
		auto it = worldMap.find(key);
		if (it != worldMap.end()) {
			// Calculate local coordinates
			int localX = (((worldX % 64) + 64) % 64);
			int localZ = (((worldZ % 64) + 64) % 64);
			it->second->breakBlock(localX, worldY, localZ);
			updateMesh(key);
			// Update neighboring chunks if necessary
			if (localX == 0) {
				updateMesh({ key.first - 1, key.second });
			}
			if (localX == 63) {
				updateMesh({ key.first + 1, key.second });
			}
			if (localZ == 0) {
				updateMesh({ key.first, key.second - 1 });
			}
			if (localZ == 63) {
				updateMesh({ key.first, key.second + 1 });
			}
		}
	}
	chunkCondition.notify_one();
}

void WorldManager::placeBlock(int worldX, int worldY, int worldZ, unsigned char blockToPlace) {
	// Corrected bit shift from >>5 to >>6 for 64-block chunks
	int chunkX = convertWorldCoordToChunkCoord(worldX);
	int chunkZ = convertWorldCoordToChunkCoord(worldZ);
	std::pair<int, int> key = { chunkX, chunkZ };
	{
		std::lock(worldMapMtx, chunkUpdateMtx);
		std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx, std::adopt_lock);
		std::lock_guard<std::recursive_mutex> chunkLock(chunkUpdateMtx, std::adopt_lock);
		chunkUpdate.store(true);
		auto it = worldMap.find(key);
		if (it != worldMap.end()) {
			// Calculate local coordinates
			int localX = (((worldX % 64) + 64) % 64);
			int localZ = (((worldZ % 64) + 64) % 64);
			it->second->placeBlock(localX, worldY, localZ, blockToPlace);
			updateMesh(key);
			// Update neighboring chunks if necessary
			if (localX == 0) {
				updateMesh({ key.first - 1, key.second });
			}
			if (localX == 63) {
				updateMesh({ key.first + 1, key.second });
			}
			if (localZ == 0) {
				updateMesh({ key.first, key.second - 1 });
			}
			if (localZ == 63) {
				updateMesh({ key.first, key.second + 1 });
			}
		}
	}
	chunkCondition.notify_one();
}

void WorldManager::updateMesh(ChunkCoordPair key) {
	std::lock(worldMapMtx, chunkUpdateMtx, renderBuffersMtx);
	std::lock_guard<std::recursive_mutex> worldLock(worldMapMtx, std::adopt_lock);
	std::lock_guard<std::recursive_mutex> chunkLock(chunkUpdateMtx, std::adopt_lock);
	std::lock_guard<std::recursive_mutex> renderLock(renderBuffersMtx, std::adopt_lock);
	auto it = worldMap.find(key);
	if (it != worldMap.end()) {
		it->second->setWholeChunkMeshes();
		{
			verticesByChunk.erase(key);
			indicesByChunk.erase(key);
		}
		for (int i = 0; i < 6; i++) {
			auto pairVec = it->second->getGreedyMeshByFaceType(i);
			for (const auto& meshData : pairVec) {
				for (const auto& quad : meshData.first) {
					addQuadVerticesAndIndices(quad, key, i, meshData.second, it->second->getLod());
				}
			}
		}

		it->second->unload();

		markChunkReadyForRender(key);
	}
	else {
		std::cerr << "updateMesh - Chunk not found or is null: (" << key.first << ", " << key.second << ")\n";
	}

	chunkUpdate.store(false);
}

void WorldManager::addQuadVerticesAndIndices(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>> quad, ChunkCoordPair chunkCoords, int faceType, int offset, int levelOfDetail) {
	std::lock_guard<std::recursive_mutex> renderLock(renderBuffersMtx);
	// Calculate positions for each corner of the quad
	glm::vec3 bottomLeft = calculatePosition(quad, 0, faceType, chunkCoords, offset, levelOfDetail);
	glm::vec3 bottomRight = calculatePosition(quad, 1, faceType, chunkCoords, offset, levelOfDetail);
	glm::vec3 topLeft = calculatePosition(quad, 2, faceType, chunkCoords, offset, levelOfDetail);
	glm::vec3 topRight = calculatePosition(quad, 3, faceType, chunkCoords, offset, levelOfDetail);

	// Calculate texture coordinates for each corner
	glm::vec2 texCoordsBL = calculateTexCoords(quad, 0, levelOfDetail);
	glm::vec2 texCoordsBR = calculateTexCoords(quad, 1, levelOfDetail);
	glm::vec2 texCoordsTL = calculateTexCoords(quad, 2, levelOfDetail);
	glm::vec2 texCoordsTR = calculateTexCoords(quad, 3, levelOfDetail);

	// Define normal based on face type
	int normal = faceType + 1;

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

glm::vec3 WorldManager::calculatePosition(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>& q, int corner, int faceType, ChunkCoordPair cxcz, int offset, int levelOfDetail) {
	int positionMult = pow(2, levelOfDetail);

	int startX = cxcz.first * 64;
	int startZ = cxcz.second * 64;
	int firstFirstConvert = q.second.first.first * positionMult;
	int secondFirstConvert = q.second.second.first * positionMult;
	int firstSecondConvert = q.second.first.second * positionMult;
	int secondSecondConvert = q.second.second.second * positionMult;
	int offsetConverted = offset * positionMult;

	int xOffset = startX + offsetConverted;
	int xOffset1 = xOffset + positionMult;

	int yOffset = offsetConverted;
	int yOffset1 = yOffset + positionMult;

	int zOffset = startZ + offsetConverted;
	int zOffset1 = zOffset + positionMult;

	// x face variables
	int xy = firstFirstConvert;
	int xy1 = firstSecondConvert + positionMult;
	int xz = startZ + secondFirstConvert;
	int xz1 = startZ + secondSecondConvert + positionMult;

	// y face variables

	int yx = startX + secondFirstConvert;
	int yx1 = startX + secondSecondConvert + positionMult;
	int yz = startZ + firstFirstConvert;
	int yz1 = startZ + firstSecondConvert + positionMult;

	// z face variables
	int zx = startX + secondFirstConvert;
	int zx1 = startX + secondSecondConvert + positionMult;
	int zy = firstFirstConvert;
	int zy1 = firstSecondConvert + positionMult;

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

glm::vec2 WorldManager::calculateTexCoords(std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>& q, int corner, int levelOfDetail) {
	int positionMult = pow(2, levelOfDetail);
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