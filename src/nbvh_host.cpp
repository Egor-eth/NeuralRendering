#include "nbvh.h"
#include "render_common.h"
#include "utils.h"
#include "hydraxml.h"
#include "loader_utils/gltf_loader.h"
#include "Timer.h"

using LiteMath::DEG_TO_RAD;

using LiteMath::BBox3f;
using LiteMath::float2;
using LiteMath::float3;
using LiteMath::float4;

using LiteMath::perspectiveMatrix;
using LiteMath::lookAt;
using LiteMath::inverse4x4;

N_BVH::N_BVH() 
{ 
  nn::TensorProcessor::init(nn::TensorProcessor::Backend::GPU);

  m_pAccelStruct = std::make_shared<BVH2CommonRT>();

  int L = 8, T = 8*8*8, F = 8, N_min = 4, N_max = 32;
  int int_size = 64;

  nn.set_batch_size_for_evaluate(2048);
  nn.add_layer(std::make_shared<nn::StackedHashGrid3DLayer>(m_samplesPerRay, L, T, F, N_min, N_max), nn::Initializer::He);
  nn.add_layer(std::make_shared<nn::DenseLayer>(m_samplesPerRay * L * F, int_size), nn::Initializer::Siren);
  // nn.add_layer(std::make_shared<nn::DenseLayer>(m_samplesPerRay * 3, 256), nn::Initializer::Siren);
  nn.add_layer(std::make_shared<nn::ReLULayer>());
  nn.add_layer(std::make_shared<nn::DenseLayer>(int_size, int_size), nn::Initializer::Siren);
  nn.add_layer(std::make_shared<nn::ReLULayer>());
  nn.add_layer(std::make_shared<nn::DenseLayer>(int_size, int_size), nn::Initializer::Siren);
  nn.add_layer(std::make_shared<nn::ReLULayer>());
  nn.add_layer(std::make_shared<nn::DenseLayer>(int_size, int_size), nn::Initializer::Siren);
  nn.add_layer(std::make_shared<nn::ReLULayer>());
  nn.add_layer(std::make_shared<nn::DenseLayer>(int_size,  m_outputSize), nn::Initializer::Siren);
  nn.add_layer(std::make_shared<nn::SigmoidLayer>());

}

void N_BVH::SetViewport(int a_xStart, int a_yStart, int a_width, int a_height)
{
  m_width  = a_width;
  m_height = a_height;
  m_packedXY.resize(m_width*m_height);
}

#if defined(__ANDROID__)
bool N_BVH::LoadScene(const char* a_scenePath, AAssetManager* assetManager)
#else
bool N_BVH::LoadScene(const char* a_scenePath)
#endif
{
  m_pAccelStruct->ClearGeom();

  const std::string& path = a_scenePath;

  auto isHydraScene = gltf_loader::ends_with(path, ".xml");

  if(isHydraScene)
  {
#if defined(__ANDROID__)
    return LoadSceneHydra(path, assetManager);
#else
    return LoadSceneHydra(path);
#endif
  }
  else
  {
#if defined(__ANDROID__)
    return LoadSceneGLTF(path, assetManager);
#else
    return LoadSceneGLTF(path);
#endif
  }

  return false;
}

