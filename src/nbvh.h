#include "BVH2CommonRT.h"
#include "gltf_loader.h"

#include <string>
#include <memory>

class N_BVH
{
public:
    N_BVH();
    ~N_BVH();
    bool LoadSceneGLTF(std::string a_path);
    void SetAccelStruct(std::shared_ptr<ISceneObject> a_customAccelStruct) 
    { 
        m_pAccelStruct = a_customAccelStruct;
    }
protected:
    uint32_t m_width, m_height;
    int m_gltfCamId = -1;
    LiteMath::float4x4 m_projInv;
    LiteMath::float4x4 m_worldViewInv;

    std::shared_ptr<ISceneObject> m_pAccelStruct;
    std::vector<uint32_t>         m_packedXY;

    uint64_t m_totalTris         = 0;
    uint64_t m_totalTrisVisiable = 0;
};
