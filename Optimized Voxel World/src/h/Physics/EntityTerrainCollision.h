#pragma once

#include "h/external/glm/vec3.hpp"
#include "h/Terrain/WorldManager.h"
#include "h/Terrain/Utility/ChunkUtils.h"
#include "h/Terrain/Utility/BlockID.h"
#include "h/Utility/AABB.h"

#include <unordered_map>
#include <vector>

class EntityTerrainCollision {
public:
	struct Contact {
		bool hitX = false, hitY = false, hitZ = false;
		glm::vec3 normal{ 0.f };
	};

	struct Result {
		glm::vec3 newCenter;
		glm::vec3 newVelocity;
		Contact contact;
	};

	struct CachedChunk {
		std::shared_ptr<const std::vector<BlockID>> data;
	};

	EntityTerrainCollision() : world(nullptr) {}
	void setWorldPtr(WorldManager* w) { world = w; }

	Result sweepResolve(const AABB& box, float dt);
private:
	void refreshLocalChunks(const glm::ivec3& wmin, const glm::ivec3& wmax);

	bool isSolidLocal(int wx, int wy, int wz) const;

	static constexpr float kEps = 1e-4f;

	void sweepAxisX(glm::vec3 half, float& x, float y, float z, float& dx, Contact& c) const;
	void sweepAxisY(glm::vec3 half, float x, float& y, float z, float& dy, Contact& c) const;
	void sweepAxisZ(glm::vec3 half, float x, float y, float& z, float& dz, Contact& c) const;

	static inline int floori(float v) { return static_cast<int>(std::floor(v)); }
	static inline int ceili(float v) { return static_cast<int>(std::ceil(v)); }

	WorldManager* world;
	std::unordered_map<ChunkUtils::ChunkCoordPair, CachedChunk, ChunkUtils::PairHash> chunkData_;
};