# cjcraft

A Minecraft made with Vulkan.
![image](https://github.com/cjt10136/cjcraft/assets/147172598/b5328921-bee7-4a37-bd74-dd990c6a228e)
![image](https://github.com/cjt10136/cjcraft/assets/147172598/cab4a0e1-e6cd-42db-886d-04d7876b7e85)

## Place and Destroy Blocks
https://youtu.be/Ck5vfoPDROs

## Fast Terrain Generation
https://youtu.be/nZvSB0XK5Gk

## Build with CMake
### Requirement
Vulkan SDK

### Build
```
git clone --recursive https://github.com/cjuffgl/cjcraft.git
cd cjcraft
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

### Run
```
./main
```

## Used Libraries
- VulkanHpp: C++ Bindings for Vulkan
- glm: math
- glfw: window management
- FastNoiseLite: for terrain generation
- stb: load texture
- unordered_dense: faster HashMap
- llvm::SmallVector: for small vector optimization 
- ms-gsl: C++ Core Guidelines support libraries
