#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>

enum BlockFace;
class GreedyAlgorithm {
public:
	GreedyAlgorithm();
	void populatePlanes(std::map<BlockFace, std::vector<unsigned int>>& vb, std::vector<unsigned char>& c);
	void firstPassOn(BlockFace f);
	std::vector<std::pair<std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>>, int>> getAllGreedyGraphs(int faceType);
	void unload();
private:
	std::unordered_map<BlockFace, std::vector<std::pair<int, std::vector<std::vector<unsigned char>>>>> planes;
	std::vector<std::pair<std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>>, int>> Greedy_Graphs[6];
};