#pragma once
#ifndef BLOCKID_H
#define BLOCKID_H

enum class BlockID : unsigned char {
	AIR,
	DIRT,
	GRASS,
	STONE,
	BEDROCK, 
	NONE,
	Count
};

#endif