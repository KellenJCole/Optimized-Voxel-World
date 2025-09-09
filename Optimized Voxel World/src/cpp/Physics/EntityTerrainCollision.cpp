#include "h/Physics/EntityTerrainCollision.h"
#include <cmath>
#include <algorithm>
#include <iostream>

EntityTerrainCollision::Result EntityTerrainCollision::sweepResolve(const AABB& box, float dt)
{
    Result r;
    r.newCenter = box.center;

    // Planned change in coordinates
    glm::vec3 vel = box.velocity;
    float dx = vel.x * dt;
    float dy = vel.y * dt;
    float dz = vel.z * dt;

    // Compute conservative swept AABB in world coords entity may touch this frame.
    glm::vec3 startMin = box.center - box.half;
    glm::vec3 startMax = box.center + box.half;
    glm::vec3 endMin = startMin + glm::vec3(dx, dy, dz);
    glm::vec3 endMax = startMax + glm::vec3(dx, dy, dz);

    glm::vec3 bbMin = glm::min(startMin, endMin);
    glm::vec3 bbMax = glm::max(startMax, endMax);

    // Clamp vertical to world height
    int yMin = std::max(0, floori(bbMin.y));
    int yMax = std::min(255, floori(bbMax.y));

    // Refresh local chunk cache if necessary
    refreshLocalChunks(glm::ivec3(floori(bbMin.x), yMin, floori(bbMin.z)),
        glm::ivec3(floori(bbMax.x), yMax, floori(bbMax.z)));

    // Resolve Y first (landing), then X, then Z
    sweepAxisY(box.half, r.newCenter.x, r.newCenter.y, r.newCenter.z, dy, r.contact);
    if (r.contact.hitY) vel.y = 0.0f;

    sweepAxisX(box.half, r.newCenter.x, r.newCenter.y, r.newCenter.z, dx, r.contact);
    if (r.contact.hitX) vel.x = 0.0f;

    sweepAxisZ(box.half, r.newCenter.x, r.newCenter.y, r.newCenter.z, dz, r.contact);
    if (r.contact.hitZ) vel.z = 0.0f;

    r.newVelocity = vel;
    return r;
}

void EntityTerrainCollision::refreshLocalChunks(const glm::ivec3& wmin, const glm::ivec3& wmax)
{
    if (!world) return;

    // prefetch ring
    constexpr int padChunks = 1;

    int cx0 = ChunkUtils::worldToChunkCoord(wmin.x);
    int cz0 = ChunkUtils::worldToChunkCoord(wmin.z);
    int cx1 = ChunkUtils::worldToChunkCoord(wmax.x);
    int cz1 = ChunkUtils::worldToChunkCoord(wmax.z);
    if (cx0 > cx1) std::swap(cx0, cx1);
    if (cz0 > cz1) std::swap(cz0, cz1);

    const int pcx0 = cx0 - padChunks;
    const int pcz0 = cz0 - padChunks;
    const int pcx1 = cx1 + padChunks;
    const int pcz1 = cz1 + padChunks;

    std::unordered_set<ChunkUtils::ChunkCoordPair, ChunkUtils::PairHash> needed;
    needed.reserve((pcx1 - pcx0 + 1) * (pcz1 - pcz0 + 1));
    for (int cx = pcx0; cx <= pcx1; ++cx)
        for (int cz = pcz0; cz <= pcz1; ++cz)
            needed.insert({ cx, cz });

    for (const auto& key : needed) {
        auto snap = world->tryGetChunkSnapshot(key);
        if (snap && !snap->empty()) {
            chunkData_[key].data = std::move(snap);
        }
    }

    for (auto it = chunkData_.begin(); it != chunkData_.end(); ) {
        if (!needed.count(it->first)) it = chunkData_.erase(it);
        else ++it;
    }
}

bool EntityTerrainCollision::isSolidLocal(int wx, int wy, int wz) const
{
    if (wy < 0 || wy > 255) return false;

    int cx = ChunkUtils::worldToChunkCoord(wx);
    int cz = ChunkUtils::worldToChunkCoord(wz);

    auto it = chunkData_.find({ cx, cz });
    if (it == chunkData_.end() || !it->second.data) return true;

    int lx = ChunkUtils::convertWorldCoordToLocalCoord(wx);
    int lz = ChunkUtils::convertWorldCoordToLocalCoord(wz);

    int idx = ChunkUtils::flattenChunkCoords(lx, wy, lz, 0);

    const auto& vec = *it->second.data;

    if (idx < 0 || idx >= (int)vec.size()) return true;

    return vec[idx] != BlockID::AIR;
}

