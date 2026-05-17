#include "raytracer.h"
#include "intersections.h"

#include <cstdio>
#include <cmath>
#include <iostream>

RayTracer::RayTracer(const std::string &filename) : data(filename)
{
    if (data.eye.size() < 3 || data.viewdir.size() < 3 || data.updir.size() < 3 || data.imsize.size() < 2)
    {
        std::cout << "Need proper input data\n";
        return;
    }

    w = -vec3(data.viewdir).unit();
    u = vec3(data.viewdir).cross(vec3(data.updir)).unit();
    v = u.cross(vec3(data.viewdir)).unit();

    n = vec3(data.viewdir).unit();

    imgW = data.imsize[0];
    imgH = data.imsize[1];
    aspect = (double)imgW / (double)imgH;

    thetaH = (double)data.vfov * M_PI / 180;
    thetaW = 2 * std::atan(aspect * std::tan(thetaH / 2));

    d = imgH * 0.5 / std::tan(0.5 * thetaH);

    viewingWindowH = 2 * d * std::tan(0.5 * thetaH);
    viewingWindowW = 2 * d * std::tan(0.5 * thetaW);

    vec3 eye = vec3(data.eye);
    upperLeft = eye + n * d - u * (viewingWindowW / 2) + v * (viewingWindowH / 2);
    upperRight = eye + n * d + u * (viewingWindowW / 2) + v * (viewingWindowH / 2);
    lowerLeft = eye + n * d - u * (viewingWindowW / 2) - v * (viewingWindowH / 2);
    lowerRight = eye + n * d + u * (viewingWindowW / 2) - v * (viewingWindowH / 2);

    delta_h = (upperRight - upperLeft) / (double)imgW;
    delta_v = (lowerLeft - upperLeft) / (double)imgH;

    delta_c_h = delta_h * 0.5;
    delta_c_v = delta_v * 0.5;
}

vec3 RayTracer::viewingRay(int i, int j) const
{
    return upperLeft + delta_h * (double)i + delta_v * (double)j + delta_c_h + delta_c_v;
}

double RayTracer::shadowFactor(const vec3 &hitPoint, const Light &light, const vec3 &N) const
{
    vec3 L;
    double maxT;
    const double EPS = 1e-6;

    if (light.w == 1)
    {
        vec3 toLight = vec3(light.x, light.y, light.z) - hitPoint;
        maxT = toLight.norm();
        L = toLight.unit();
    }
    else
    {
        maxT = 0x3f3f3f3f;
        L = -vec3(light.x, light.y, light.z).unit();
    }

    Ray shadowRay{hitPoint + N * EPS, L};

    double lightFactor = 1;

    for (const sphere &s : data.spheres)
    {
        double t;
        if (hitSphere(s, shadowRay, t) && t > EPS && t < maxT - EPS)
        {
            lightFactor *= (1 - s.alpha);
            if (lightFactor <= 0)
                return 0;
        }
    }

    for (const cone &c : data.cones)
    {
        double t;
        if (hitCone(c, shadowRay, t) && t > EPS && t < maxT - EPS)
        {
            lightFactor *= (1 - c.alpha);
            if (lightFactor <= 0)
                return 0;
        }
    }

    for (const cylinder &c : data.cylinders)
    {
        double t;
        if (hitCylinder(c, shadowRay, t) && t > EPS && t < maxT - EPS)
        {
            lightFactor *= (1 - c.alpha);
            if (lightFactor <= 0)
                return 0;
        }
    }

    for (const triangle &tri : data.triangles)
    {
        double t, a, b, c;
        if (hitTriangle(tri, shadowRay, t, a, b, c) && t > EPS && t < maxT - EPS)
        {
            lightFactor *= (1 - tri.alpha);
            if (lightFactor <= 0)
                return 0;
        }
    }

    return lightFactor;
}

vec3 RayTracer::reflection(const vec3 &I, const vec3 &N) const
{
    vec3 R = N * 2 * I.dot(N) - I;
    return R;
}

double RayTracer::fresnel(const double ni, const double nt, const vec3 &I, const vec3 &N) const
{
    double cosTheta = std::max(0.0, N.dot(I));
    return std::pow((ni - nt) / (ni + nt), 2) + (1 - std::pow((ni - nt) / (ni + nt), 2)) * std::pow(1 - cosTheta, 5);
}

