A simple Minecraft clone featuring  
-Procedural generation  
-Player collisions and physics  
-Greedy meshing  
-Block placing/breaking  
-Infinite terrain  
-Basic lighting  
-View frustum culling   
-Level of detail    

I aim to focus on optimizing the memory usage and speed of generating, meshing, and rendering chunks before moving on to real gameplay elements.    
My next small goal is to introduce a menu screen where you can access your saves. My saving technique I have planned is to only store changes made to    
a chunk in a stack. So instead of saving entire chunks, we only save the changes made to the freshly generated chunk. Then, when we want to load the    
chunk again, we simply pop the stack until the chunk is back to its present state. This should be much less expensive to store than saving an entire    
chunk would be and comes with the added benefit of being able to sort of step through the history of the chunk if you wanted to.
I will soon face the daunting task of rendering semi-transparent blocks, something that requires one to use special techniques to get working properly.    
After I get semi-transparent blocks working, I will move on to adding more blocks and designing more interesting procedural generation. I will add 3D noise    
for caves and overpasses, then work on adding trees, then an inventory system, then block collecting, crafting, tools, animations for actions, mobs

Credits  
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
![image](https://github.com/KellenJCole/Optimized-Voxel-World/assets/34790396/3d11db5f-4ab3-4c26-aadd-1a1ac6bd4553)  
![image](https://github.com/KellenJCole/Optimized-Voxel-World/assets/34790396/9e892363-af67-4ca8-8884-9b40fef71ec6)  
![image](https://github.com/KellenJCole/Optimized-Voxel-World/assets/34790396/c296b1ef-4d92-4ea7-bc3d-6f40f8fc1dbb)  
  
Video highly compressed due to limits of GitHub  
https://github.com/KellenJCole/Optimized-Voxel-World/assets/34790396/b1510a41-948c-4f8a-8c77-bc7d50ebb59a

