# A performance-oriented voxel engine.  
  
## Demo       
https://www.youtube.com/watch?v=wc1ZzzBtpKw
  
## Installation  
### Prerequisites  
- Windows 10/11
- Visual Studio 2022 (workload: Desktop development with C++)
- Git
- vcpkg (dependency manager)
### 1) Install vcpkg (one time)  
- Open PowerShell
- git clone https://github.com/microsoft/vcpkg C:\vcpkg
- C:\vcpkg\bootstrap-vcpkg.bat
### 2) Clone this repo  
### 3) Build in Visual Studio  
- Open *Optimized Voxel World.sln* in Visual Studio 2022
- Set *x64* and *Release*
- Build -> Build Solution
### 4) Run  

## Note  
Pressing escape will unlock your cursor and allow you to resize the window.  
  
## Features:  
-Procedural generation  
-Player collisions and physics  
-Greedy meshing  
-Block placing/breaking  
-Infinite terrain  
-Basic lighting  
-View frustum culling   
-Level of detail    
-Vertex pooling
  
## Goal    
Focus is on optimizing memory usage and speed of generating, meshing, and rendering chunks before moving on to real gameplay elements.  
Once I feel performance is efficient enough, and bug-free, I will move on to interesting procedural generation.  

## Controls  
- WASD: move around
- F: toggle polygon fill mode
- G: toggle gravity
- I: toggle FPS/coordinates display
- Up Arrow: increase render distance
- Down Arrow: decrease render distance
- Escape: unlock cursor
- P: Toggle post-processing shader (hides T-junction errors)
- Space: Jump or move up (if gravity is disabled)
- Left Shift: Move down (if gravity is disabled)
- Left click: Place block
- Right click: Break block  
  
## Credits  
Textures used are obtained from Faithful: https://faithfulpack.net  
  
Libraries used:  
GLEW: https://glew.sourceforge.net - for loading OpenGL functions  
GLFW: https://www.glfw.org - for creating a window and handling user input  
GLM: https://github.com/g-truc/glm - for OpenGL maths  
stb_image: https://github.com/nothings/stb/blob/master/stb_image.h - for loading images into textures  
FastNoise: https://github.com/charlesangus/FastNoise - for generating procedural terrain  
FreeType: https://freetype.org - for converting true type fonts to glyph textures  
Dear ImGui https://github.com/ocornut/imgui - for easily changing parameters of procedural terrain generation on the fly  
  
Resources used:  
LearnOpenGL: https://learnopengl.com  
The Cherno's OpenGL YouTube series: https://www.youtube.com/watch?v=W3gAzLwfIP0&list=PLlrATfBNZ98foTJPJ_Ev03o2oq3-GGOS2  
0fps' "Meshing in a Minecraft Game" https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/  
Nicholas Mcdonald's "High Performance Voxel Engine: Vertex Pooling" https://nickmcd.me/2021/04/04/high-performance-voxel-engine/
![image](https://github.com/KellenJCole/Optimized-Voxel-World/assets/34790396/3d11db5f-4ab3-4c26-aadd-1a1ac6bd4553)  
![image](https://github.com/KellenJCole/Optimized-Voxel-World/assets/34790396/9e892363-af67-4ca8-8884-9b40fef71ec6)  
![image](https://github.com/KellenJCole/Optimized-Voxel-World/assets/34790396/c296b1ef-4d92-4ea7-bc3d-6f40f8fc1dbb)  
