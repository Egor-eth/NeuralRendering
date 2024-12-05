#include "utils.h"
#include "LiteMath.h"
#include <cmath>
#include <cstdlib>

float3 sampleUniformBBox(LiteMath::BBox3f BBox)
{
    float3 point;
    point.x = BBox.boxMin.x + (BBox.boxMax.x - BBox.boxMin.x) * (std::rand() / static_cast<float>(RAND_MAX));
    point.y = BBox.boxMin.y + (BBox.boxMax.y - BBox.boxMin.y) * (std::rand() / static_cast<float>(RAND_MAX));
    point.z = BBox.boxMin.z + (BBox.boxMax.z - BBox.boxMin.z) * (std::rand() / static_cast<float>(RAND_MAX));
    return point;
}

float3 sampleUnitSphere()
{
    float3 point = {1.f, 1.f, 1.f};
    
    while (dot(point, point) >= 1.f)
    {
        point.x = (std::rand() / static_cast<float>(RAND_MAX) - 0.5f) * 2.f;
        point.y = (std::rand() / static_cast<float>(RAND_MAX) - 0.5f) * 2.f;
        point.z = (std::rand() / static_cast<float>(RAND_MAX) - 0.5f) * 2.f;
    }

    return normalize(point);
}

void positional_encoding(float3 pos, float* res)
{
    uint32_t idx = 0;
    const float FACTOR = 0.01;

    float3 pol[ENCODE_LENGTH] = {pos};
    for (uint32_t i = 1; i < ENCODE_LENGTH; ++i)
    {
        pol[i].x = pol[i - 1].x * 2 * pos.x;
        pol[i].y = pol[i - 1].y * 2 * pos.y;
        pol[i].z = pol[i - 1].z * 2 * pos.z;
    }

    std::copy(pos.M, pos.M + 3, res);

    for (uint32_t i = 0; i < ENCODE_LENGTH; ++i)
    {
        float3 sin_enc = {sinf(pos.x * powf(2, i) * FACTOR), sinf(pos.y * powf(2, i) * FACTOR), sinf(pos.z * powf(2, i) * FACTOR)};
        float3 cos_enc = {cosf(pos.x * powf(2, i) * FACTOR), cosf(pos.y * powf(2, i) * FACTOR), cosf(pos.z * powf(2, i) * FACTOR)};
        std::copy(sin_enc.M, sin_enc.M + 3, res + sizeof(float3) * (1 + i * 2));
        std::copy(cos_enc.M, cos_enc.M + 3, res + sizeof(float3) * (2 + i * 2));
    }
}

LiteMath::Box4f getInstanceBBox(LiteMath::float4x4 transform, LiteMath::Box4f origBBox)
{
    LiteMath::Box4f instanceBBox = {};

    instanceBBox.include(transform * origBBox.boxMin);
    instanceBBox.include(transform * float4(origBBox.boxMin.x, origBBox.boxMin.y, origBBox.boxMax.z, 1.f));
    instanceBBox.include(transform * float4(origBBox.boxMin.x, origBBox.boxMax.y, origBBox.boxMin.z, 1.f));
    instanceBBox.include(transform * float4(origBBox.boxMin.x, origBBox.boxMax.y, origBBox.boxMax.z, 1.f));
    instanceBBox.include(transform * float4(origBBox.boxMax.x, origBBox.boxMin.y, origBBox.boxMin.z, 1.f));
    instanceBBox.include(transform * float4(origBBox.boxMax.x, origBBox.boxMin.y, origBBox.boxMax.z, 1.f));
    instanceBBox.include(transform * float4(origBBox.boxMax.x, origBBox.boxMax.y, origBBox.boxMin.z, 1.f));
    instanceBBox.include(transform * origBBox.boxMax);

    return instanceBBox;
}

uint32_t packNormal(const float3& normal) { 
    float3 n = normalize(normal);

    float nx = (n.x + 1.0f) * 0.5f;
    float ny = (n.y + 1.0f) * 0.5f;

    uint16_t x = static_cast<uint16_t>(std::round(nx * 65535.0f));
    uint16_t y = static_cast<uint16_t>(std::round(ny * 65535.0f));

    return (x & 0xFFFF) | ((y & 0xFFFF) << 16);
}

float3 unpackNormal(uint32_t packed) {
    uint16_t x = packed & 0xFFFF;
    uint16_t y = (packed >> 16) & 0xFFFF;

    float nx = static_cast<float>(x) / 65535.0f;
    float ny = static_cast<float>(y) / 65535.0f;

    nx = nx * 2.0f - 1.0f;
    ny = ny * 2.0f - 1.0f;

    float nz = std::sqrt(fabs(1.0f - nx * nx - ny * ny));

    return {nx, ny, nz};
}