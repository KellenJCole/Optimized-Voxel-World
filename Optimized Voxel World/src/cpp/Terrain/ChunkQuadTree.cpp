#include "h/Terrain/ChunkQuadTree.h"

ChunkQuadTree::ChunkQuadTree() {
	int worldSize = 2048; // The radius, in chunks, of the quadtree. So each quadrant within the head node will be 2048x2048 in this case
	
    root = new QuadNode(glm::vec3(0, 128, 0), worldSize, false, false);
}


void ChunkQuadTree::collectChunks(const Frustum& frustum, std::vector<std::pair<int, int>>& visibleChunks) {
    collectChunksRecursively(root, frustum, visibleChunks);
}

#include <iostream>
void ChunkQuadTree::collectChunksRecursively(QuadNode* node, const Frustum& frustum, std::vector<std::pair<int, int>>& visibleChunks) {
    if (!node) return;

    if (!intersects(frustum, *node)) return;

    if (node->isLeafNode()) {
        visibleChunks.push_back(node->chunkCoords);
    }
    else {
        for (int i = 0; i < 4; ++i) {
            collectChunksRecursively(node->children[i], frustum, visibleChunks);
        }
    }
}

bool ChunkQuadTree::intersects(const Frustum& frustum, const QuadNode& node) {
    glm::vec3 chunkMin = glm::vec3(node.center.x - node.size * 8, 0, node.center.z - node.size * 8);
    glm::vec3 chunkMax = glm::vec3(node.center.x + node.size * 8, 255, node.center.z + node.size * 8);
    BoundingBox chunkBox(chunkMin, chunkMax);

    // Check intersection with each plane of the frustum
    int intersectsCount = 0;
    for (int i = 0; i < 4; ++i) {
        if (chunkBox.intersectsPlane(frustum.planes[i])) {
            intersectsCount++;
        }
    }

    if (intersectsCount > 1) {
        return true;
    }
    else {
        return false; // Bounding box intersects the frustum
    }
}

void ChunkQuadTree::addChunk(const std::pair<int, int>& chunkCoords) {

    addChunkRecursively(root, 1024 * 16, chunkCoords);
}

void ChunkQuadTree::addChunkRecursively(QuadNode* node, int size, const std::pair<int, int>& chunkCoords) {
    if (!node) return;

    int midX = node->center.x / 16;
    int midZ = node->center.z / 16;

    // Quadrant/childIndex of QuadTree
    bool rightHalf = chunkCoords.first >= midX;
    bool topHalf = chunkCoords.second >= midZ;
    int childIndex;
    if (topHalf && !rightHalf) {
        childIndex = 0;
    }
    else if (topHalf && rightHalf) {
        childIndex = 1;
    }
    else if (!topHalf && rightHalf) {
        childIndex = 2;
    }
    else {
        childIndex = 3;
    }

    // Calculate the new quadrant's top-left corner in chunk coordinates
    int newX = rightHalf ? (node->center.x / 16) + (size / 2) : (node->center.x / 16) - (size / 2);
    int newZ = topHalf ? (node->center.z / 16) + (size / 2) : (node->center.z / 16) - (size / 2);

    // Create the child node if it does not exist
    if (!node->children[childIndex]) {
        glm::vec3 childCenter = glm::vec3(newX * 16, node->center.y, newZ * 16);
        node->children[childIndex] = new QuadNode(childCenter, size / 2, false, false);  // It's not a leaf yet
    }

    // If we've reached a leaf node, this is the end of the recursion
    if (size == 2) {  // Since we're dealing with chunks, a size of 2 means we're at the chunk level
        glm::vec3 leafCenter = glm::vec3(newX * 16, node->center.y, newZ * 16);
        node->children[childIndex]->center = leafCenter;
        node->children[childIndex]->chunkCoords = chunkCoords;
        node->children[childIndex]->chunkExists = true;
        node->children[childIndex]->isLeaf = true;
        std::cout << "Expected (" << chunkCoords.first * 16 + 8 << ", 128, " << chunkCoords.second * 16 + 8 << "). Actual (" << node->center.x << ", " << node->center.y << ", " << node->center.z << ").\n";
        return;
    }

    // Recursively add the chunk to the correct child node
    addChunkRecursively(node->children[childIndex], size / 2, chunkCoords);
}
