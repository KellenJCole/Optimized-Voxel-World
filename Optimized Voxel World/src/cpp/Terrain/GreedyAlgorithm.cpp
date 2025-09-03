#include "h/Terrain/GreedyAlgorithm.h"
#include <algorithm>
#include <iostream>

GreedyAlgorithm::GreedyAlgorithm() {

}

void GreedyAlgorithm::populatePlanes(
	std::map<BlockFace, std::vector<unsigned int>>& visibleBlockIndexes, 
	std::vector<BlockID>& chunkData, 
	int levelOfDetail
) {

	int blockResolution = 1 << levelOfDetail;
	int width = ChunkUtils::WIDTH / blockResolution;
	int depth = ChunkUtils::DEPTH / blockResolution;
	int height = ChunkUtils::HEIGHT / blockResolution;

	for (auto const& kv : visibleBlockIndexes) {
		BlockFace face = kv.first;
        const std::vector<unsigned int>& indices = kv.second;
        auto& facePlanes = _planes[static_cast<size_t>(face)];

		for (std::vector<unsigned int>::const_iterator itIdx = indices.begin(); itIdx != indices.end(); ++itIdx) {
            unsigned int location = *itIdx;

			int remainder = location % (width * depth);
            int localX = remainder % width;
			int localY = location / (width * depth);
			int localZ = remainder / width;

			int u, v, sliceIndex;
			assignCoordinates(face, localX, localY, localZ, u, v, sliceIndex);

			BlockID type = chunkData[location];
			if (type == BlockID::AIR) continue;

			BlockTextureID tex = textureForFace(type, face);

			// Check if the plane has a vector here
            std::vector<MeshUtils::Plane>::iterator itPlane = std::find_if(
				facePlanes.begin(), facePlanes.end(),
				[sliceIndex](const MeshUtils::Plane& p) { return p.sliceIndex == sliceIndex; });

			if (itPlane == facePlanes.end()) {
                MeshUtils::Plane newPlane;
                newPlane.sliceIndex = sliceIndex;
                int rows, cols;
				switch (face) {
					case BlockFace::NEG_X:
                    case BlockFace::POS_X:
                    case BlockFace::NEG_Z:
                    case BlockFace::POS_Z:
                        rows = width;
                        cols = height;
                        break;
                    case BlockFace::NEG_Y:
                    case BlockFace::POS_Y:
                        rows = width;
                        cols = depth;
                        break;
				}
				newPlane.grid.assign(rows, std::vector<BlockTextureID>(cols, BlockTextureID::AIR));
				facePlanes.push_back(newPlane);
				itPlane = std::prev(facePlanes.end()); // Set iterator to newly added plane
			}

			itPlane->grid[u][v] = tex;
		}
	}
}

void GreedyAlgorithm::firstPassOn(BlockFace f) {
    std::vector<MeshUtils::Plane>& facePlanes = _planes[static_cast<size_t>(f)];
    MeshUtils::MeshGraph& graph = _greedyGraphs[static_cast<size_t>(f)];
    graph.clear();

    for (std::vector<MeshUtils::Plane>::const_iterator pit = facePlanes.begin(); pit != facePlanes.end(); ++pit) {
        const MeshUtils::Plane& plane = *pit;
        int rows = static_cast<int>(plane.grid.size());
        int cols = rows > 0 ? static_cast<int>(plane.grid[0].size()) : 0;
        std::vector<std::vector<bool>> processed(rows, std::vector<bool>(cols, false));

        std::vector<MeshUtils::Quad> quads;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (plane.grid[r][c] == BlockTextureID::AIR || processed[r][c]) continue;

                BlockTextureID tex = plane.grid[r][c];
                int startR = r, endR = r;
                int startC = c, endC = c;

                while (endC + 1 < cols && plane.grid[r][endC + 1] == tex && !processed[r][endC + 1]) {
                    ++endC;
                    processed[r][endC] = true;
                }

                bool canExpand = true;
                while (canExpand && endR + 1 < rows) {
                    for (int cc = startC; cc <= endC; ++cc) {
                        if (plane.grid[endR + 1][cc] != tex || processed[endR + 1][cc]) {
                            canExpand = false;
                            break;
                        }
                    }
                    if (canExpand) {
                        ++endR;
                        for (int cc = startC; cc <= endC; ++cc) {
                            processed[endR][cc] = true;
                        }
                    }
                }

                MeshUtils::Quad quad;
                quad.tex = tex;
                quad.bounds.u0 = startC;
                quad.bounds.v0 = startR;
                quad.bounds.u1 = endC;
                quad.bounds.v1 = endR;
                quads.push_back(quad);
            }
        }

        MeshUtils::MeshSlice slice;
        slice.sliceIndex = plane.sliceIndex;
        slice.quads = quads;
        graph.push_back(slice);
    }

    facePlanes.clear();
}

void GreedyAlgorithm::assignCoordinates(BlockFace face, int localX, int localY, int localZ, int& u, int& v, int& sliceIndex) {
	switch (face) {
        case BlockFace::NEG_X:
        case BlockFace::POS_X:
            u = localZ;
            v = localY;
            sliceIndex = localX;
			break;
        case BlockFace::NEG_Y:
        case BlockFace::POS_Y:
            u = localX;
            v = localZ;
            sliceIndex = localY;
			break;
        case BlockFace::NEG_Z:
        case BlockFace::POS_Z:
            u = localX;
            v = localY;
            sliceIndex = localZ;
			break;
	}
}

const MeshUtils::MeshGraph& GreedyAlgorithm::getMeshGraph(BlockFace f) const {
    return _greedyGraphs[static_cast<size_t>(f)];
}

void GreedyAlgorithm::unload() {
    for (size_t face = 0; face < MeshUtils::FACE_COUNT; ++face) {
        _planes[face].clear();
        _greedyGraphs[face].clear();
    }
}