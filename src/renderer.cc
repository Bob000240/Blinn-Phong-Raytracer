#include "renderer.h"
#include "intersections.h"
#include <cstdio>
#include <cmath>
#include <iostream>
#include <time.h>
#include <vector>
#include <pthread.h>
#include <unistd.h>

Renderer::Renderer(const Camera &camera, const Scene &scene)
    : cam(camera), scene(scene)
{
}

double Renderer::shadowFactor(const vec3 &hitPoint, const Light &light, const vec3 &N) const
{
    vec3 L;
    double maxT;
    const double EPS = 1e-6;

    if (light.w == 1)
    {
        vec3 toLight = vec3(light.x, light.y, light.z) - hitPoint;
        maxT = toLight.norm();
        L    = toLight.unit();
    }
    else
    {
        maxT = 0x3f3f3f3f;
        L    = -vec3(light.x, light.y, light.z).unit();
    }

    Ray shadowRay{hitPoint + N * EPS, L};

    double lightFactor = 1;

    for (const sphere &s : scene.spheres)
    {
        double t;
        if (hitSphere(s, shadowRay, t) && t > EPS && t < maxT - EPS)
        {
            lightFactor *= (1 - s.mat.alpha);
            if (lightFactor <= 0) return 0;
        }
    }

    for (const cone &c : scene.cones)
    {
        double t;
        if (hitCone(c, shadowRay, t) && t > EPS && t < maxT - EPS)
        {
            lightFactor *= (1 - c.mat.alpha);
            if (lightFactor <= 0) return 0;
        }
    }

    for (const cylinder &c : scene.cylinders)
    {
        double t;
        if (hitCylinder(c, shadowRay, t) && t > EPS && t < maxT - EPS)
        {
            lightFactor *= (1 - c.mat.alpha);
            if (lightFactor <= 0) return 0;
        }
    }

    for (const triangle &tri : scene.triangles)
    {
        double t, a, b, c;
        if (hitTriangle(tri, shadowRay, t, a, b, c) && t > EPS && t < maxT - EPS)
        {
            lightFactor *= (1 - tri.mat.alpha);
            if (lightFactor <= 0) return 0;
        }
    }

    return lightFactor;
}

vec3 Renderer::reflection(const vec3 &I, const vec3 &N) const
{
    return N * 2 * I.dot(N) - I;
}

double Renderer::fresnel(double ni, double nt, const vec3 &I, const vec3 &N) const
{
    double cosTheta = std::max(0.0, N.dot(I));
    return std::pow((ni - nt) / (ni + nt), 2) +
           (1 - std::pow((ni - nt) / (ni + nt), 2)) * std::pow(1 - cosTheta, 5);
}

vec3 Renderer::refraction(const vec3 &I, const vec3 &N, double ni, double nt) const
{
    double eta        = ni / nt;
    double cosThetaSqr = 1 - eta * eta * (1 - I.dot(N) * I.dot(N));
    if (cosThetaSqr < 0)
        return vec3(0, 0, 0);
    vec3 C = (-N) * std::sqrt(cosThetaSqr);
    vec3 D = (N * N.dot(I) - I) * (eta);
    return (C + D).unit();
}

Color Renderer::blinnPhong(
    const vec3   &hitPoint,
    const vec3   &N,
    const vec3   &V,
    const Color  &Od,
    const Color  &Os,
    double ka, double kd, double ks,
    int    shininess) const
{
    Color finalColor = Od * ka;

    for (const Light &light : scene.lights)
    {
        double shadow = shadowFactor(hitPoint, light, N);
        if (shadow <= 0)
            continue;

        vec3 L;

        if (light.w == 1)
        {
            vec3 toLight = vec3(light.x, light.y, light.z) - hitPoint;
            L = toLight.unit();
        }
        else
        {
            L = -vec3(light.x, light.y, light.z).unit();
        }

        double ndotl = N.dot(L);
        if (ndotl < 0) ndotl = 0;

        Color diffuse(0, 0, 0);
        Color specular(0, 0, 0);

        if (ndotl > 0)
        {
            diffuse = Od * (kd * ndotl);

            vec3 H = (L + V).unit();
            double ndoth = N.dot(H);
            if (ndoth < 0) ndoth = 0;

            double specPow = (shininess > 0) ? std::pow(ndoth, shininess) : 0;
            specular = Os * (ks * specPow);
        }

        finalColor += (diffuse + specular) * (light.intensity * shadow);
    }

    return finalColor.clamped();
}

