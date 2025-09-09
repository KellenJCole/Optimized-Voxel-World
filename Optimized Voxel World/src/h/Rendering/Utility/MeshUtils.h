#pragma once

#include <vector>
#include <array>
#include "h/external/glm/vec2.hpp"
#include "h/external/glm/vec3.hpp"
#include "h/Rendering/Utility/BlockFace.h"
#include "h/Rendering/Utility/BlockTextureID.h"
#include "h/Rendering/Utility/BlockGeometry.h"
#include "h/Terrain/Utility/ChunkUtils.h"

namespace MeshUtils {

	struct Plane {
		int sliceIndex;
        std::vector<std::vector<BlockTextureID>> grid;
	};

	struct Bounds {
		int u0, v0;
        int u1, v1;
	};

	struct Quad {
        BlockTextureID tex;
        Bounds bounds;
	};

	struct MeshSlice {
        int sliceIndex;
		std::vector<Quad> quads;
	};

	using MeshGraph = std::vector<MeshSlice>;

	static constexpr size_t FACE_COUNT = 6;
	using FaceMeshGraphs = std::array<MeshGraph, FACE_COUNT>;

	void addVerticesForQuad(std::vector<Vertex>& verts, std::vector<unsigned int>& inds, const Quad& quad, ChunkUtils::ChunkCoordPair chunkCoords, int faceType, int sliceIndex, int levelOfDetail, unsigned int baseIndex);

	glm::vec3 calculatePosition(const Quad& quad, int corner, size_t faceType, ChunkUtils::ChunkCoordPair chunkCoords, int sliceIndex, int levelOfDetail);

	glm::vec2 calculateTexCoords(const Quad& quad, int corner);

}