#ifdef __ANDROID__
bool N_BVH::LoadSceneHydra(const std::string& a_path, AAssetManager* assetManager)
#else
bool N_BVH::LoadSceneHydra(const std::string& a_path)
#endif
{
  hydra_xml::HydraScene scene;
  if(
#if defined(__ANDROID__)
scene.LoadState(assetManager, a_path) < 0
#else
scene.LoadState(a_path) < 0
#endif
          )
    return false;

  for(auto cam : scene.Cameras())
  {
    float aspect   = float(m_width) / float(m_height);
    auto proj      = perspectiveMatrix(cam.fov, aspect, cam.nearPlane, cam.farPlane);
    m_camPos = float3(cam.pos);
    m_camLookAt = float3(cam.lookAt);
    m_camUp = float3(cam.up);
    auto worldView = lookAt(m_camPos, m_camLookAt, m_camUp);

    m_projInv      = inverse4x4(proj);
    m_worldViewInv = inverse4x4(worldView);

    break; // take first cam
  }

  std::vector<uint64_t> trisPerObject;
  trisPerObject.reserve(1000);
  m_totalTris = 0;
  m_pAccelStruct->ClearGeom();

  for(auto meshPath : scene.MeshFiles())
  {
    std::cout << "[LoadScene]: mesh = " << meshPath.c_str() << std::endl;
#if defined(__ANDROID__)
    auto currMesh = cmes4h::LoadMeshFromVSGF(assetManager, meshPath.c_str());
#else
    auto currMesh = cmesh4::LoadMeshFromVSGF(meshPath.c_str());
#endif
    auto geomId   = m_pAccelStruct->AddGeom_Triangles3f((const float*)currMesh.vPos4f.data(), currMesh.vPos4f.size(),
                                                        currMesh.indices.data(), currMesh.indices.size(), BUILD_HIGH, sizeof(float)*4);
    (void)geomId; // silence "unused variable" compiler warnings
    m_totalTris += currMesh.indices.size()/3;
    trisPerObject.push_back(currMesh.indices.size()/3);
  }

  LiteMath::Box4f BBox = {};
  for (uint32_t i = 0; i < m_pAccelStruct->GetGeomNum(); ++i)
  {
    BBox.include(m_pAccelStruct->GetGeomBoxes()[2 * i]);
    BBox.include(m_pAccelStruct->GetGeomBoxes()[2 * i + 1]);
  }
  
  m_sceneBBox = {};
  m_totalTrisVisiable = 0;
  m_pAccelStruct->ClearScene();
  for(auto inst : scene.InstancesGeom())
  {
    m_sceneBBox.include(getInstanceBBox(inst.matrix, BBox));
    m_pAccelStruct->AddInstance(inst.geomId, inst.matrix);
    m_totalTrisVisiable += trisPerObject[inst.geomId];
  }
  m_pAccelStruct->CommitScene(BuildQuality::BUILD_HIGH);

  std::cout << "[HydraXML]: camPos     = (" << m_camPos.x << "," << m_camPos.y << "," << m_camPos.z << ")" << std::endl;
  std::cout << "[HydraXML]: camLookAt  = (" << m_camLookAt.x << "," << m_camLookAt.y << "," << m_camLookAt.z << ")" << std::endl;
  std::cout << "[HydraXML]: camUp      = (" << m_camUp.x << "," << m_camUp.y << "," << m_camUp.z << ")" << std::endl;
  std::cout << "[HydraXML]: scnBoxMin  = (" << m_sceneBBox.boxMin.x << "," << m_sceneBBox.boxMin.y << "," << m_sceneBBox.boxMin.z << ")" << std::endl;
  std::cout << "[HydraXML]: scnBoxMax  = (" << m_sceneBBox.boxMax.x << "," << m_sceneBBox.boxMax.y << "," << m_sceneBBox.boxMax.z << ")" << std::endl;

  return true;
}


struct DataRefs
{
  DataRefs(std::unordered_map<int, std::pair<uint32_t, BBox3f>> &a_loadedMeshesToMeshId, std::vector<uint64_t>& a_trisPerObject, Box4f& a_sceneBBox,
           int& a_gltfCam, float4x4& a_worldViewInv, std::shared_ptr<ISceneObject> a_pAccelStruct, uint64_t& a_totalTris) : 
           m_loadedMeshesToMeshId(a_loadedMeshesToMeshId), m_trisPerObject(a_trisPerObject), m_sceneBBox(a_sceneBBox),
           m_gltfCamId(a_gltfCam), m_worldViewInv(a_worldViewInv), m_pAccelStruct(a_pAccelStruct), m_totalTris(a_totalTris) {}

  
  std::unordered_map<int, std::pair<uint32_t, BBox3f>>& m_loadedMeshesToMeshId;
  std::vector<uint64_t>&                                m_trisPerObject;
  Box4f&                                                m_sceneBBox;

  int&                          m_gltfCamId;
  float4x4&                     m_worldViewInv;
  std::shared_ptr<ISceneObject> m_pAccelStruct;
  uint64_t&                     m_totalTris;
};

