#include "h/Terrain/ChunkLoader.h"
#include <algorithm>
#include <iostream>

ChunkLoader::ChunkLoader() {

}

std::pair<std::vector<std::pair<int, int>>, std::vector<std::pair<int, int>>> ChunkLoader::getUnloadAndLoadList(int chunkX, int chunkZ, int renderRadius) {
	std::unordered_set<std::pair<int, int>, Hash> currentChunksInView;
	std::vector <std::pair<int, int>> unloadList, loadList, surroundList;

	for (int x = chunkX - renderRadius; x <= chunkX + renderRadius; x++) {
		for (int z = chunkZ - renderRadius; z <= chunkZ + renderRadius; z++) {
			if ((x - chunkX) * (x - chunkX) + (z - chunkZ) * (z - chunkZ) <= renderRadius * renderRadius) {
				std::pair<int, int> currentChunk = { x, z };
				currentChunksInView.insert(currentChunk);
			}
		}
	}

	// Determine which loaded chunks are no longer in view to unload
	for (const auto& chunk : loadedChunks) {
		if (currentChunksInView.find(chunk) == currentChunksInView.end()) {
			unloadList.push_back(chunk);
		}
	}

	// Determine which chunks are in view but not currently loaded to load
	for (const auto& chunk : currentChunksInView) {
		if (loadedChunks.find(chunk) == loadedChunks.end()) {
			loadList.push_back(chunk);
		}
	}

	// Update loadedChunks based on unloadList and loadList
	for (const auto& chunk : unloadList) {
		loadedChunks.erase(chunk);
	}
	for (const auto& chunk : loadList) {
		loadedChunks.insert(chunk);
	}

	std::cout << "Rendered " << loadList.size() << " chunks.\n";
	return { unloadList, loadList };

}