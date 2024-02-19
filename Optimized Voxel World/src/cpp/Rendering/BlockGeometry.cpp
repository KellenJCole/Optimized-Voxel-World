#include "h/Rendering/BlockGeometry.h"

const float BlockGeometry::vertices[192] = {
    // Left face
   -0.5f, -0.5f,   -0.5f,      0.0f, 0.0f,      -1.0f, 0.0f, 0.0f,// bottom left 
   -0.5f, -0.5f,    0.5f,      1.0f, 0.0f,      -1.0f, 0.0f, 0.0f,// bottom right
   -0.5f,  0.5f,    0.5f,      1.0f, 1.0f,      -1.0f, 0.0f, 0.0f,// top right
   -0.5f,  0.5f,   -0.5f,      0.0f, 1.0f,      -1.0f, 0.0f, 0.0f,// top left

   // Right face
    0.5f, -0.5f,   -0.5f,      0.0f, 0.0f,      1.0f, 0.0f, 0.0f,// bottom left 
    0.5f, -0.5f,    0.5f,      1.0f, 0.0f,      1.0f, 0.0f, 0.0f,// bottom right
    0.5f,  0.5f,    0.5f,      1.0f, 1.0f,      1.0f, 0.0f, 0.0f,// top right
    0.5f,  0.5f,   -0.5f,      0.0f, 1.0f,      1.0f, 0.0f, 0.0f,// top left

   //Bottom face
   -0.5f,  -0.5f,   -0.5f,     0.0f, 0.0f,      0.0f, -1.0f, 0.0f,// bottom left 
    0.5f,  -0.5f,   -0.5f,     1.0f, 0.0f,      0.0f, -1.0f, 0.0f,// bottom right
    0.5f,  -0.5f,    0.5f,     1.0f, 1.0f,      0.0f, -1.0f, 0.0f,// top right
   -0.5f,  -0.5f,    0.5f,     0.0f, 1.0f,      0.0f, -1.0f, 0.0f,// top left

   //Top face
   -0.5f,   0.5f,   -0.5f,     0.0f, 0.0f,      0.0f, 1.0f, 0.0f,// bottom left 
    0.5f,   0.5f,   -0.5f,     1.0f, 0.0f,      0.0f, 1.0f, 0.0f,// bottom right
    0.5f,   0.5f,    0.5f,     1.0f, 1.0f,      0.0f, 1.0f, 0.0f,// top right
   -0.5f,   0.5f,    0.5f,     0.0f, 1.0f,      0.0f, 1.0f, 0.0f,// top left

   // Back face
   -0.5f, -0.5f,    -0.5f,      0.0f, 0.0f,     0.0f, 0.0f, -1.0f,// bottom left 
    0.5f, -0.5f,    -0.5f,      1.0f, 0.0f,     0.0f, 0.0f, -1.0f,// bottom right
    0.5f,  0.5f,    -0.5f,      1.0f, 1.0f,     0.0f, 0.0f, -1.0f,// top right
   -0.5f,  0.5f,    -0.5f,      0.0f, 1.0f,     0.0f, 0.0f, -1.0f, // top left

   // Front face
   -0.5f, -0.5f,    0.5f,      0.0f, 0.0f,     0.0f, 0.0f, 1.0f,// bottom left 
    0.5f, -0.5f,    0.5f,      1.0f, 0.0f,     0.0f, 0.0f, 1.0f,// bottom right
    0.5f,  0.5f,    0.5f,      1.0f, 1.0f,     0.0f, 0.0f, 1.0f,// top right
   -0.5f,  0.5f,    0.5f,      0.0f, 1.0f,     0.0f, 0.0f, 1.0f// top left
};

const unsigned int BlockGeometry::indices[6][6] = { 
        {0,      1,         2,
         2,      3,         0},

        {4,      5,         6,
         6,      7,         4},

        {8,      9,         10,
         10,     11,        8},

        {12,      13,       14,
         14,      15,       12},

        {16,      17,       18,
         18,      19,       16},

        {20,      21,       22,
         22,     23,        20},
};