static void LoadGLTFNodesRecursive(const tinygltf::Model &a_model, const tinygltf::Node& a_node, const LiteMath::float4x4& a_parentMatrix, 
                                   const DataRefs& a_refs)
{
  auto nodeMatrix = a_parentMatrix * gltf_loader::transformMatrixFromGLTFNode(a_node);

  for (size_t i = 0; i < a_node.children.size(); i++)
  {
    LoadGLTFNodesRecursive(a_model, a_model.nodes[a_node.children[i]], nodeMatrix, a_refs);
  }

  if(a_node.camera > -1 && a_node.camera == a_refs.m_gltfCamId)
  {
    // works only for simple cases ?
    float3 eye = {0, 0, 0};
    float3 center = {0, 0, -1};
    float3 up = {0, 1, 0};
    auto tmp = lookAt(nodeMatrix * eye, nodeMatrix * center, up);
    a_refs.m_worldViewInv = inverse4x4(tmp);
  }
  if(a_node.mesh > -1)
  {
    if(!a_refs.m_loadedMeshesToMeshId.count(a_node.mesh))
    {
      const tinygltf::Mesh mesh = a_model.meshes[a_node.mesh];
      auto simpleMesh = gltf_loader::simpleMeshFromGLTFMesh(a_model, mesh);

      if(simpleMesh.VerticesNum() > 0)
      {
        auto meshId = a_refs.m_pAccelStruct->AddGeom_Triangles3f((const float*)simpleMesh.vPos4f.data(), simpleMesh.vPos4f.size(),
                                                                simpleMesh.indices.data(), simpleMesh.indices.size(),
                                                                BUILD_HIGH, sizeof(float)*4);
        a_refs.m_loadedMeshesToMeshId[a_node.mesh] = {meshId, simpleMesh.bbox};

        std::cout << "Loading mesh # " << meshId << std::endl;

        a_refs.m_totalTris += simpleMesh.indices.size() / 3;
        a_refs.m_trisPerObject.push_back(simpleMesh.indices.size() / 3);
      }
    }
    auto tmp_box = a_refs.m_loadedMeshesToMeshId[a_node.mesh].second;
    Box4f mesh_box {};
    mesh_box.boxMin = to_float4(tmp_box.boxMin, 1.0);
    mesh_box.boxMax = to_float4(tmp_box.boxMax, 1.0);
    Box4f inst_box = {};
    inst_box.boxMin = nodeMatrix * mesh_box.boxMin;
    inst_box.boxMax = nodeMatrix * mesh_box.boxMax;

    a_refs.m_sceneBBox.include(inst_box);
    a_refs.m_pAccelStruct->AddInstance(a_refs.m_loadedMeshesToMeshId[a_node.mesh].first, nodeMatrix);
  }
}


#ifdef __ANDROID__
bool N_BVH::LoadSceneGLTF(const std::string& a_path, AAssetManager* assetManager)
#else
bool N_BVH::LoadSceneGLTF(const std::string& a_path)
#endif
{
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF gltfContext;
  std::string error, warning;

#ifdef __ANDROID__
  tinygltf::asset_manager = assetManager;
#endif

  bool loaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, a_path);

  if(!loaded)
  {
    std::cerr << "Cannot load glTF scene from: " << a_path << std::endl;;
    return false;
  }

  const tinygltf::Scene& scene = gltfModel.scenes[0];

  float aspect   = float(m_width) / float(m_height);
  for(size_t i = 0; i < gltfModel.cameras.size(); ++i)
  {
    auto gltfCam = gltfModel.cameras[i];
    if(gltfCam.type == "perspective")
    {
      auto proj = perspectiveMatrix(gltfCam.perspective.yfov / DEG_TO_RAD, aspect,
                                    gltfCam.perspective.znear, gltfCam.perspective.zfar);
      m_projInv = inverse4x4(proj);

      m_gltfCamId = i;
      break; // take first perspective cam
    }
    else if (gltfCam.type == "orthographic")
    {
      std::cerr << "Orthographic camera not supported!" << std::endl;
    }
  }


  // load and instance geometry
  std::vector<uint64_t> trisPerObject;
  trisPerObject.reserve(1000);

  m_totalTris = 0;
  m_pAccelStruct->ClearGeom();
  m_pAccelStruct->ClearScene();

  m_sceneBBox = {};
  std::unordered_map<int, std::pair<uint32_t, BBox3f>> loaded_meshes_to_meshId;
  for(size_t i = 0; i < scene.nodes.size(); ++i)
  {
    const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
    auto identity = LiteMath::float4x4();
    LoadGLTFNodesRecursive(gltfModel, node, identity, 
                           DataRefs(loaded_meshes_to_meshId, trisPerObject, m_sceneBBox, m_gltfCamId, m_worldViewInv, m_pAccelStruct, m_totalTris));
  }

  // glTF scene can have no cameras specified
  if(m_gltfCamId == -1)
  {
    std::tie(m_worldViewInv, m_projInv) = gltf_loader::makeCameraFromSceneBBox(m_width, m_height, m_sceneBBox);
  }
  
  const float3 camPos    = m_worldViewInv*float3(0,0,0);
  const float4 camLookAt = LiteMath::normalize(m_worldViewInv.get_col(2));
  std::cout << "[GLTF]: camPos2    = (" << camPos.x << "," << camPos.y << "," << camPos.z << ")" << std::endl;
  std::cout << "[GLTF]: camLookAt2 = (" << camLookAt.x << "," << camLookAt.y << "," << camLookAt.z << ")" << std::endl;
  std::cout << "[GLTF]: scnBoxMin  = (" << m_sceneBBox.boxMin.x << "," << m_sceneBBox.boxMin.y << "," << m_sceneBBox.boxMin.z << ")" << std::endl;
  std::cout << "[GLTF]: scnBoxMax  = (" << m_sceneBBox.boxMax.x << "," << m_sceneBBox.boxMax.y << "," << m_sceneBBox.boxMax.z << ")" << std::endl;


  m_pAccelStruct->CommitScene(BuildQuality::BUILD_HIGH);

  return true;
}


