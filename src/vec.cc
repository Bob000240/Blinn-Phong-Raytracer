#include "vec.h"
#include <cmath>

vec3::vec3(double x, double y, double z) : x(x), y(y), z(z) {}
vec3::vec3(std::vector<double> v) : x(v[0]), y(v[1]), z(v[2]) {}

vec3 vec3::operator-() const { return vec3(-x, -y, -z); }
vec3 vec3::operator-(const vec3 &other) const { return vec3(x - other.x, y - other.y, z - other.z); }
vec3 vec3::operator+(const vec3 &other) const { return vec3(x + other.x, y + other.y, z + other.z); }
vec3 vec3::operator*(double scalar) const { return vec3(x * scalar, y * scalar, z * scalar); }
vec3 vec3::operator/(double scalar) const { return vec3(x / scalar, y / scalar, z / scalar); }

double vec3::dot(const vec3 &other) const { return x * other.x + y * other.y + z * other.z; }

vec3 vec3::cross(const vec3 &other) const
{
    return vec3(y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x);
}

double vec3::norm() const { return std::sqrt(x * x + y * y + z * z); }

vec3 vec3::unit() const
{
    double n = norm();
    if (n == 0)
        return vec3(0, 0, 0);
    return vec3(x / n, y / n, z / n);
}

vec2::vec2(double x, double y) : x(x), y(y) {}
vec2::vec2(std::vector<double> v) : x(v[0]), y(v[1]) {}
vec2 vec2::operator-() const { return vec2(-x, -y); }
vec2 vec2::operator-(const vec2 &other) const { return vec2(x - other.x, y - other.y); }
vec2 vec2::operator+(const vec2 &other) const { return vec2(x + other.x, y + other.y); }
vec2 vec2::operator*(double scalar) const { return vec2(x * scalar, y * scalar); }