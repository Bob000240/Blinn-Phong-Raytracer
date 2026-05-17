#include "intersections.h"
#include <cmath>

bool hitSphere(const sphere &s, const Ray &ray, double &tHit)
{
    vec3 center(s.cx, s.cy, s.cz);
    vec3 oc = ray.orig - center;

    double A = ray.direc.dot(ray.direc);
    double B = 2 * oc.dot(ray.direc);
    double C = oc.dot(oc) - s.radius * s.radius;

    double disc = B * B - 4 * A * C;
    if (disc < 0)
        return false;

    double sq = std::sqrt(disc);
    double t0 = (-B - sq) / (2 * A);
    double t1 = (-B + sq) / (2 * A);

    bool found = false;
    double best = 0x3f3f3f3f;

    if (t0 > 0)
    {
        best = t0;
        found = true;
    }
    if (t1 > 0 && t1 < best)
    {
        best = t1;
        found = true;
    }

    if (!found)
        return false;

    tHit = best;
    return true;
}

bool hitCylinder(const cylinder &s, const Ray &ray, double &tHit)
{
    vec3 base(s.cx, s.cy, s.cz);
    vec3 axis(s.dx, s.dy, s.dz);
    axis = axis.unit();

    vec3 f = ray.orig - base;
    vec3 d = ray.direc;

    double A = d.dot(d) - (d.dot(axis) * d.dot(axis));
    double B = 2 * (d.dot(f) - (d.dot(axis) * f.dot(axis)));
    double C = f.dot(f) - (f.dot(axis) * f.dot(axis)) - s.radius * s.radius;

    double disc = B * B - 4 * A * C;
    if (disc < 0)
        return false;

    double sq = std::sqrt(disc);
    double t0 = (-B - sq) / (2 * A);
    double t1 = (-B + sq) / (2 * A);

    bool found = false;
    double best = 0x3f3f3f3f;

    if (t0 > 0)
    {
        vec3 x0 = ray.orig + d * t0;
        double proj0 = (x0 - base).dot(axis);
        if (proj0 >= 0 && proj0 <= s.length)
        {
            best = t0;
            found = true;
        }
    }

    if (t1 > 0)
    {
        vec3 x1 = ray.orig + d * t1;
        double proj1 = (x1 - base).dot(axis);
        if (proj1 >= 0 && proj1 <= s.length && t1 < best)
        {
            best = t1;
            found = true;
        }
    }

    if (!found)
        return false;

    tHit = best;
    return true;
}

bool hitCone(const cone &s, const Ray &ray, double &tHit)
{
    vec3 tip(s.cx, s.cy, s.cz);
    vec3 axis(s.dx, s.dy, s.dz);
    axis = axis.unit();

    vec3 v = ray.orig - tip;
    vec3 d = ray.direc;

    double theta = s.angle * M_PI / 180.0;
    double cosT = std::cos(theta);
    double cos2 = cosT * cosT;

    double da = d.dot(axis);
    double va = v.dot(axis);

    double A = da * da - cos2 * d.dot(d);
    double B = 2 * (da * va - cos2 * v.dot(d));
    double C = va * va - cos2 * v.dot(v);

    if (std::abs(A) < 1e-12)
        return false;

    double disc = B * B - 4 * A * C;
    if (disc < 0)
        return false;

    double sq = std::sqrt(disc);
    double t0 = (-B - sq) / (2 * A);
    double t1 = (-B + sq) / (2 * A);

    bool found = false;
    double best = 0x3f3f3f3f;

    if (t0 > 0)
    {
        vec3 x0 = ray.orig + d * t0;
        double h0 = (x0 - tip).dot(axis);
        if (h0 >= 0 && h0 <= s.height)
        {
            best = t0;
            found = true;
        }
    }

    if (t1 > 0)
    {
        vec3 x1 = ray.orig + d * t1;
        double h1 = (x1 - tip).dot(axis);
        if (h1 >= 0 && h1 <= s.height && t1 < best)
        {
            best = t1;
            found = true;
        }
    }

    if (!found)
        return false;

    tHit = best;
    return true;
}

bool hitTriangle(const triangle &tri, const Ray &ray,
                 double &tHit, double &A, double &B, double &C)
{
    const double EPS = 1e-8;
    vec3 v0 = tri.v0;
    vec3 v1 = tri.v1;
    vec3 v2 = tri.v2;

    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;
    vec3 n = e1.cross(e2);

    if (n.norm() < EPS)
        return false;

    double denom = n.dot(ray.direc);

    if (std::abs(denom) < EPS)
        return false;

    double t = n.dot(v0 - ray.orig) / denom;
    if (t <= EPS)
        return false;

    vec3 p = ray.orig + ray.direc * t;

    vec3 c0 = (v1 - v0).cross(p - v0);
    if (n.dot(c0) < 0)
        return false;

    vec3 c1 = (v2 - v1).cross(p - v1);
    if (n.dot(c1) < 0)
        return false;

    vec3 c2 = (v0 - v2).cross(p - v2);
    if (n.dot(c2) < 0)
        return false;

    double area2 = n.dot(n);

    A = n.dot((v1 - p).cross(v2 - p)) / area2;
    B = n.dot((v2 - p).cross(v0 - p)) / area2;
    C = n.dot((v0 - p).cross(v1 - p)) / area2;

    tHit = t;
    return true;
}