#if defined(__ANDROID__)
bool N_BVH::LoadSingleMesh(const char* a_meshPath, const float* transform4x4ColMajor, AAssetManager * assetManager)
#else
bool N_BVH::LoadSingleMesh(const char* a_meshPath, const float* transform4x4ColMajor)
#endif
{
  m_pAccelStruct->ClearGeom();

  std::cout << "[LoadScene]: mesh = " << a_meshPath << std::endl;
#if defined(__ANDROID__)
  auto currMesh = cmesh4::LoadMeshFromVSGF(assetManager, a_meshPath);
#else
  auto currMesh = cmesh4::LoadMeshFromVSGF(a_meshPath);
#endif
  if(currMesh.TrianglesNum() == 0)
  {
    std::cout << "[LoadScene]: can't load mesh '" << a_meshPath << "'" << std::endl;
    return false;
  }

  auto geomId   = m_pAccelStruct->AddGeom_Triangles3f((const float*)currMesh.vPos4f.data(), currMesh.vPos4f.size(),
                                                      currMesh.indices.data(), currMesh.indices.size(), BUILD_HIGH, sizeof(float)*4);
  float4x4 mtransform;
  if(transform4x4ColMajor != nullptr)
  {
    mtransform.set_col(0, float4(transform4x4ColMajor+0));
    mtransform.set_col(1, float4(transform4x4ColMajor+4));
    mtransform.set_col(2, float4(transform4x4ColMajor+8));
    mtransform.set_col(3, float4(transform4x4ColMajor+12));
  }

  m_pAccelStruct->ClearScene();
  m_pAccelStruct->AddInstance(geomId, mtransform);
  m_pAccelStruct->CommitScene(BuildQuality::BUILD_HIGH);
  
  float3 camPos  = float3(0,0,5);
  float aspect   = float(m_width) / float(m_height);
  auto proj      = perspectiveMatrix(45.0f, aspect, 0.01f, 100.0f);
  auto worldView = lookAt(camPos, float3(0,0,0), float3(0,1,0));
  
  m_projInv      = inverse4x4(proj);
  m_worldViewInv = inverse4x4(worldView);

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////NEURAL//PART//////////////////////////////////////////

void N_BVH::GenRayBBoxDataset(std::vector<float>& inputData, std::vector<float>& outputData, uint32_t points)
{
  inputData.resize(points * m_raysPerPoint * m_samplesPerRay * 3);
  outputData.resize(points * m_raysPerPoint * m_outputSize);

  BBox3f BBox;
  BBox.boxMax = float3(m_sceneBBox.boxMax.x, m_sceneBBox.boxMax.y, m_sceneBBox.boxMax.z);
  BBox.boxMin = float3(m_sceneBBox.boxMin.x, m_sceneBBox.boxMin.y, m_sceneBBox.boxMin.z);
  float3 BBoxSize = BBox.boxMax - BBox.boxMin;
  BBox.boxMax = BBox.boxMax + BBoxSize * m_BBoxBound;
  BBox.boxMin = BBox.boxMin - BBoxSize * m_BBoxBound;
  BBoxSize = BBox.boxMax - BBox.boxMin;

  for (uint32_t i = 0; i < points; ++i)
  {
    float3 point1 = sampleUniformBBox(BBox);
    float3 point2 = sampleUniformBBox(BBox);
    float3 hitPoint;
    bool hitFlag = false;

    for (uint32_t r = 0; r < m_raysPerPoint; ++r)
    {
      if (hitFlag)
      {
        point1 = point1 + sampleUnitSphere() * length(point1 - hitPoint) * 0.1;
        point2 = hitPoint;
      }
      auto dir = point2 - point1;
      auto hitBBox = BBox.Intersection(point1, 1.f / dir, -INFINITY, +INFINITY);

      auto hitBBoxPoint1 = point1 + dir * hitBBox.t1;
      auto hitBBoxPoint2 = point1 + dir * hitBBox.t2;

      auto rayDir_   = hitBBoxPoint2 - hitBBoxPoint1;
      float4 rayDir  = float4(rayDir_.x, rayDir_.y, rayDir_.z, MAXFLOAT);
      float4 rayOrig = float4(hitBBoxPoint1.x, hitBBoxPoint1.y, hitBBoxPoint1.z, 0.f);

      auto step = rayDir_ / static_cast<float>(m_samplesPerRay + 1);
      for (uint32_t j = 0; j < m_samplesPerRay; ++j)
      {
        float3 sample = hitBBoxPoint1 + step * (j + 1);
        sample = (sample - BBox.boxMin) / BBoxSize;

        //positional_encoding(sample, inputData.data() + (i * m_samplesPerRay + j) * 3 * (ENCODE_LENGTH * 2 + 1));
        inputData[((i * m_raysPerPoint + r) * m_samplesPerRay + j) * 3 + 0] = sample.x;
        inputData[((i * m_raysPerPoint + r) * m_samplesPerRay + j) * 3 + 1] = sample.y;
        inputData[((i * m_raysPerPoint + r) * m_samplesPerRay + j) * 3 + 2] = sample.z;
      }

      auto hitObj   = m_pAccelStruct->RayQuery_NearestHit(rayOrig, rayDir);

      if (hitObj.primId != uint32_t(-1))
      {
        hitPoint = (hitBBoxPoint1 + rayDir_ * hitObj.t - BBox.boxMin) / BBoxSize;
        hitFlag = true;
        // visibility
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 0] = 1.f;

        // surface position (depth)

        // local depth approach:
        //float k = static_cast<float>(m_samplesPerRay);
        //outputData[i * m_outputSize + 1] = (hitObj.t * (k + 1.f) - 1.f) / (k - 1.f); // distance related to sampled points

        // global coords approach:
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 1] = hitPoint.x;
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 2] = hitPoint.y;
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 3] = hitPoint.z;

        // surface normal

        uint32_t normalPacked = *reinterpret_cast<uint32_t*>(&hitObj.coords[2]);
        float3 normal = (unpackNormal(normalPacked) + 1.f) * 0.5f;

        //std::cout << normal.x << " " << normal.y << " " << normal.z << std::endl;

        outputData[(i * m_raysPerPoint + r) * m_outputSize + 4] = normal.x;
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 5] = normal.y;
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 6] = normal.z;
      }
      else
      {
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 0] = 0.f;
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 1] = 0.f;
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 2] = 0.f;
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 3] = 0.f;
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 4] = 0.f;
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 5] = 0.f;
        outputData[(i * m_raysPerPoint + r) * m_outputSize + 6] = 0.f;
      }
    }

  }
}

