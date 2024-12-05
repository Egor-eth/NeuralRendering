#pragma once 

#include "LiteMath.h"
#include <cmath>
#include <cstdlib>

using LiteMath::float3;
using LiteMath::float4;

const uint32_t ENCODE_LENGTH = 8;
	
template<class T>
T clip(T a, T b, T value)
{
    if (value < a) return a;
    if (value > b) return b;
    return value;
}

float3 sampleUniformBBox(LiteMath::BBox3f BBox);

float3 sampleUnitSphere();

void positional_encoding(float3 pos, float* res);

LiteMath::Box4f getInstanceBBox(LiteMath::float4x4 transform, LiteMath::Box4f origBBox);

uint32_t packNormal(const float3& normal);

float3 unpackNormal(uint32_t packed);