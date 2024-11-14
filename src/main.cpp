#include "nbvh.h"
#include "Image2d.h"
#include <filesystem>
#include <iostream>

int main(int argc, const char** argv)
{
    uint32_t WIDTH  = 100;
    uint32_t HEIGHT = 100;

    const char* scenePath    = argv[1];
    const char* refImage = "pic_ref.bmp";
    const char* outImage = "pic_out.bmp";

    LiteImage::Image2D<uint32_t> image(WIDTH, HEIGHT);
    LiteImage::Image2D<float> depth_map(WIDTH, HEIGHT);
    std::shared_ptr<N_BVH> pRender = std::make_shared<N_BVH>();

    pRender->SetViewport(0,0,WIDTH,HEIGHT);

    std::cout << "[main]: load scene '" << scenePath << "'" << std::endl;

    bool loaded = pRender->LoadScene((std::string(std::filesystem::current_path()) + "/" + std::string(scenePath)).c_str());
    if(!loaded) 
    {
        std::cout << "can't load scene '" << scenePath << "'" << std::endl; 
        return -1;
    }
    
    pRender->Clear(WIDTH, HEIGHT, "color");
    
    std::cout << "[main]: do reference rendering ..." << std::endl;
    pRender->Render(image.data(), depth_map.data(), WIDTH, HEIGHT, "color", 1); 

    std::cout << "[main]: save image to file ..." << std::endl;
    LiteImage::SaveImage(refImage, image);

    std::cout << "[main]: generate dataset ..." << std::endl;
    std::vector<float> train_input, train_output;
    pRender->GenRayBBoxDataset(train_input, train_output, 100000, 1);

    std::cout << "[main]: do training ..." << std::endl;
    pRender->TrainNetwork(train_input, train_output);

    LiteImage::Image2D<uint32_t> test_image(WIDTH, HEIGHT);
    std::cout << "[main]: do neural rendering ..." << std::endl;
    pRender->Render(test_image.data(), WIDTH, HEIGHT, "color", 1); 

    std::cout << "[main]: save image to file ..." << std::endl;
    LiteImage::SaveImage(outImage, test_image);
}
