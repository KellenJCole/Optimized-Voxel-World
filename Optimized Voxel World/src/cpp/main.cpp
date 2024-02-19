#include "h/VoxelEngine.h"
#include <iostream>

int main() {
	VoxelEngine engine;
	if (!engine.initialize()) {
		std::cout << "Failed to initialize Voxel Engine. aborting.\n";
		return -1;
	}

	engine.run();

	return 0;
}