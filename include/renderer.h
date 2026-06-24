#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include "camera.h"
#include "scene.h"

class Renderer
{
public:
    Renderer(const Camera &camera, const Scene &scene);
    void render(const std::string &outputFile);

private:
    static void *worker(void *arg);
    const Camera &cam;
    const Scene  &scene;

    Color recursiveTrace(const Ray &ray, int depth) const;

    vec3 reflection(const vec3 &I, const vec3 &N) const;
    double fresnel(double ni, double nt, const vec3 &I, const vec3 &N) const;
    vec3 refraction(const vec3 &I, const vec3 &N, double ni, double nt) const;

    double shadowFactor(const vec3 &hitPoint, const Light &light, const vec3 &N) const;
    Color blinnPhong(const vec3 &hitPoint, const vec3 &N, const vec3 &V,
                      const Color &Od, const Color &Os,
                      double ka, double kd, double ks, int shininess) const;
};

#endif
