#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>
#include "h/Constants.h"

enum BlockFace;
class GreedyAlgorithm {
public:
	GreedyAlgorithm();
	void populatePlanes(std::map<BlockFace, std::vector<unsigned int>>& vb, std::vector<unsigned char>& c, int levelOfDetail);
	void firstPassOn(BlockFace f);
	std::vector<std::pair<std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>>, int>> getAllGreedyGraphs(int faceType);
	void unload();
private:
	std::unordered_map<BlockFace, std::vector<std::pair<int, std::vector<std::vector<unsigned char>>>>> planes;
	std::vector<std::pair<std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>>, int>> Greedy_Graphs[6];
	void assignCoordinates(BlockFace face, int unknownCoord1, int unknownCoord2, int unknownCoord3, int& x, int& y, int& z);
};