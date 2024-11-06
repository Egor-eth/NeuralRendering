#include "nbvh.h"
#include "Image2d.h"
#include <filesystem>
#include <iostream>

int main(int argc, const char** argv)
{
    uint32_t WIDTH  = 2048;
    uint32_t HEIGHT = 2048;

    const char* scenePath    = argv[1];
    //const char* meshPath     = "scenes/meshes/bunny.vsgf";
    const char* outImageFile = "pic_out.bmp";

    LiteImage::Image2D<uint32_t> image(WIDTH, HEIGHT);
    std::shared_ptr<IRenderer> pRender = std::make_shared<N_BVH>();

    pRender->SetAccelStruct(std::make_shared<BVH2CommonRT>());
    pRender->SetViewport(0,0,WIDTH,HEIGHT);

    std::cout << "[main]: load scene '" << scenePath << "'" << std::endl;

    bool loaded = pRender->LoadScene((std::string(std::filesystem::current_path()) + "/" + std::string(scenePath)).c_str());
    if(!loaded) 
    {
        std::cout << "can't load scene '" << scenePath << "'" << std::endl; 
        return -1;
    }
    
    pRender->Clear(WIDTH, HEIGHT, "color");
    
    std::cout << "[main]: do rendering ..." << std::endl;
    pRender->Render(image.data(), WIDTH, HEIGHT, "color"); 
    std::cout << std::endl;

    std::cout << "[main]: save image to file ..." << std::endl;
    LiteImage::SaveImage(outImageFile, image);

    //std::cout << "Current path is " << std::filesystem::current_path() << std::endl;
}