Color Renderer::recursiveTrace(const Ray &ray, int depth) const
{
    if (depth <= 0)
        return scene.bkgcolor;

    HitRecord rec;
    if (!scene.intersect(ray, rec))
        return scene.bkgcolor;

    bool entering = ray.direc.dot(rec.N) < 0;
    vec3 normal = entering ? rec.N : -rec.N;
    double ni = entering ? scene.bgdIoRefraction : rec.eta;
    double nt = entering ? rec.eta : scene.bgdIoRefraction;

    Color reflectColor(0, 0, 0);
    Color refractColor(0, 0, 0);

    vec3 I = (-ray.direc).unit();

    if (rec.ks > 0)
    {
        vec3 Rdir = reflection(I, normal).unit();
        Ray reflectRay{rec.hitPoint + normal * 1e-6, Rdir};
        reflectColor = recursiveTrace(reflectRay, depth - 1);
    }

    double F = (rec.alpha < 1) ? fresnel(ni, nt, I, normal) : rec.ks;

    if (rec.alpha < 1)
    {
        vec3 Tdir = refraction(I, normal, ni, nt);
        if (Tdir.norm() > 0)
        {
            Ray refractRay{rec.hitPoint - normal * 1e-6, Tdir.unit()};
            refractColor = recursiveTrace(refractRay, depth - 1);
        }
    }

    vec3 V = (-ray.direc).unit();
    Color localColor = blinnPhong(rec.hitPoint, normal, V,
                                  rec.Od, rec.Os,
                                  rec.ka, rec.kd, rec.ks, rec.shininess);

    return (localColor + reflectColor * F + refractColor * (1 - F)).clamped();
}

struct PixelsBlock {
    int x0, y0, x1, y1;
};

struct BlockWork {
    const Renderer *renderer;
    std::vector<Color> *fb;
    int W, H;
    int totalBlocks;
    PixelsBlock *blocks;
    int nextBlock;
    pthread_mutex_t mutex;
};

void *Renderer::worker(void *arg)
{
    BlockWork *work = (BlockWork *)arg;

    while (1)
    {
        pthread_mutex_lock(&work->mutex);
        int idx = work->nextBlock++;
        pthread_mutex_unlock(&work->mutex);

        if (idx >= work->totalBlocks)
            break;

        PixelsBlock &b = work->blocks[idx];
        for (int j = b.y0; j < b.y1; j++)
            for (int i = b.x0; i < b.x1; i++)
                (*work->fb)[j * work->W + i] =
                    work->renderer->recursiveTrace(work->renderer->cam.generateRay(i, j), 5);
    }

    return NULL;
}

void Renderer::render(const std::string &outputFile)
{
    int W = cam.width();
    int H = cam.height();
    std::vector<Color> fb(W * H);
    const int len = 16;
    std::vector<PixelsBlock> blocks;
    for (int y = 0; y < H; y += len)
        for (int x = 0; x < W; x += len)
            blocks.push_back({x, y, std::min(x + len, W), std::min(y + len, H)});

    BlockWork work;
    work.renderer = this;
    work.fb = &fb;
    work.W = W;
    work.H = H;
    work.totalBlocks = (int)blocks.size();
    work.blocks = blocks.data();
    work.nextBlock = 0;
    pthread_mutex_init(&work.mutex, NULL);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int nThreads = (int)sysconf(_SC_NPROCESSORS_ONLN);
    std::vector<pthread_t> threads(nThreads);
    for (int t = 0; t < nThreads; t++)
        pthread_create(&threads[t], NULL, worker, &work);

    for (int t = 0; t < nThreads; t++)
        pthread_join(threads[t], NULL);

    pthread_mutex_destroy(&work.mutex);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    std::fprintf(stderr, "Render time: %.3f s\n", elapsed);

    FILE *out = std::fopen(outputFile.c_str(), "w");
    if (!out)
    {
        std::cout << "Failed to open output file\n";
        return;
    }
    std::fprintf(out, "P3\n%d %d\n255\n", W, H);
    for (int j = 0; j < H; j++)
    {
        for (int i = 0; i < W; i++)
        {
            const Color &c = fb[j * W + i];
            std::fprintf(out, "%d %d %d ", c.Ri(), c.Gi(), c.Bi());
        }
        std::fprintf(out, "\n");
    }
    std::fclose(out);
}
