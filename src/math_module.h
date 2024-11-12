#pragma once 

template<class T>
T clip(T a, T b, T value)
{
    if (value < a) return a;
    if (value > b) return b;
    return value;
}