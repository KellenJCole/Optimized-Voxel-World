#include "h/Terrain/GreedyGraph.h"

void GreedyGraph::addVertex(
    const std::pair<unsigned char, std::pair<std::pair<int, int>, int>>& q)
{
    std::map<std::pair<unsigned char, std::pair<std::pair<int, int>, int>>, graphVertex*>::iterator itr = work.find(q);
    if (itr == work.end())
    {
        graphVertex* v;
        v = new graphVertex(q);
        work[q] = v;
        return;
    }
}

void GreedyGraph::addEdge(
    const std::pair<unsigned char, std::pair<std::pair<int, int>, int>>& qfrom, 
    const std::pair<unsigned char, std::pair<std::pair<int, int>, int>>& qTo)
{
    graphVertex* f = (work.find(qfrom)->second);
    graphVertex* t = (work.find(qTo)->second);
    graphVertex* edge = t;
    f->adj.push_back(edge);
}