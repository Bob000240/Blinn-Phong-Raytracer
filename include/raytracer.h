#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <string>
#include "data.h"

class RayTracer
{
private:
    Data data;

    // camera frame
    vec3 u, v, w;
    vec3 n;

    // image window params
    int imgW = 0, imgH = 0;
    double aspect = 0;

    double thetaH = 0;
    double thetaW = 0;
    double d = 0;
    double viewingWindowH = 0;
    double viewingWindowW = 0;

    vec3 upperLeft, upperRight, lowerLeft, lowerRight;
    vec3 delta_h, delta_v;
    vec3 delta_c_h, delta_c_v;

    vec3 viewingRay(int i, int j) const;

    void sphereHelper(const sphere &s, const Ray &ray, double t,
                      vec3 &hitPoint, vec3 &N,
                      Color &Od, Color &Os,
                      double &ka, double &kd, double &ks,
                      int &shininess) const;
    void coneHelper(const cone &c, const Ray &ray, double t,
                    vec3 &hitPoint, vec3 &N,
                    Color &Od, Color &Os,
                    double &ka, double &kd, double &ks,
                    int &shininess) const;
    void cylinderHelper(const cylinder &c, const Ray &ray, double t,
                        vec3 &hitPoint, vec3 &N,
                        Color &Od, Color &Os,
                        double &ka, double &kd, double &ks,
                        int &shininess) const;

    void triangleHelper(const triangle &tri, const Ray &ray,
                        double tHit, double a, double b, double c,
                        vec3 &hitPoint, vec3 &N,
                        Color &Od, Color &Os,
                        double &ka, double &kd, double &ks,
                        int &shininess) const;

    vec3 reflection(const vec3 &I, const vec3 &N) const;
    double fresnel(const double ni, const double nt, const vec3 &I, const vec3 &N) const;
    vec3 refraction(const vec3 &I, const vec3 &N, double ni, double nt) const;
    Color nearestNeighbor(const Texture &tex, double u, double v) const;
    Color recursiveTrace(const Ray &ray, int depth) const;

public:
    RayTracer(const std::string &filename);
    double shadowFactor(const vec3 &hitPoint, const Light &light, const vec3 &N) const;
    Color blinnPhong(
        const vec3 &hitPoint,
        const vec3 &N,
        const vec3 &V,
        const Color &Od,
        const Color &Os,
        double ka,
        double kd,
        double ks,
        int shininess) const;
    void renderShapes();
};

#endif