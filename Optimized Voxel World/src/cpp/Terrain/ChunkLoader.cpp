#include "h/Terrain/ChunkLoader.h"
#include <algorithm>
#include <iostream>

ChunkLoader::ChunkLoader() {

}

std::vector<std::pair<int, int>> ChunkLoader::getLoadList(int chunkX, int chunkZ, int renderRadius) {
	std::vector<std::pair<int, int>> loadList;

	for (int x = chunkX - renderRadius; x <= chunkX + renderRadius; x++) {
		for (int z = chunkZ - renderRadius; z <= chunkZ + renderRadius; z++) {
			if ((x - chunkX) * (x - chunkX) + (z - chunkZ) * (z - chunkZ) <= renderRadius * renderRadius && x != chunkX - renderRadius && x != chunkX + renderRadius && z != chunkZ - renderRadius && z != chunkZ + renderRadius) {
				std::pair<int, int> currentChunk = { x, z };
				loadList.push_back(currentChunk);
			}
		}
	}

	std::sort(loadList.begin(), loadList.end(), [chunkX, chunkZ](const auto& left, const auto& right) {
		int dxLeft = chunkX - left.first;
		int dzLeft = chunkZ - left.second;
		int distSquaredLeft = dxLeft * dxLeft + dzLeft * dzLeft;

		int dxRight = chunkX - right.first;
		int dzRight = chunkZ - right.second;
		int distSquaredRight = dxRight * dxRight + dzRight * dzRight;

		// Compare squared distances
		return distSquaredLeft < distSquaredRight;
		});

	return loadList;

}