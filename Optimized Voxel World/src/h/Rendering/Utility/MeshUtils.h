#pragma once

#include <vector>
#include <array>
#include "h/Rendering/Utility/BlockFace.h"
#include "h/Rendering/Utility/BlockTextureID.h"

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

}