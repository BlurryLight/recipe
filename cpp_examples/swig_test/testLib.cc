#include "testLib.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <functional>
#include <numeric>

int Shape::nShapes = 0;

int add(int a,int b)
{
    return a + b;
}

Vec3f Vec3f::operator+(const Vec3f &other) {
      return Vec3f(x + other.x, y + other.y, z + other.z);
}

void Vec3f::Display() { std::printf("Vec3f: x %f y %f z %f\n", x, y, z); }

std::string Vec3f::Text() {

  return "Vec3f: x:" + std::to_string(x) + "y:" + std::to_string(y) +
         "z:" + std::to_string(z);
}

double Circle::area()  {
    return M_PI * r * r;
}


double average(const std::vector<int>& v)
{
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();

}
double averagex(const double* arr,int len)
{
    return std::accumulate(arr, arr+len, 0.0) / len;
}