#ifndef SCENE_TYPES_H
#define SCENE_TYPES_H

#include <vector>
#include "vec.h"
#include "color.h"
#define M_PI 3.14159265358979323846
struct Ray
{
    vec3 orig, direc; // r(t) = orig + t * direc
};

struct Light
{
    double x, y, z;
    int w;
    double intensity;
};

struct sphere
{
    double cx, cy, cz;
    double radius;
    Color diffuseLight;
    Color specularLight;
    std::vector<double> coefficients;
    int shininess;
    int textureId = -1;
    double alpha = 0;
    double eta = 1;
};

struct ellipsoid
{
    double cx, cy, cz;
    double rx, ry, rz;
    Color diffuseLight{1, 1, 1};
    Color specularLight{1, 1, 1};
    std::vector<double> coefficients{1, 1, 1};
    int shininess = 0;
    int textureId = -1;
    double alpha = 0;
    double eta = 1;
};

struct cone
{
    double cx, cy, cz;
    double dx, dy, dz;
    double angle;
    double height;
    Color diffuseLight{1, 1, 1};
    Color specularLight{1, 1, 1};
    std::vector<double> coefficients{1, 1, 1};
    int shininess = 0;
    int textureId = -1;
    double alpha = 0;
    double eta = 1;
};

struct cylinder
{
    double cx, cy, cz;
    double dx, dy, dz;
    double radius;
    double length;
    Color diffuseLight{1, 1, 1};
    Color specularLight{1, 1, 1};
    std::vector<double> coefficients{1, 1, 1};
    int shininess = 0;
    int textureId = -1;
    double alpha = 0;
    double eta = 1;
};

struct triangle
{
    vec3 v0, v1, v2;
    vec3 n0, n1, n2;
    vec2 t0, t1, t2;

    Color diffuseLight;
    Color specularLight;
    std::vector<double> coefficients;
    int shininess;

    int textureId;
    int bumpId;
    double alpha = 0;
    double eta = 1;
};

#endif