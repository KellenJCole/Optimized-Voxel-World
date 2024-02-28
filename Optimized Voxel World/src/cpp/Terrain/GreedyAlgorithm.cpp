#include "h/Terrain/GreedyAlgorithm.h"
#include <algorithm>
#include <iostream>
#define CHUNK_WIDTH 16
#define CHUNK_HEIGHT 256
#define CHUNK_DEPTH 16

enum BlockFace {
	NEG_X = 1 << 0, // 0b000001
	POS_X = 1 << 1, // 0b000010
	NEG_Y = 1 << 2, // 0b000100
	POS_Y = 1 << 3, // 0b001000
	NEG_Z = 1 << 4, // 0b010000
	POS_Z = 1 << 5  // 0b100000
};

GreedyAlgorithm::GreedyAlgorithm() {

}

void GreedyAlgorithm::firstPassOn(BlockFace f) {
	int greedyGraphIndex;

	switch (f) {
	case NEG_X: greedyGraphIndex = 0; break;
	case POS_X: greedyGraphIndex = 1; break;
	case NEG_Y: greedyGraphIndex = 2; break;
	case POS_Y: greedyGraphIndex = 3; break;
	case NEG_Z: greedyGraphIndex = 4; break;
	case POS_Z: greedyGraphIndex = 5; break;
	}


	for (auto& p : planes[f]) {
		std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>> meshesForThisFace;
		auto& plane = p.second;
		std::vector<std::vector<bool>> processed(plane.size(), std::vector<bool>(plane[0].size(), false));

		for (int firstAxis = 0; firstAxis < plane.size(); ++firstAxis) {
			for (int secondAxis = 0; secondAxis < plane[firstAxis].size(); ++secondAxis) {
				if (plane[firstAxis][secondAxis] == 0 || processed[firstAxis][secondAxis]) continue;

				unsigned char blockType = plane[firstAxis][secondAxis];
				int startFirst = firstAxis, endFirst = firstAxis, startSecond = secondAxis, endSecond = secondAxis;

				// Correctly increment endSecond in the loop
				while (endSecond + 1 < plane[firstAxis].size() && plane[firstAxis][endSecond + 1] == blockType && !processed[firstAxis][endSecond + 1]) {
					processed[firstAxis][++endSecond] = true;
				}

				// Expand vertically across columns (the previous comment was misleading)
				bool canExpand = true;
				while (canExpand && endFirst + 1 < plane.size()) {
					for (int checkSecond = startSecond; checkSecond <= endSecond; ++checkSecond) {
						if (plane[endFirst + 1][checkSecond] != blockType || processed[endFirst + 1][checkSecond]) {
							canExpand = false;
							break;
						}
					}
					if (canExpand) {
						++endFirst;
						for (int markSecond = startSecond; markSecond <= endSecond; ++markSecond) {
							processed[endFirst][markSecond] = true;
						}
					}
				}

				// Correct the mesh addition with the right variable names
				meshesForThisFace.push_back({ blockType, {{startSecond, endSecond}, {startFirst, endFirst}} });
			}
		}

		Greedy_Graphs[greedyGraphIndex].push_back({ meshesForThisFace, p.first });
	}

	planes[f].clear();
}

void GreedyAlgorithm::populatePlanes(std::map<BlockFace, std::vector<unsigned int>>& vb, std::vector<unsigned char>& c) {
	for (const auto& blockLocation : vb) {
		BlockFace face = blockLocation.first;

		for (auto location : blockLocation.second) {
			int x, y, z;
			switch (face) {
			case NEG_X:
			case POS_X:
				x = location & 0x0F;
				y = (location & 0xFF) >> 4;
				z = location >> 8;
				break;
			case NEG_Y:
			case POS_Y:
				x = location >> 8;
				y = location & 0x0F;
				z = (location & 0xFF) >> 4;
				break;
			case NEG_Z:
			case POS_Z:
				x = (location & 0xFF) >> 4;
				y = location & 0x0F;
				z = location >> 8;
				break;
			}

			unsigned char blockType = c[location];
			if (blockType != 0) { // If not an air block
				// Check if the plane has a vector here
				auto& planeVec = planes[face]; // Reference to the vector of planes for this face
				auto it = std::find_if(planeVec.begin(), planeVec.end(),
					[x](const std::pair<int, std::vector<std::vector<unsigned char>>>& elem) { return elem.first == x; });

				if (it == planeVec.end()) {
					std::vector<std::vector<unsigned char>> newPlane;
					switch (face) {
					case NEG_X:
					case POS_X:
					case NEG_Z:
					case POS_Z: newPlane.resize(CHUNK_WIDTH, std::vector<unsigned char>(CHUNK_HEIGHT, 0)); break;
					case NEG_Y:
					case POS_Y: newPlane.resize(CHUNK_WIDTH, std::vector<unsigned char>(CHUNK_DEPTH, 0)); break;
					}
					planeVec.push_back({ x, newPlane });
					it = std::prev(planeVec.end()); // Set iterator to newly added plane
				}

				it->second[y][z] = blockType;
			}
		}
	}
}

std::vector<std::pair<std::vector<std::pair<unsigned char, std::pair<std::pair<int, int>, std::pair<int, int>>>>, int>> GreedyAlgorithm::getAllGreedyGraphs(int faceType) {
	return Greedy_Graphs[faceType];
}

void GreedyAlgorithm::unload() {
	for (int i = 0; i < 6; i++) {
		Greedy_Graphs[i].clear();
	}
}