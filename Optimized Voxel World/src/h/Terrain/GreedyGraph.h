#pragma once
#include <vector>
#include <map>

struct graphVertex {
	typedef graphVertex* ve;
	std::vector<ve> adj;
	std::pair<unsigned char, std::pair<std::pair<int, int>, int>> quad;
    graphVertex(std::pair<unsigned char, std::pair<std::pair<int, int>, int>> q) : quad(q) {}
};

class GreedyGraph {
public:
    GreedyGraph() {}
    std::map<std::pair<unsigned char, std::pair<std::pair<int, int>, int>>, graphVertex*> work;
	void addVertex(const std::pair<unsigned char, std::pair<std::pair<int, int>, int>>& q);
	void addEdge(const std::pair<unsigned char, std::pair<std::pair<int, int>, int>>& qfrom, const std::pair<unsigned char, std::pair<std::pair<int, int>, int>>& qTo);
};