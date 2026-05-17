#ifndef INTERSECTIONS_H
#define INTERSECTIONS_H

#include "scene_types.h"

bool hitSphere(const sphere &s, const Ray &ray, double &tHit);
bool hitCylinder(const cylinder &s, const Ray &ray, double &tHit);
bool hitCone(const cone &s, const Ray &ray, double &tHit);
bool hitTriangle(const triangle &tri, const Ray &ray, double &tHit, double &a, double &b, double &c);

#endif