#pragma once
#include <vector>
#include <set>
#include <unordered_set>
#include <algorithm>

struct Hash {
	size_t operator()(const std::pair<int, int>& coord) const {
		return std::hash<int>()(coord.first) ^ std::hash<int>()(coord.second);
	}
};

class ChunkLoader {
public:
	ChunkLoader();
	std::pair<std::vector<std::pair<int, int>>, std::vector<std::pair<int, int>>> getUnloadAndLoadList(int worldX, int worldZ, int renderRadius);
private:
	std::unordered_set<std::pair<int, int>, Hash> loadedChunks;
};