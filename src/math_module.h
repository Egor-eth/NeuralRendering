#pragma once 

#include "LiteMath.h"
#include <cmath>
#include <cstdlib>
	
template<class T>
T clip(T a, T b, T value)
{
    if (value < a) return a;
    if (value > b) return b;
    return value;
}

float3 sampleUniformBBox(BBox3f BBox)
{
    float3 point;
    point.x = BBox.boxMin.x + (BBox.boxMax.x - BBox.boxMin.x) * (std::rand() / static_cast<float>(RAND_MAX));
    point.y = BBox.boxMin.y + (BBox.boxMax.y - BBox.boxMin.y) * (std::rand() / static_cast<float>(RAND_MAX));
    point.z = BBox.boxMin.z + (BBox.boxMax.z - BBox.boxMin.z) * (std::rand() / static_cast<float>(RAND_MAX));
    return point;
}