#include "nbvh.h"
#include "render_common.h"
#include "math_module.h"

void N_BVH::CastRaySingle(uint32_t tidX, uint32_t* out_color, float* out_depth)
{
  float4 rayPosAndNear, rayDirAndFar;
  kernel_InitEyeRay(tidX, &rayPosAndNear, &rayDirAndFar);
  kernel_RayTrace  (tidX, &rayPosAndNear, &rayDirAndFar, out_color, out_depth);
}

void N_BVH::kernel_InitEyeRay(uint32_t tidX, float4* rayPosAndNear, float4* rayDirAndFar)
{
  const uint XY = m_packedXY[tidX];
  const uint x  = (XY & 0x0000FFFF);
  const uint y  = (XY & 0xFFFF0000) >> 16;

  float3 rayDir = EyeRayDirNormalized((float(x)+0.5f)/float(m_width), (float(y)+0.5f)/float(m_height), m_projInv);
  float3 rayPos = float3(0,0,0);

  transform_ray3f(m_worldViewInv, 
                  &rayPos, &rayDir);
  
  *rayPosAndNear = to_float4(rayPos, 0.0f);
  *rayDirAndFar  = to_float4(rayDir, MAXFLOAT);
}

void N_BVH::kernel_RayTrace(uint32_t tidX, const float4* rayPosAndNear,
                                   const float4* rayDirAndFar, uint32_t* out_color, float* out_depth)
{
  const float4 rayPos = *rayPosAndNear;
  const float4 rayDir = *rayDirAndFar ;
  
  if(m_measureOverhead == 0)
  {
    CRT_Hit hit   = m_pAccelStruct->RayQuery_NearestHit(rayPos, rayDir);
    const uint XY = m_packedXY[tidX];
    const uint x  = (XY & 0x0000FFFF);
    const uint y  = (XY & 0xFFFF0000) >> 16;

    if (hit.primId != uint32_t(-1))
    {
      uint32_t normalPacked = *reinterpret_cast<uint32_t*>(&hit.coords[2]);
      float3 normal = unpackNormal(normalPacked);

      //out_color[y * m_width + x] = (hit.primId == 0xFFFFFFFF) ? 0 : m_palette[(hit.primId) % palette_size];
      uint8_t r = uint8_t((normal.x + 1.f) * 0.5f * 255.f);
      uint8_t g = uint8_t((normal.y + 1.f) * 0.5f * 255.f);
      uint8_t b = uint8_t((normal.z + 1.f) * 0.5f * 255.f);
      out_color[y * m_width + x] = (r << 8 | g) << 8 | b;
      out_depth[y * m_width + x] = hit.t;
    }
    else
    {
      out_color[y * m_width + x] = 0u;
      out_depth[y * m_width + x] = INF_POSITIVE;
    }

  }
  else
  {
    float4 colorF = abs(rayDir);
    const uint XY = m_packedXY[tidX];
    const uint x  = (XY & 0x0000FFFF);
    const uint y  = (XY & 0xFFFF0000) >> 16;
    out_color[y * m_width + x] = RealColorToUint32(colorF);
    out_depth[y * m_width + x] = 0.f;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline uint BlockIndex2D(uint tidX, uint tidY, uint a_width)
{
  const uint inBlockIdX = tidX % 4; // 4x4 blocks
  const uint inBlockIdY = tidY % 4; // 4x4 blocks
 
  const uint localIndex = inBlockIdY*4 + inBlockIdX;
  const uint wBlocks    = a_width/4;

  const uint blockX     = tidX/4;
  const uint blockY     = tidY/4;
  const uint offset     = (blockX + blockY*wBlocks)*4*4 + localIndex;
  return offset;
}

static inline uint SuperBlockIndex2D(uint tidX, uint tidY, uint a_width)
{
  const uint inBlockIdX = tidX % 4; // 4x4 blocks
  const uint inBlockIdY = tidY % 4; // 4x4 blocks
  const uint localIndex = inBlockIdY*4 + inBlockIdX;
  const uint wBlocks    = a_width/4;
  const uint blockX     = tidX/4;
  const uint blockY     = tidY/4;
  
  const uint inHBlockIdX = blockX % 2; // 2x2 SuperBlocks
  const uint inHBlockIdY = blockY % 2; // 2x2 SuperBlocks
  const uint localIndexH = inHBlockIdY*2 + inHBlockIdX;
  const uint wBlocksH    = wBlocks/2;
  const uint blockHX     = blockX/2;
  const uint blockHY     = blockY/2;

  return (blockHX + blockHY*wBlocksH)*8*8 + localIndexH*4*4 + localIndex;
}

static inline uint SuperBlockIndex2DOpt(uint tidX, uint tidY, uint a_width)
{
  const uint inBlockIdX = tidX & 0x00000003; // 4x4 blocks
  const uint inBlockIdY = tidY & 0x00000003; // 4x4 blocks
  const uint localIndex = inBlockIdY*4 + inBlockIdX;
  const uint wBlocks    = a_width >> 2;
  const uint blockX     = tidX    >> 2;
  const uint blockY     = tidY    >> 2;
  
  const uint inHBlockIdX = blockX & 0x00000001; // 2x2 SuperBlocks
  const uint inHBlockIdY = blockY & 0x00000001; // 2x2 SuperBlocks
  const uint localIndexH = inHBlockIdY*2 + inHBlockIdX;
  const uint wBlocksH    = wBlocks >> 1;
  const uint blockHX     = blockX  >> 1;
  const uint blockHY     = blockY  >> 1;

  return (blockHX + blockHY*wBlocksH)*64 + localIndexH*16 + localIndex;
}

void N_BVH::kernel_PackXY(uint tidX, uint tidY, uint* out_pakedXY)
{
  //const uint offset   = BlockIndex2D(tidX, tidY, m_width);
  const uint offset   = SuperBlockIndex2DOpt(tidX, tidY, m_width);
  out_pakedXY[offset] = ((tidY << 16) & 0xFFFF0000) | (tidX & 0x0000FFFF);
}

void N_BVH::PackXY(uint tidX, uint tidY)
{
  kernel_PackXY(tidX, tidY, m_packedXY.data());
}

void N_BVH::PackXYBlock(uint tidX, uint tidY, uint a_passNum)
{
  #pragma omp parallel for default(shared)
  for(int y=0;y<tidY;y++)
    for(int x=0;x<tidX;x++)
      PackXY(x, y);
}

void N_BVH::Clear(uint32_t a_width, uint32_t a_height, const char* a_what)
{
  PackXYBlock(a_width, a_height, 1);
}

void N_BVH::SetPresets(const RenderPreset& a_presets)
{
  m_presets = a_presets;
  if(a_presets.measureOverhead)
    m_measureOverhead = 1;
  else
    m_measureOverhead = 0;
}