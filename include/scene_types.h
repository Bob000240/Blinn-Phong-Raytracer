#ifndef SCENE_TYPES_H
#define SCENE_TYPES_H

#include <vector>
#include "vec.h"
#include "color.h"
#define M_PI 3.14159265358979323846

struct Ray
{
    vec3 orig, direc;
};

struct Light
{
    double x, y, z;
    int w;
    double intensity;
};

struct Material
{
    Color diffuse{1, 1, 1};
    Color specular{1, 1, 1};
    double ka = 1, kd = 1, ks = 1;
    int shininess = 0;
    int textureId = -1;
    double alpha = 0;
    double eta = 1;
};

struct HitRecord
{
    double t = 0;
    vec3 hitPoint, N;
    Color Od, Os;
    double ka = 0, kd = 0, ks = 0;
    int shininess = 0;
    double alpha = 1;
    double eta = 1;
};

struct Texture
{
    int width = 0, height = 0;
    std::vector<Color> pixels;
    bool valid() const { return width > 0 && height > 0 && !pixels.empty(); }
};

struct sphere
{
    double cx, cy, cz;
    double radius;
    Material mat;
};

struct cone
{
    double cx, cy, cz;
    double dx, dy, dz;
    double angle;
    double height;
    Material mat;
};

struct cylinder
{
    double cx, cy, cz;
    double dx, dy, dz;
    double radius;
    double length;
    Material mat;
};

struct triangle
{
    vec3 v0, v1, v2;
    vec3 n0, n1, n2;
    vec2 t0, t1, t2;
    Material mat;
};

#endif