vec3 RayTracer::refraction(const vec3 &I, const vec3 &N, double ni, double nt) const
{
    double eta = ni / nt;
    double cosThetaSqr = 1 - eta * eta * (1 - I.dot(N) * I.dot(N));
    if (cosThetaSqr < 0)
        return vec3(0, 0, 0); // Total internal reflection
    vec3 C = (-N) * std::sqrt(cosThetaSqr);
    vec3 D = (N * N.dot(I) - I) * (eta);
    return (C + D).unit();
}

Color RayTracer::blinnPhong(
    const vec3 &hitPoint,
    const vec3 &N,
    const vec3 &V,
    const Color &Od,
    const Color &Os,
    double ka,
    double kd,
    double ks,
    int shininess) const
{
    Color finalColor = Od * ka;

    for (const Light &light : data.lights)
    {
        double shadow = shadowFactor(hitPoint, light, N);
        if (shadow <= 0)
            continue;

        vec3 L;

        if (light.w == 1)
        {
            vec3 toLight = vec3(light.x, light.y, light.z) - hitPoint;
            double dist = toLight.norm();
            L = toLight.unit();
        }
        else
        {
            L = -vec3(light.x, light.y, light.z).unit();
        }

        double ndotl = N.dot(L);
        if (ndotl < 0)
            ndotl = 0;

        Color diffuse(0, 0, 0);
        Color specular(0, 0, 0);

        if (ndotl > 0)
        {
            diffuse = Od * (kd * ndotl);

            vec3 H = (L + V).unit();
            double ndoth = N.dot(H);
            if (ndoth < 0)
                ndoth = 0;

            double specPow = (shininess > 0) ? std::pow(ndoth, shininess) : 0;
            specular = Os * (ks * specPow);
        }

        finalColor += (diffuse + specular) * (light.intensity * shadow);
    }

    return finalColor.clamped();
}

void RayTracer::sphereHelper(const sphere &s, const Ray &ray, double t,
                             vec3 &hitPoint, vec3 &N,
                             Color &Od, Color &Os,
                             double &ka, double &kd, double &ks,
                             int &shininess) const
{
    Od = s.diffuseLight;
    Os = s.specularLight;
    ka = s.coefficients[0];
    kd = s.coefficients[1];
    ks = s.coefficients[2];
    shininess = s.shininess;

    hitPoint = ray.orig + ray.direc * t;
    N = (hitPoint - vec3(s.cx, s.cy, s.cz)).unit();

    if (s.textureId >= 0 && s.textureId < data.textures.size())
    {
        double Nx = N.x;
        double Ny = N.y;
        double Nz = N.z;

        double phi = std::acos(Nz);
        double theta = std::atan2(Ny, Nx);

        double v = phi / M_PI;
        double u = (theta >= 0) ? theta / (2 * M_PI)
                                : (theta + 2 * M_PI) / (2 * M_PI);

        Od = nearestNeighbor(data.textures[s.textureId], u, v);
    }
}

void RayTracer::coneHelper(const cone &s, const Ray &ray, double t,
                           vec3 &hitPoint, vec3 &N,
                           Color &Od, Color &Os,
                           double &ka, double &kd, double &ks,
                           int &shininess) const
{
    Od = s.diffuseLight;
    Os = s.specularLight;
    ka = s.coefficients[0];
    kd = s.coefficients[1];
    ks = s.coefficients[2];
    shininess = s.shininess;

    hitPoint = ray.orig + ray.direc * t;

    vec3 tip(s.cx, s.cy, s.cz);
    vec3 axis(s.dx, s.dy, s.dz);
    axis = axis.unit();

    double theta = s.angle * M_PI / 180;
    double cosT = std::cos(theta);
    double cos2 = cosT * cosT;

    vec3 q = hitPoint - tip;
    double m = q.dot(axis);

    N = (axis * m - q * cos2).unit();

    if (N.dot(ray.direc) > 0)
        N = -N;

    if (s.textureId >= 0 && s.textureId < (int)data.textures.size())
    {
        vec3 C(s.cx, s.cy, s.cz);
        vec3 A = vec3(s.dx, s.dy, s.dz).unit();
        vec3 CP = hitPoint - C;

        double h = CP.dot(A);
        double v = h / s.height;

        vec3 radial = CP - A * h;

        vec3 helper = (std::fabs(A.z) < 0.999) ? vec3(0, 0, 1) : vec3(0, 1, 0);
        vec3 U = (helper.cross(A)).unit();
        vec3 VV = (A.cross(U)).unit();

        double ang = std::atan2(radial.dot(VV), radial.dot(U));
        double u = (ang >= 0) ? ang / (2 * M_PI)
                              : (ang + 2 * M_PI) / (2 * M_PI);

        Od = nearestNeighbor(data.textures[s.textureId], u, v);
    }
}

