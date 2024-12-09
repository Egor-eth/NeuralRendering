#include "bvh_tree.h"
#include "gltf_loader.h"
#include "IRenderer.h"
#include "neural_core/src/neural_network.h"

#include <string>
#include <memory>

class N_BVH
{
public:
    N_BVH();
  const char* Name() const;
  
  virtual void SceneRestrictions(uint32_t a_restrictions[4]) const
  {
    uint32_t maxMeshes            = 1024;
    uint32_t maxTotalVertices     = 8'000'000;
    uint32_t maxTotalPrimitives   = 8'000'000;
    uint32_t maxPrimitivesPerMesh = 4'000'000;

    a_restrictions[0] = maxMeshes;
    a_restrictions[1] = maxTotalVertices;
    a_restrictions[2] = maxTotalPrimitives;
    a_restrictions[3] = maxPrimitivesPerMesh;
  }

#ifdef __ANDROID__
  virtual bool LoadScene(const char* a_scenePath, AAssetManager* assetManager = nullptr);
#else
  virtual bool LoadScene(const char* a_scenePath);
#endif

#ifdef __ANDROID__
  bool LoadSingleMesh(const char* a_meshPath, const float* transform4x4ColMajor, AAssetManager* assetManager = nullptr);
#else
  bool LoadSingleMesh(const char* a_meshPath, const float* transform4x4ColMajor);
#endif

  //////////////////////////////NEURAL//PART/////////////////////////////////////

  void GenRayBBoxDataset(std::vector<float>& inputData, std::vector<float>& outputData, uint32_t points);
  void TrainNetwork(std::vector<float>& inputData, std::vector<float>& outputData);

  ///////////////////////////////////////////////////////////////////////////////

  void Clear (uint32_t a_width, uint32_t a_height, const char* a_what);
  void Render(uint32_t* imageData, uint32_t a_width, uint32_t a_height, const char* a_what, int a_passNum);
  void Render(uint32_t* imageData, float* depthData, uint32_t a_width, uint32_t a_height, const char* a_what, int a_passNum);
  void SetViewport(int a_xStart, int a_yStart, int a_width, int a_height);

  void SetAccelStruct(std::shared_ptr<ISceneObject> a_customAccelStruct) {};
  std::shared_ptr<ISceneObject> GetAccelStruct() { return m_pAccelStruct; }

  void GetExecutionTime(const char* a_funcName, float a_out[4]);
  
  #ifndef KERNEL_SLICER
  CustomMetrics GetMetrics() const;
  #endif

  void CommitDeviceData() {}
  
protected:

#ifdef __ANDROID__
  bool LoadSceneHydra(const std::string& a_path, AAssetManager* assetManager = nullptr);
  bool LoadSceneGLTF(const std::string& a_path, AAssetManager* assetManager = nullptr);
#else
  bool LoadSceneHydra(const std::string& a_path);
  bool LoadSceneGLTF(const std::string& a_path);
#endif

  virtual void PackXYBlock(uint tidX, uint tidY, uint a_passNum);
  virtual void PackXY(uint tidX, uint tidY);
  virtual void kernel_PackXY(uint tidX, uint tidY, uint* out_pakedXY);

  #ifdef KERNEL_SLICER
  void CastRaySingle(uint32_t tidX, uint32_t* out_color __attribute__((size("tidX"))));
  #else
  void CastRaySingle(uint32_t tidX, uint32_t* out_color, float* out_depth);
  #endif

  virtual void CastRaySingleBlock(uint32_t tidX, uint32_t* out_color, float* out_depth, uint32_t a_numPasses = 1);

  void kernel_InitEyeRay(uint32_t tidX, LiteMath::float4* rayPosAndNear, LiteMath::float4* rayDirAndFar);
  void kernel_RayTrace(uint32_t tidX, const LiteMath::float4* rayPosAndNear, const LiteMath::float4* rayDirAndFar, uint32_t* out_color, float* out_depth);

  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_measureOverhead = 0;

  nn::NeuralNetwork nn;
  uint32_t m_samplesPerRay = 2;
  uint32_t m_raysPerPoint = 1;
  uint32_t m_outputSize = 7; // visibility (1) + surface (3) + normal (3)
  LiteMath::Box4f m_sceneBBox = {};
  float m_BBoxBound = 0.2;
  float3 m_lightSourcePos = float3(1.f, 1.f, 1.f);
  float3 m_lightSourcePower = float3(1.f, 1.f, 1.f);

  LiteMath::float3 m_camPos, m_camLookAt, m_camUp;
  int m_gltfCamId = -1;
  LiteMath::float4x4 m_projInv;
  LiteMath::float4x4 m_worldViewInv;

  std::shared_ptr<BVH2CommonRT> m_pAccelStruct; 
  //std::shared_ptr<ISceneObject> m_pAccelStruct;
  std::vector<uint32_t>         m_packedXY;

  // color palette to select color for objects based on mesh/instance id
  static constexpr uint32_t palette_size = 20;
  static constexpr uint32_t m_palette[palette_size] = {
    0xffe6194b, 0xff3cb44b, 0xffffe119, 0xff0082c8,
    0xfff58231, 0xff911eb4, 0xff46f0f0, 0xfff032e6,
    0xffd2f53c, 0xfffabebe, 0xff008080, 0xffe6beff,
    0xffaa6e28, 0xfffffac8, 0xff800000, 0xffaaffc3,
    0xff808000, 0xffffd8b1, 0xff000080, 0xff808080
  };

  static constexpr uint32_t BSIZE =8;

  std::unordered_map<std::string, float> timeDataByName;
  mutable std::string m_tempName;
  double m_avgLCV;

  uint64_t m_totalTris         = 0;
  uint64_t m_totalTrisVisiable = 0;

  uint32_t GetGeomNum() const { return m_pAccelStruct->GetGeomNum(); };
  uint32_t GetInstNum() const { return m_pAccelStruct->GetInstNum(); };
  const LiteMath::float4* GetGeomBoxes() const  { return m_pAccelStruct->GetGeomBoxes(); };
};
