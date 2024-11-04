#include "nbvh.h"
#include <filesystem>
#include <iostream>

int main()
{
    N_BVH nbvh;
    nbvh.SetAccelStruct(std::make_shared<BVH2CommonRT>());
    nbvh.LoadSceneGLTF(std::string(std::filesystem::current_path()) + "/resources/scenes/01_Bunny/01_Bunny.gltf");
    std::cout << "Current path is " << std::filesystem::current_path() << std::endl;
}
