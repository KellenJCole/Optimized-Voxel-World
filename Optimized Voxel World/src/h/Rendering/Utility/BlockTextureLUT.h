#pragma once

#include "h/Rendering/Utility/BlockTextureID.h"
#include "h/Rendering/Utility/BlockFace.h"
#include "h/Terrain/Utility/BlockID.h"

#include <array>

struct BlockTexturesPerFace {
    BlockTextureID negX, posX, negY, posY, negZ, posZ;
};

constexpr std::array<BlockTexturesPerFace, static_cast<std::size_t>(BlockID::Count)> kBlockTextures = {
    {
        
        /* AIR */
        {   BlockTextureID::AIR, BlockTextureID::AIR, 
            BlockTextureID::AIR, BlockTextureID::AIR, 
            BlockTextureID::AIR,BlockTextureID::AIR
        },

        /* DIRT */
        {  
            BlockTextureID::DIRT, BlockTextureID::DIRT, 
            BlockTextureID::DIRT, BlockTextureID::DIRT, 
            BlockTextureID::DIRT, BlockTextureID::DIRT
        },

        /* GRASS */
        {  
            BlockTextureID::GRASS_SIDE, BlockTextureID::GRASS_SIDE, 
            BlockTextureID::DIRT, BlockTextureID::GRASS_TOP,
            BlockTextureID::GRASS_SIDE, BlockTextureID::GRASS_SIDE
        },

        /* STONE */
        {  
            BlockTextureID::STONE, BlockTextureID::STONE, 
            BlockTextureID::STONE, BlockTextureID::STONE, 
            BlockTextureID::STONE, BlockTextureID::STONE
        },

        /* BEDROCK */
        { 
            BlockTextureID::BEDROCK, BlockTextureID::BEDROCK, 
            BlockTextureID::BEDROCK, BlockTextureID::BEDROCK,
            BlockTextureID::BEDROCK, BlockTextureID::BEDROCK
        },

        /* NONE */
        { 
            BlockTextureID::NONE, BlockTextureID::NONE, 
            BlockTextureID::NONE, BlockTextureID::NONE, 
            BlockTextureID::NONE, BlockTextureID::NONE
        }
}};

inline constexpr BlockTextureID textureForFace(BlockID id, BlockFace face) {
    const BlockTexturesPerFace& t = kBlockTextures[static_cast<std::size_t>(id)];

    switch (face) {
        case BlockFace::NEG_X:
            return t.negX;
        case BlockFace::POS_X:
            return t.posX;
        case BlockFace::NEG_Y:
            return t.negY;
        case BlockFace::POS_Y:
            return t.posY;
        case BlockFace::NEG_Z:
            return t.negZ;
        case BlockFace::POS_Z:
            return t.posZ;
        default:
            return BlockTextureID::AIR;  // fallback, should not occur
    }
}
