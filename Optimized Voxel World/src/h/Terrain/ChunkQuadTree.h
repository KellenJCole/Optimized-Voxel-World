#pragma once
#include <vector>
#include <set>
#include "h/glm/glm.hpp"
#include "h/glm/common.hpp"
#include "h/glm/gtc/matrix_access.hpp"

struct Plane {
    glm::vec3 normal; // Normal vector of the plane
    float distance;   // Distance from the origin

    Plane() : normal(glm::vec3(0, 0, 0)), distance(0) {}

    Plane(const glm::vec4& coefficients)
        : normal(glm::vec3(coefficients)), distance(coefficients.w) {
        normalize();
    }

    void normalize() {
        float length = glm::length(normal);
        if (length > 0.0f) {
            normal /= length;
            distance /= length;
        }
    }

    // Calculate the distance from the plane to a point in space
    float distanceToPoint(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }
};

struct Frustum {
    Plane planes[6]; // left, right, bottom, top, near, far

    // Initialize the frustum planes from the view-projection matrix
    void update(const glm::mat4& vpMatrix) {

        // Right plane
        planes[0] = Plane(glm::row(vpMatrix, 3) - glm::row(vpMatrix, 0));
        // Left plane
        planes[1] = Plane(glm::row(vpMatrix, 3) + glm::row(vpMatrix, 0));
        // Top plane
        planes[2] = Plane(glm::row(vpMatrix, 3) - glm::row(vpMatrix, 1));
        // Bottom plane
        planes[3] = Plane(glm::row(vpMatrix, 3) + glm::row(vpMatrix, 1));
        // Far plane
        planes[4] = Plane(glm::row(vpMatrix, 3) - glm::row(vpMatrix, 2));
        // Near plane
        planes[5] = Plane(glm::row(vpMatrix, 3) + glm::row(vpMatrix, 2));

    }
};

struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;

    BoundingBox(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    bool intersectsPlane(const Plane& plane) const {
        // Get positive and negative vertex in relation to the plane normal
        glm::vec3 pVertex = min, nVertex = max;

        if (plane.normal.x >= 0) {
            pVertex.x = max.x;
            nVertex.x = min.x;
        }
        if (plane.normal.y >= 0) {
            pVertex.y = max.y;
            nVertex.y = min.y;
        }
        if (plane.normal.z >= 0) {
            pVertex.z = max.z;
            nVertex.z = min.z;
        }

        // If the positive vertex is behind the plane, the whole AABB is behind the plane
        if (plane.distanceToPoint(pVertex) < 0) {
            return false;
        }

        // If the negative vertex is in front of the plane, the AABB intersects the plane
        return true;
    }
};

struct QuadNode {
	glm::vec3 center; // Center of this node
	float size; // Length of the sides of the cube represented by this node
	QuadNode* children[4] = { nullptr };
	std::pair<int, int> chunkCoords; // Only has value if deepest layer of quadtree
    bool chunkExists;
	bool isLeaf;

	QuadNode(const glm::vec3& center, float size = 0, bool isLeaf = false, bool chunkExists = false)
		: center(center), size(size), isLeaf(isLeaf), chunkExists(chunkExists) {}
	bool isLeafNode() const { return isLeaf; }

	~QuadNode() {
        for (auto& child : children) {
            delete child;
        }
    }
};

class ChunkQuadTree {
public:
	ChunkQuadTree();
    void collectChunks(const Frustum& frustum, std::vector<std::pair<int, int>>& visibleChunks);
    void addChunk(const std::pair<int, int>& chunkCoords);
private:
    void addChunkRecursively(QuadNode* node, int size, const std::pair<int, int>& chunkCoords);
    void collectChunksRecursively(QuadNode* node, const Frustum& frustum, std::vector<std::pair<int, int>>& visibleChunks);
    bool intersects(const Frustum& frustum, const QuadNode& node);
	QuadNode* root;
    std::set<std::pair<int, int>> test;
    std::set<std::pair<int, int>> cctest;
    int count = 0;
};