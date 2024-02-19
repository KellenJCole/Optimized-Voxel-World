#pragma once
#include <vector>
#include <set>
#include <unordered_set>
#include <algorithm>

using ChunkCoord = std::pair<int, int>;
using ChunkCoordList = std::vector<ChunkCoord>;
struct Hash {
	size_t operator()(const ChunkCoord& coord) const {
		return std::hash<int>()(coord.first) ^ std::hash<int>()(coord.second);
	}
};
using ChunkSet = std::unordered_set<ChunkCoord, Hash>;
using ChunkCoordList = std::vector<ChunkCoord>;

class ChunkLoader {
public:
	ChunkLoader();
	
	using ChunkCoord = std::pair<int, int>;
	using ChunkCoordList = std::vector<ChunkCoord>;

	std::pair<ChunkCoordList, ChunkCoordList> getUnloadAndLoadList(int worldX, int worldZ, int renderRadius);
private:
	ChunkSet loadedChunks;
};