void RayTracer::cylinderHelper(const cylinder &s, const Ray &ray, double t,
                               vec3 &hitPoint, vec3 &N,
                               Color &Od, Color &Os,
                               double &ka, double &kd, double &ks,
                               int &shininess) const
{
    Od = s.diffuseLight;
    Os = s.specularLight;
    ka = s.coefficients[0];
    kd = s.coefficients[1];
    ks = s.coefficients[2];
    shininess = s.shininess;

    hitPoint = ray.orig + ray.direc * t;

    vec3 base(s.cx, s.cy, s.cz);
    vec3 axis(s.dx, s.dy, s.dz);
    axis = axis.unit();

    vec3 q = hitPoint - base;
    double proj = q.dot(axis);
    vec3 radial = q - axis * proj;

    N = radial.unit();

    if (s.textureId >= 0 && s.textureId < (int)data.textures.size())
    {
        vec3 C(s.cx, s.cy, s.cz);
        vec3 A = vec3(s.dx, s.dy, s.dz).unit();
        vec3 CP = hitPoint - C;

        double h = CP.dot(A);
        double v = h / s.length;

        vec3 radial2 = CP - A * h;

        vec3 helper = (std::fabs(A.z) < 0.999) ? vec3(0, 0, 1) : vec3(0, 1, 0);
        vec3 U = (helper.cross(A)).unit();
        vec3 VV = (A.cross(U)).unit();

        double theta = std::atan2(radial2.dot(VV), radial2.dot(U));
        double u = (theta >= 0) ? theta / (2 * M_PI)
                                : (theta + 2 * M_PI) / (2 * M_PI);

        Od = nearestNeighbor(data.textures[s.textureId], u, v);
    }
}

void RayTracer::triangleHelper(const triangle &tri, const Ray &ray,
                               double tHit, double a, double b, double c,
                               vec3 &hitPoint, vec3 &N,
                               Color &Od, Color &Os,
                               double &ka, double &kd, double &ks,
                               int &shininess) const
{
    Od = tri.diffuseLight;
    Os = tri.specularLight;
    ka = tri.coefficients[0];
    kd = tri.coefficients[1];
    ks = tri.coefficients[2];
    shininess = tri.shininess;

    hitPoint = ray.orig + ray.direc * tHit;

    bool hasNormals = !(tri.n0.norm() == 0 && tri.n1.norm() == 0 && tri.n2.norm() == 0);

    if (!hasNormals)
    {
        vec3 e1 = tri.v1 - tri.v0;
        vec3 e2 = tri.v2 - tri.v0;
        N = e1.cross(e2).unit();
    }
    else
    {
        N = (tri.n0 * a + tri.n1 * b + tri.n2 * c).unit();
    }

    if (N.dot(ray.direc) > 0)
        N = -N;

    if (tri.textureId >= 0 && tri.textureId < (int)data.textures.size())
    {
        double u = a * tri.t0.x + b * tri.t1.x + c * tri.t2.x;
        double v = a * tri.t0.y + b * tri.t1.y + c * tri.t2.y;

        Od = nearestNeighbor(data.textures[tri.textureId], u, v);
    }
}

Color RayTracer::nearestNeighbor(const Texture &tex, double u, double v) const
{
    if (!tex.valid())
        return Color(1, 1, 1);

    // wrap to [0,1)
    u = u - std::floor(u);
    v = v - std::floor(v);

    int i = (int)std::round(u * (tex.width - 1));
    int j = (int)std::round(v * (tex.height - 1));

    if (i < 0)
        i = 0;
    if (i >= tex.width)
        i = tex.width - 1;
    if (j < 0)
        j = 0;
    if (j >= tex.height)
        j = tex.height - 1;

    return tex.pixels[j * tex.width + i];
}

