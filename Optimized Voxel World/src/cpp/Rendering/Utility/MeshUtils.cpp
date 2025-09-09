#include "h/Rendering/Utility/MeshUtils.h"

#include <unordered_map>

void MeshUtils::addVerticesForQuad(std::vector<Vertex>& verts, std::vector<unsigned int>& inds, const Quad& quad, ChunkUtils::ChunkCoordPair chunkCoords, int faceType, int sliceIndex, int levelOfDetail, unsigned int baseIndex) {
	// Calculate positions for each corner of the quad
	glm::vec3 bl = calculatePosition(quad, 0, faceType, chunkCoords, sliceIndex, levelOfDetail);
	glm::vec3 br = calculatePosition(quad, 1, faceType, chunkCoords, sliceIndex, levelOfDetail);
	glm::vec3 tl = calculatePosition(quad, 2, faceType, chunkCoords, sliceIndex, levelOfDetail);
	glm::vec3 tr = calculatePosition(quad, 3, faceType, chunkCoords, sliceIndex, levelOfDetail);

	// Calculate texture coordinates for each corner
	glm::vec2 texBL = calculateTexCoords(quad, 0);
	glm::vec2 texBR = calculateTexCoords(quad, 1);
	glm::vec2 texTL = calculateTexCoords(quad, 2);
	glm::vec2 texTR = calculateTexCoords(quad, 3);

	// Define normal based on face type
	int normal = faceType;
	BlockTextureID tex = quad.tex;

	// Add vertices for the quad
	verts.push_back(Vertex{ bl, texBL, normal + static_cast<int>(tex) * 16 });
	verts.push_back(Vertex{ br, texBR, normal + static_cast<int>(tex) * 16 });
	verts.push_back(Vertex{ tr, texTR, normal + static_cast<int>(tex) * 16 });
	verts.push_back(Vertex{ tl, texTL, normal + static_cast<int>(tex) * 16 });

	// Add indices for two triangles making up the quad
	switch (faceType) {
	case 0: case 2: case 5:
		inds.insert(inds.end(), { baseIndex + 2, baseIndex + 1, baseIndex + 0,
								  baseIndex + 2, baseIndex + 3, baseIndex + 1 });
		break;
	case 1: case 3: case 4:
		inds.insert(inds.end(), { baseIndex + 0, baseIndex + 1, baseIndex + 2,
								  baseIndex + 1, baseIndex + 3, baseIndex + 2 });
		break;
	}
}

glm::vec3 MeshUtils::calculatePosition(const Quad& quad, int corner, size_t faceType, ChunkUtils::ChunkCoordPair cxcz, int sliceIndex, int levelOfDetail) {
	int blockResolution = 1 << levelOfDetail;

	int startX = cxcz.first * ChunkUtils::WIDTH;
	int startZ = cxcz.second * ChunkUtils::DEPTH;

	int u0 = quad.bounds.u0 * blockResolution;
	int v0 = quad.bounds.v0 * blockResolution;
	int u1 = quad.bounds.u1 * blockResolution;
	int v1 = quad.bounds.v1 * blockResolution;

	int sliceConverted = sliceIndex * blockResolution;

	int sliceXNeg = startX + sliceConverted;
	int sliceXPos = sliceXNeg + blockResolution;

	int sliceYNeg = sliceConverted;
	int sliceYPos = sliceYNeg + blockResolution;

	int sliceZNeg = startZ + sliceConverted;
	int sliceZPos = sliceZNeg + blockResolution;

	// x face variables
	int xy = u0;
	int xy1 = u1 + blockResolution;
	int xz = startZ + v0;
	int xz1 = startZ + v1 + blockResolution;

	// y face variables

	int yx = startX + v0;
	int yx1 = startX + v1 + blockResolution;
	int yz = startZ + u0;
	int yz1 = startZ + u1 + blockResolution;

	// z face variables
	int zx = startX + v0;
	int zx1 = startX + v1 + blockResolution;
	int zy = u0;
	int zy1 = u1 + blockResolution;

	std::unordered_map<int, int> sliceMap;

	sliceMap[0] = sliceXNeg;
	sliceMap[1] = sliceXPos;
	sliceMap[2] = sliceYNeg;
	sliceMap[3] = sliceYPos;
	sliceMap[4] = sliceZNeg;
	sliceMap[5] = sliceZPos;

	switch (faceType) {
	case 0:
	case 1:
		switch (corner) {
		case 0:
			return glm::vec3(sliceMap[faceType], xy, xz);
		case 1:
			return glm::vec3(sliceMap[faceType], xy1, xz);
		case 2:
			return glm::vec3(sliceMap[faceType], xy1, xz1);
		case 3:
			return glm::vec3(sliceMap[faceType], xy, xz1);
		}
	case 2:
	case 3:
		switch (corner) {
		case 0:
			return glm::vec3(yx, sliceMap[faceType], yz);
		case 1:
			return glm::vec3(yx, sliceMap[faceType], yz1);
		case 2:
			return glm::vec3(yx1, sliceMap[faceType], yz1);
		case 3:
			return glm::vec3(yx1, sliceMap[faceType], yz);
		}
	case 4:
	case 5:
		switch (corner) {
		case 0:
			return glm::vec3(zx, zy, sliceMap[faceType]);
		case 1:
			return glm::vec3(zx, zy1, sliceMap[faceType]);
		case 2:
			return glm::vec3(zx1, zy1, sliceMap[faceType]);
		case 3:
			return glm::vec3(zx1, zy, sliceMap[faceType]);
		}
	default:
		return glm::vec3(NULL, NULL, NULL);
	}
}

glm::vec2 MeshUtils::calculateTexCoords(const Quad& quad, int corner) {
	switch (corner) {
	case 0:
		return glm::vec2(0.0f, 0.0f);
	case 1:
		return glm::vec2((quad.bounds.u1 - quad.bounds.u0 + 1), 0.0f);
	case 2:
		return glm::vec2((quad.bounds.u1 - quad.bounds.u0 + 1), (quad.bounds.v1 - quad.bounds.v0 + 1));
	case 3:
		return glm::vec2(0.0f, (quad.bounds.v1 - quad.bounds.v0 + 1));
	default:
		break;
	}
}