void N_BVH::TrainNetwork(std::vector<float>& inputData, std::vector<float>& outputData)
{
  nn::TrainStatistics stats;
  nn.set_trainer(5000, nn::OptimizerAdam(0.003f), nn::Loss::NBVH);
  nn.continue_train(inputData.data(), outputData.data(), &stats, inputData.size() / (m_samplesPerRay * 3), 5000, 2, false, nn::OptimizerAdam(0.003f), nn::Loss::NBVH, nn::Metric::Accuracy, true);
  std::cout << "Resulting loss: " << stats.avg_loss << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

void N_BVH::Render(uint32_t* a_outColor, float* out_depth, uint32_t a_width, uint32_t a_height, const char* a_what, int a_passNum)
{
  CastRaySingleBlock(a_width*a_height, a_outColor, out_depth, a_passNum);
}

void N_BVH::Render(uint32_t* a_outColor, uint32_t a_width, uint32_t a_height, const char* a_what, int a_passNum)
{
  profiling::Timer timer;
  timer.restart();

  std::vector<float> nn_input, nn_output, bboxMask;
  nn_input.resize(a_width * a_height * m_samplesPerRay * 3);
  nn_output.resize(a_width * a_height * m_outputSize);
  bboxMask.resize(a_width * a_height);

  BBox3f BBox;
  BBox.boxMax = float3(m_sceneBBox.boxMax.x, m_sceneBBox.boxMax.y, m_sceneBBox.boxMax.z);
  BBox.boxMin = float3(m_sceneBBox.boxMin.x, m_sceneBBox.boxMin.y, m_sceneBBox.boxMin.z);
  float3 BBoxSize = BBox.boxMax - BBox.boxMin;
  BBox.boxMax = BBox.boxMax + BBoxSize * m_BBoxBound;
  BBox.boxMin = BBox.boxMin - BBoxSize * m_BBoxBound;

  float threshold = min(min(BBoxSize.x, BBoxSize.y), BBoxSize.z) * m_BBoxBound;
  BBoxSize = BBox.boxMax - BBox.boxMin;

  for (int i=0; i<a_height; i++)
  {
    for (int j=0; j<a_width; j++)
    {
      float3 initRayDir = EyeRayDirNormalized((float(j)+0.5f)/float(m_width), (float(i)+0.5f)/float(m_height), m_projInv);
      float3 initRayPos = float3(0.f, 0.f, 0.f);

      transform_ray3f(m_worldViewInv, 
                  &initRayPos, &initRayDir);
      auto hitBBox = BBox.Intersection(initRayPos, 1.f / initRayDir, -INFINITY, +INFINITY);

      if (hitBBox.t1 < hitBBox.t2 && length(initRayDir * (hitBBox.t2 - hitBBox.t1)) > threshold)
      {
        float3 hitBBoxPoint1, hitBBoxPoint2;
        hitBBoxPoint1 = initRayPos + initRayDir * hitBBox.t1;
        hitBBoxPoint2 = initRayPos + initRayDir * hitBBox.t2;

        auto step = (hitBBoxPoint2 - hitBBoxPoint1) / static_cast<float>(m_samplesPerRay + 1);
        for (uint32_t k = 0; k < m_samplesPerRay; ++k)
        {
          auto sample = hitBBoxPoint1 + step * (k + 1);
          sample = (sample - BBox.boxMin) / BBoxSize;
          //std::cout << sample.x << " " << sample.y << " " << sample.z << std::endl;

          //positional_encoding(sample, nn_input.data() + ((i * a_width + j) * m_samplesPerRay + k) * 3 * (ENCODE_LENGTH * 2 + 1));
          nn_input[((i * a_width + j) * m_samplesPerRay + k) * 3 + 0] = sample.x;
          nn_input[((i * a_width + j) * m_samplesPerRay + k) * 3 + 1] = sample.y;
          nn_input[((i * a_width + j) * m_samplesPerRay + k) * 3 + 2] = sample.z;
        }

        bboxMask[i * a_width + j] = 1.f;
      }
      else
      {
        bboxMask[i * a_width + j] = 0.f;
      }
    }
  }

  std::cout << timer.getElapsedTime().asMilliseconds() << " ms for ray generation" << std::endl;
  timer.restart();

  nn.evaluate(nn_input, nn_output);

  std::cout << timer.getElapsedTime().asMilliseconds() << " ms for inference" << std::endl;
  timer.restart();
    
  float3 viewPos = m_camPos;
  mymul4x3(m_worldViewInv, viewPos);

  for (int i=0; i<a_height; i++)
  {
    for (int j=0; j<a_width; j++)
    {
      if (bboxMask[i * a_width + j] > 0.5f)
      {
        bool visibility = nn_output[(i * a_width + j) * m_outputSize] > 0.5f;
        if (visibility)
        {
          float3 hitPoint = float3(nn_output[(i * a_width + j) * m_outputSize + 1],
                                   nn_output[(i * a_width + j) * m_outputSize + 2],
                                   nn_output[(i * a_width + j) * m_outputSize + 3]) * BBoxSize + BBox.boxMin;

          float depth = length(hitPoint - viewPos);

          float3 normal = float3(nn_output[(i * a_width + j) * m_outputSize + 4], 
                                           nn_output[(i * a_width + j) * m_outputSize + 5],
                                           nn_output[(i * a_width + j) * m_outputSize + 6]);
          
          normal = normalize((normal - 0.5f) * 2.f);

          ///uint32_t r = int32_t(255.f - (depth - 3.3f) * 400.f);
          ///uint32_t g = int32_t(255.f - (depth - 3.3f) * 400.f);
          ///uint32_t b = int32_t(255.f - (depth - 3.3f) * 400.f);

          //float3 lambert = m_lightSourcePower * max(0.f, dot(normal, normalize(m_lightSourcePos - hitPoint)));

          //out_color[y * m_width + x] = (hit.primId == 0xFFFFFFFF) ? 0 : m_palette[(hit.primId) % palette_size];
          //uint8_t r = uint8_t(clip(0.f, 255.f, (lambert.x + 0.2f) * 255.f));
          //uint8_t g = uint8_t(clip(0.f, 255.f, (lambert.y + 0.2f) * 255.f));
          //uint8_t b = uint8_t(clip(0.f, 255.f, (lambert.z + 0.2f) * 255.f));

          //uint8_t r = uint8_t(normal.x * 255.f);
          //uint8_t g = uint8_t(normal.y * 255.f);
          //uint8_t b = uint8_t(normal.z * 255.f);
          uint8_t r = uint8_t((normal.x + 1.f) * 0.5f * 255.f);
          uint8_t g = uint8_t((normal.y + 1.f) * 0.5f * 255.f);
          uint8_t b = uint8_t((normal.z + 1.f) * 0.5f * 255.f);

          a_outColor[i*a_width + j] = (r << 8 | g) << 8 | b;
        }
      }
      else
        a_outColor[i*a_width + j] = uint32_t(0u);
      //a_outColor[i*a_width + j] = nn_output[i * a_width + j] > 0.5 ? 0xff : 0u;
    }
  }

  std::cout << timer.getElapsedTime().asMilliseconds() << " ms for rendering" << std::endl;
}

void N_BVH::CastRaySingleBlock(uint32_t tidX, uint32_t * out_color, float* out_depth, uint32_t a_numPasses)
{
  profiling::Timer timer;
  
  #ifndef _DEBUG
  #ifndef ENABLE_METRICS
  #pragma omp parallel for default(shared)
  #endif
  #endif
  for(int i=0;i<tidX;i++)
    CastRaySingle(i, out_color, out_depth);

  timeDataByName["CastRaySingleBlock"] = timer.getElapsedTime().asMilliseconds();
}

const char* N_BVH::Name() const
{
  std::stringstream strout;
  strout << "N_BVH(" << m_pAccelStruct->Name() << ")";
  m_tempName = strout.str();
  return m_tempName.c_str();
}

CustomMetrics N_BVH::GetMetrics() const 
{
  auto traceMetrics = m_pAccelStruct->GetStats();
  CustomMetrics res = {};
  for (int i = 0; i < TREELET_ARR_SIZE; i++) {
    res.ljc_data[i] = traceMetrics.avgLJC[i];
    res.cmc_data[i] = traceMetrics.avgCMC[i];
    res.wss_data[i] = float(traceMetrics.avgWSS[i])/1000.0f;
  }
  res.common_data[0] = traceMetrics.avgNC;
  res.common_data[1] = traceMetrics.avgLC;
  res.common_data[2] = float(m_avgLCV);
  res.common_data[3] = traceMetrics.avgTC;
  res.common_data[4] = traceMetrics.avgBLB;
  res.common_data[5] = traceMetrics.avgSOC;
  res.common_data[6] = traceMetrics.avgSBL;
  res.common_data[7] = 0.0f; // traceMetrics.avgTS;
  res.size_data[0] = traceMetrics.bvhTotalSize;
  res.size_data[1] = traceMetrics.geomTotalSize;
  res.prims_count[0] = m_totalTrisVisiable;
  res.prims_count[1] = m_totalTris;
  return res;
}


void N_BVH::GetExecutionTime(const char* a_funcName, float a_out[4])
{
  auto p = timeDataByName.find(a_funcName);
  if(p == timeDataByName.end())
    return;
  a_out[0] = p->second;
}