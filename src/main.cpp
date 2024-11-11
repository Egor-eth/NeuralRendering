#include "nbvh.h"
#include "Image2d.h"
#include <filesystem>
#include <iostream>

int main(int argc, const char** argv)
{
    uint32_t WIDTH  = 1000;
    uint32_t HEIGHT = 1000;

    const char* scenePath    = argv[1];
    //const char* meshPath     = "scenes/meshes/bunny.vsgf";
    const char* refImage = "pic_ref.bmp";
    const char* outImage = "pic_out.bmp";

    LiteImage::Image2D<uint32_t> image(WIDTH, HEIGHT);
    LiteImage::Image2D<float> depth_map(WIDTH, HEIGHT);
    std::shared_ptr<N_BVH> pRender = std::make_shared<N_BVH>();

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
    pRender->Render(image.data(), depth_map.data(), WIDTH, HEIGHT, "color", 1); 
    std::cout << std::endl;

    std::cout << "[main]: save image to file ..." << std::endl;
    LiteImage::SaveImage(refImage, image);

    std::vector<float> train_input, train_output;
    train_input.resize(2 * WIDTH * HEIGHT);
    train_output.resize(WIDTH * HEIGHT);
    for (int i=0; i<HEIGHT; i++)
    {
        for (int j=0; j<WIDTH; j++)
        {
            train_input[2*(i*WIDTH + j) + 0] = 2*(float)j/WIDTH-1;
            train_input[2*(i*WIDTH + j) + 1] = 2*(float)i/HEIGHT-1;
            train_output[i*WIDTH + j] = depth_map.data()[i*WIDTH + j];
            if (train_output[i*WIDTH + j] > 1000.f)
                train_output[i*WIDTH + j] = 0.0f;
            //std::cout << train_input[2*(i*WIDTH + j) + 0] << " " << train_input[2*(i*WIDTH + j) + 1] << " " << train_output[i*WIDTH + j] << std::endl;
        }
    }

    pRender->SetNetwork();
    pRender->TrainNetwork(train_input, train_output);

    auto test_input = train_input;
    std::vector<float> test_output;
    test_output.resize(WIDTH * HEIGHT);

    pRender->InferenceNetwork(test_input, test_output);

    LiteImage::Image2D<uint32_t> test_image(WIDTH, HEIGHT);

    for (int i=0; i<HEIGHT; i++)
    {
        for (int j=0; j<WIDTH; j++)
        {
            test_image.data()[i*WIDTH + j] = uint32_t(test_output[i*WIDTH + j] * 10) % 256;//uint32_t((1.f / (test_output[i*WIDTH + j] + 1)) * 255.f * 4);
            //std::cout << test_output[i*WIDTH + j] << std::endl;
        }
    }

    std::cout << "[main]: save image to file ..." << std::endl;
    LiteImage::SaveImage(outImage, test_image);

    //std::cout << "Current path is " << std::filesystem::current_path() << std::endl;
}