Color RayTracer::recursiveTrace(const Ray &ray, int depth) const
{
    if (depth <= 0)
        return data.bkgcolor;

    double closestT = 0x3f3f3f3f;

    const sphere *hitSph = nullptr;
    double hitSphT = 0;
    const cone *hitCon = nullptr;
    double hitConT = 0;
    const cylinder *hitCyl = nullptr;
    double hitCylT = 0;
    const triangle *hitTri = nullptr;
    double hitTriT = 0;
    double hitA = 0, hitB = 0, hitC = 0;

    enum HitType
    {
        HIT_NONE,
        HIT_SPHERE,
        HIT_CONE,
        HIT_CYL,
        HIT_TRI
    };
    HitType hitType = HIT_NONE;

    for (const sphere &s : data.spheres)
    {
        double t;
        if (hitSphere(s, ray, t) && t < closestT)
        {
            closestT = t;
            hitType = HIT_SPHERE;
            hitSph = &s;
            hitSphT = t;
        }
    }

    for (const cone &c : data.cones)
    {
        double t;
        if (hitCone(c, ray, t) && t < closestT)
        {
            closestT = t;
            hitType = HIT_CONE;
            hitCon = &c;
            hitConT = t;
        }
    }

    for (const cylinder &c : data.cylinders)
    {
        double t;
        if (hitCylinder(c, ray, t) && t < closestT)
        {
            closestT = t;
            hitType = HIT_CYL;
            hitCyl = &c;
            hitCylT = t;
        }
    }

    for (const triangle &tri : data.triangles)
    {
        double t, a, b, c;
        if (hitTriangle(tri, ray, t, a, b, c) && t < closestT)
        {
            closestT = t;
            hitType = HIT_TRI;
            hitTri = &tri;
            hitTriT = t;
            hitA = a;
            hitB = b;
            hitC = c;
        }
    }

    vec3 hitPoint, N;
    Color Od, Os;
    double ka, kd, ks;
    int shininess;

    double alpha = 1;
    double eta = 1;
    switch (hitType)
    {
    case HIT_SPHERE:
    {
        sphereHelper(*hitSph, ray, hitSphT,
                     hitPoint, N, Od, Os, ka, kd, ks, shininess);

        alpha = hitSph->alpha;
        eta = hitSph->eta;
        break;
    }
    case HIT_CONE:
    {
        coneHelper(*hitCon, ray, hitConT,
                   hitPoint, N, Od, Os, ka, kd, ks, shininess);

        alpha = hitCon->alpha;
        eta = hitCon->eta;
        break;
    }
    case HIT_CYL:
    {
        cylinderHelper(*hitCyl, ray, hitCylT,
                       hitPoint, N, Od, Os, ka, kd, ks, shininess);
        alpha = hitCyl->alpha;
        eta = hitCyl->eta;
        break;
    }
    case HIT_TRI:
    {
        triangleHelper(*hitTri, ray, hitTriT, hitA, hitB, hitC,
                       hitPoint, N, Od, Os, ka, kd, ks, shininess);
        alpha = hitTri->alpha;
        eta = hitTri->eta;
        break;
    }
    default:
        return data.bkgcolor;
    }

    bool entering = ray.direc.dot(N) < 0;
    // flip when refracting out of the object
    vec3 normal = entering ? N : -N;
    double ni = entering ? data.bgdIoRefraction : eta;
    double nt = entering ? eta : data.bgdIoRefraction;

    Color reflectColor(0, 0, 0);
    Color refractColor(0, 0, 0);

    vec3 I = (-ray.direc).unit();
    if (ks > 0)
    {
        vec3 Rdir = reflection(I, normal).unit();
        Ray reflectRay{hitPoint + normal * 1e-6, Rdir};
        reflectColor = recursiveTrace(reflectRay, depth - 1);
    }
    double F = (alpha < 1) ? fresnel(ni, nt, I, normal) : ks;

    if (alpha < 1)
    {
        vec3 Tdir = refraction(I, normal, ni, nt);
        if (Tdir.norm() > 0)
        {
            Ray refractRay{hitPoint - normal * 1e-6, Tdir.unit()};
            refractColor = recursiveTrace(refractRay, depth - 1);
        }
    }

    vec3 V = (-ray.direc).unit();
    Color localColor = blinnPhong(hitPoint, normal, V, Od, Os, ka, kd, ks, shininess);

    return (localColor + reflectColor * F + refractColor * (1 - F)).clamped();
}

void RayTracer::renderShapes()
{
    int W = data.imsize[0];
    int H = data.imsize[1];

    std::string outFile = outputFileName(data.getFilename());
    FILE *out = std::fopen(outFile.c_str(), "w");
    if (!out)
    {
        std::cout << "Failed to open output file\n";
        return;
    }

    std::fprintf(out, "P3\n%d %d\n255\n", W, H);

    for (int j = 0; j < H; ++j)
    {
        for (int i = 0; i < W; ++i)
        {
            vec3 p = viewingRay(i, j);
            Ray ray{vec3(data.eye), (p - vec3(data.eye)).unit()};

            Color pixelColor = recursiveTrace(ray, 5);

            std::fprintf(out, "%d %d %d ", pixelColor.Ri(), pixelColor.Gi(), pixelColor.Bi());
        }
        std::fprintf(out, "\n");
    }

    std::fclose(out);
}