// ------------- Axis sweeps (voxel planes) -------------
void EntityTerrainCollision::sweepAxisX(glm::vec3 half, float& x, float y, float z, float& dx, Contact& c) const
{
    if (dx == 0.0f) return;

    const int y0 = floori(y - half.y);
    const int y1 = floori(y + half.y - kEps);
    const int z0 = floori(z - half.z);
    const int z1 = floori(z + half.z - kEps);

    if (dx > 0.0f) {
        float startMax = x + half.x;
        float endMax = startMax + dx;
        int firstPlane = ceili(startMax);
        int lastPlane = floori(endMax + kEps);

        for (int plane = firstPlane; plane <= lastPlane; ++plane) {
            bool blocked = false;
            for (int iy = y0; iy <= y1 && !blocked; ++iy)
                for (int iz = z0; iz <= z1; ++iz)
                    if (isSolidLocal(plane, iy, iz)) { blocked = true; break; }

            if (blocked) {
                float newMax = (float)plane - kEps;
                dx = newMax - startMax;
                x = newMax - half.x;
                c.hitX = true; c.normal.x = -1.f;
                return;
            }
        }
        x += dx;
    }
    else {
        float startMin = x - half.x;
        float endMin = startMin + dx;
        int firstPlane = floori(startMin);
        int lastPlane = ceili(endMin - kEps);

        for (int plane = firstPlane; plane >= lastPlane; --plane) {
            int blockX = plane - 1;
            bool blocked = false;
            for (int iy = y0; iy <= y1 && !blocked; ++iy)
                for (int iz = z0; iz <= z1; ++iz)
                    if (isSolidLocal(blockX, iy, iz)) { blocked = true; break; }

            if (blocked) {
                float newMin = (float)plane + kEps;
                dx = newMin - startMin;
                x = newMin + half.x;
                c.hitX = true; c.normal.x = 1.f;
                return;
            }
        }
        x += dx;
    }
}

void EntityTerrainCollision::sweepAxisY(glm::vec3 half, float x, float& y, float z, float& dy, Contact& c) const
{
    if (dy == 0.0f) return;

    const int x0 = floori(x - half.x);
    const int x1 = floori(x + half.x - kEps);
    const int z0 = floori(z - half.z);
    const int z1 = floori(z + half.z - kEps);

    if (dy > 0.0f) {
        float startMax = y + half.y;
        float endMax = startMax + dy;
        int firstPlane = ceili(startMax);
        int lastPlane = floori(endMax + kEps);

        for (int plane = firstPlane; plane <= lastPlane; ++plane) {
            bool blocked = false;
            for (int ix = x0; ix <= x1 && !blocked; ++ix)
                for (int iz = z0; iz <= z1; ++iz)
                    if (isSolidLocal(ix, plane, iz)) { blocked = true; break; }

            if (blocked) {
                float newMax = (float)plane - kEps;
                dy = newMax - startMax;
                y = newMax - half.y;
                c.hitY = true; c.normal.y = -1.f;
                return;
            }
        }
        y += dy;
    }
    else {
        float startMin = y - half.y;
        float endMin = startMin + dy;
        int firstPlane = floori(startMin);
        int lastPlane = ceili(endMin - kEps);

        for (int plane = firstPlane; plane >= lastPlane; --plane) {
            int blockY = plane - 1;
            bool blocked = false;
            for (int ix = x0; ix <= x1 && !blocked; ++ix)
                for (int iz = z0; iz <= z1; ++iz)
                    if (isSolidLocal(ix, blockY, iz)) { blocked = true; break; }

            if (blocked) {
                float newMin = (float)plane + kEps;
                dy = newMin - startMin;
                y = newMin + half.y;
                c.hitY = true; c.normal.y = 1.f;
                return;
            }
        }
        y += dy;
    }
}

void EntityTerrainCollision::sweepAxisZ(glm::vec3 half, float x, float y, float& z, float& dz, Contact& c) const
{
    if (dz == 0.0f) return;

    const int x0 = floori(x - half.x);
    const int x1 = floori(x + half.x - kEps);
    const int y0 = floori(y - half.y);
    const int y1 = floori(y + half.y - kEps);

    if (dz > 0.0f) {
        float startMax = z + half.z;
        float endMax = startMax + dz;
        int firstPlane = ceili(startMax);
        int lastPlane = floori(endMax + kEps);

        for (int plane = firstPlane; plane <= lastPlane; ++plane) {
            bool blocked = false;
            for (int ix = x0; ix <= x1 && !blocked; ++ix)
                for (int iy = y0; iy <= y1; ++iy)
                    if (isSolidLocal(ix, iy, plane)) { blocked = true; break; }

            if (blocked) {
                float newMax = (float)plane - kEps;
                dz = newMax - startMax;
                z = newMax - half.z;
                c.hitZ = true; c.normal.z = -1.f;
                return;
            }
        }
        z += dz;
    }
    else {
        float startMin = z - half.z;
        float endMin = startMin + dz;
        int firstPlane = floori(startMin);
        int lastPlane = ceili(endMin - kEps);

        for (int plane = firstPlane; plane >= lastPlane; --plane) {
            int blockZ = plane - 1;
            bool blocked = false;
            for (int ix = x0; ix <= x1 && !blocked; ++ix)
                for (int iy = y0; iy <= y1; ++iy)
                    if (isSolidLocal(ix, iy, blockZ)) { blocked = true; break; }

            if (blocked) {
                float newMin = (float)plane + kEps;
                dz = newMin - startMin;
                z = newMin + half.z;
                c.hitZ = true; c.normal.z = 1.f;
                return;
            }
        }
        z += dz;
    }
}