# cjcraft

A Minecraft made with Vulkan.

![image](https://github.com/cj2yt11/cjcraft/assets/147161189/92fe5c0c-d9fb-4778-9ea9-2c2dfbac73bd)
![image](https://github.com/cj2yt11/cjcraft/assets/147161189/d5bb9c7b-1fa9-4e39-a40b-27394450329a)

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
