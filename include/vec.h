#ifndef VEC_H
#define VEC_H

#include <vector>

class vec3
{
public:
    double x, y, z;
    vec3(double x = 0, double y = 0, double z = 0);
    vec3(std::vector<double> v);

    vec3 operator-() const;
    vec3 operator-(const vec3 &other) const;
    vec3 operator+(const vec3 &other) const;
    vec3 operator*(double scalar) const;
    vec3 operator/(double scalar) const;

    double dot(const vec3 &other) const;
    vec3 cross(const vec3 &other) const;
    double norm() const;
    vec3 unit() const;
};

class vec2
{
public:
    double x, y;
    vec2(double x = 0, double y = 0);
    vec2(std::vector<double> v);
    vec2 operator-() const;
    vec2 operator-(const vec2 &other) const;
    vec2 operator+(const vec2 &other) const;
    vec2 operator*(double scalar) const;
};

#endif