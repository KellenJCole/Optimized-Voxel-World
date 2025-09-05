#pragma once
#ifndef BLOCKFACE_H
#define BLOCKFACE_H

enum class BlockFace : unsigned char { NEG_X, POS_X, NEG_Y, POS_Y, NEG_Z, POS_Z, Count };

inline int toInt(BlockFace face) {
	return static_cast<int>(face);
}

inline BlockFace opposite(BlockFace face) {
	switch (face) {
	case BlockFace::NEG_X: return BlockFace::POS_X;
	case BlockFace::POS_X: return BlockFace::NEG_X;
	case BlockFace::NEG_Y: return BlockFace::POS_Y;
	case BlockFace::POS_Y: return BlockFace::NEG_Y;
	case BlockFace::NEG_Z: return BlockFace::POS_Z;
	case BlockFace::POS_Z: return BlockFace::NEG_Z;
	default: return BlockFace::Count; // How did we get here? How, like Tootle the Train, did we get so off track?
	}
}

#endif