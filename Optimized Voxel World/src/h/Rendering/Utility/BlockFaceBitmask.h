#pragma once  

enum class BlockFaceBitmask : unsigned char {
   NONE = 0,
   NEG_X = 0b000001,  
   POS_X = 0b000010,  
   NEG_Y = 0b000100,  
   POS_Y = 0b001000,  
   NEG_Z = 0b010000,  
   POS_Z = 0b100000  
};  

// Define bitwise operators for BlockFaceBitmask  
inline constexpr BlockFaceBitmask operator&(BlockFaceBitmask lhs, BlockFaceBitmask rhs) {  
   return static_cast<BlockFaceBitmask>(  
       static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs));  
}  

inline constexpr BlockFaceBitmask operator|(BlockFaceBitmask lhs, BlockFaceBitmask rhs) {  
   return static_cast<BlockFaceBitmask>(  
       static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs));  
}  

inline constexpr BlockFaceBitmask& operator|=(BlockFaceBitmask& lhs, BlockFaceBitmask rhs) {  
   lhs = lhs | rhs;  
   return lhs;  
}  

inline constexpr BlockFaceBitmask& operator&=(BlockFaceBitmask& lhs, BlockFaceBitmask rhs) {  
   lhs = lhs & rhs;  
   return lhs;  
}

inline constexpr bool has(BlockFaceBitmask mask, BlockFaceBitmask flag) {
    return static_cast<unsigned char>(mask & flag) != 0;
}
