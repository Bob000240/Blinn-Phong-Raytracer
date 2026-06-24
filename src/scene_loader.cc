#include "scene_loader.h"
#include <cstdio>
#include <cstring>
#include <iostream>

static std::string outputFileName(const std::string &name)
{
    size_t dot = name.find_last_of('.');
    if (dot == std::string::npos)
        return name + ".ppm";
    return name.substr(0, dot) + ".ppm";
}

static bool loadTexturePPM(const std::string &filename, Texture &tex)
{
    FILE *fp = std::fopen(filename.c_str(), "r");
    if (!fp)
        return false;

    char magic[8];
    int w, h, maxv;

    if (std::fscanf(fp, "%7s %d %d %d", magic, &w, &h, &maxv) != 4)
    {
        std::fclose(fp);
        return false;
    }

    if (std::strcmp(magic, "P3") != 0 || w <= 0 || h <= 0 || maxv <= 0)
    {
        std::fclose(fp);
        return false;
    }

    tex.width  = w;
    tex.height = h;
    tex.pixels.resize(w * h);

    if (maxv <= 0) { std::fclose(fp); return false; }
    const double scale = 1.0 / maxv;

    for (int j = 0; j < h; ++j)
    {
        for (int i = 0; i < w; ++i)
        {
            int r, g, b;
            if (std::fscanf(fp, "%d %d %d", &r, &g, &b) != 3)
            {
                std::fclose(fp);
                return false;
            }

            Color c;
            c.setR(r * scale)
             .setG(g * scale)
             .setB(b * scale);

            tex.pixels[j * w + i] = c;
        }
    }

    std::fclose(fp);
    return true;
}

LoadResult SceneLoader::load(const std::string &filename)
{
    LoadResult result;

    FILE *fp = std::fopen(filename.c_str(), "r");
    if (!fp)
    {
        std::cerr << "Failed to open file: " << filename << "\n";
        return result;
    }

    vec3 eye, viewdir, updir;
    int vfov = 0, imgW = 0, imgH = 0;
    bool hasEye = false, hasViewdir = false, hasUpdir = false;
    bool hasVfov = false, hasImsize = false;

    Material currentMat;
    int currentTextureId = -1;

    std::vector<vec3> vertPos;
    std::vector<vec3> vertNor;
    std::vector<vec2> texCoords;

    char key[128];
    while (std::fscanf(fp, "%s", key) == 1)
    {
        if (std::strcmp(key, "eye") == 0)
        {
            double x, y, z;
            if (std::fscanf(fp, "%lf %lf %lf", &x, &y, &z) != 3) break;
            eye = vec3(x, y, z);
            hasEye = true;
        }
        else if (std::strcmp(key, "viewdir") == 0)
        {
            double x, y, z;
            if (std::fscanf(fp, "%lf %lf %lf", &x, &y, &z) != 3) break;
            viewdir = vec3(x, y, z);
            hasViewdir = true;
        }
        else if (std::strcmp(key, "updir") == 0)
        {
            double x, y, z;
            if (std::fscanf(fp, "%lf %lf %lf", &x, &y, &z) != 3) break;
            updir = vec3(x, y, z);
            hasUpdir = true;
        }
        else if (std::strcmp(key, "vfov") == 0)
        {
            if (std::fscanf(fp, "%d", &vfov) != 1) break;
            hasVfov = true;
        }
        else if (std::strcmp(key, "imsize") == 0)
        {
            if (std::fscanf(fp, "%d %d", &imgW, &imgH) != 2) break;
            hasImsize = true;
        }
        else if (std::strcmp(key, "bkgcolor") == 0)
        {
            double r, g, b;
            if (std::fscanf(fp, "%lf %lf %lf %lf",
                            &r, &g, &b, &result.scene.bgdIoRefraction) != 4) break;
            result.scene.bkgcolor.setR(r).setG(g).setB(b);
        }
        else if (std::strcmp(key, "mtlcolor") == 0)
        {
            double dr, dg, db, sr, sg, sb;
            if (std::fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %d %lf %lf",
                            &dr, &dg, &db,
                            &sr, &sg, &sb,
                            &currentMat.ka, &currentMat.kd, &currentMat.ks,
                            &currentMat.shininess,
                            &currentMat.alpha, &currentMat.eta) != 12) break;
            currentMat.diffuse.setR(dr).setG(dg).setB(db);
            currentMat.specular.setR(sr).setG(sg).setB(sb);
        }
        else if (std::strcmp(key, "sphere") == 0)
        {
            double cx, cy, cz, radius;
            if (std::fscanf(fp, "%lf %lf %lf %lf", &cx, &cy, &cz, &radius) != 4) break;
            sphere s;
            s.cx = cx; s.cy = cy; s.cz = cz; s.radius = radius;
            s.mat = currentMat;
            s.mat.textureId = currentTextureId;
            result.scene.spheres.push_back(s);
        }
        else if (std::strcmp(key, "cone") == 0)
        {
            double cx, cy, cz, dx, dy, dz, angle, height;
            if (std::fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf %lf",
                            &cx, &cy, &cz, &dx, &dy, &dz, &angle, &height) != 8) break;
            cone c;
            c.cx = cx; c.cy = cy; c.cz = cz;
            c.dx = dx; c.dy = dy; c.dz = dz;
            c.angle = angle; c.height = height;
            c.mat = currentMat;
            c.mat.textureId = currentTextureId;
            result.scene.cones.push_back(c);
        }
        else if (std::strcmp(key, "cylinder") == 0)
        {
            double cx, cy, cz, dx, dy, dz, radius, length;
            if (std::fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf %lf",
                            &cx, &cy, &cz, &dx, &dy, &dz, &radius, &length) != 8) break;
            cylinder c;
            c.cx = cx; c.cy = cy; c.cz = cz;
            c.dx = dx; c.dy = dy; c.dz = dz;
            c.radius = radius; c.length = length;
            c.mat = currentMat;
            c.mat.textureId = currentTextureId;
            result.scene.cylinders.push_back(c);
        }
        else if (std::strcmp(key, "light") == 0)
        {
            double x, y, z, intensity;
            int w;
            if (std::fscanf(fp, "%lf %lf %lf %d %lf", &x, &y, &z, &w, &intensity) != 5) break;
            result.scene.lights.push_back(Light{x, y, z, w, intensity});
        }
        else if (std::strcmp(key, "v") == 0)
        {
            double x, y, z;
            if (std::fscanf(fp, "%lf %lf %lf", &x, &y, &z) != 3) break;
            vertPos.push_back(vec3(x, y, z));
        }
        else if (std::strcmp(key, "vn") == 0)
        {
            double nx, ny, nz;
            if (std::fscanf(fp, "%lf %lf %lf", &nx, &ny, &nz) != 3) break;
            vertNor.push_back(vec3(nx, ny, nz));
        }
        else if (std::strcmp(key, "vt") == 0)
        {
            double u, v;
            if (std::fscanf(fp, "%lf %lf", &u, &v) != 2) break;
            texCoords.push_back(vec2(u, v));
        }
        else if (std::strcmp(key, "f") == 0)
        {
            char line[256];
            if (!std::fgets(line, sizeof(line), fp)) break;

            int v[3], vt[3], vn[3];

            if (std::sscanf(line,
                            "%d/%d/%d %d/%d/%d %d/%d/%d",
                            &v[0], &vt[0], &vn[0],
                            &v[1], &vt[1], &vn[1],
                            &v[2], &vt[2], &vn[2]) == 9)
            {
                // v/vt/vn
            }
            else if (std::sscanf(line,
                                 "%d//%d %d//%d %d//%d",
                                 &v[0], &vn[0],
                                 &v[1], &vn[1],
                                 &v[2], &vn[2]) == 6)
            {
                // v//vn
                vt[0] = vt[1] = vt[2] = -1;
            }
            else if (std::sscanf(line,
                                 "%d/%d %d/%d %d/%d",
                                 &v[0], &vt[0],
                                 &v[1], &vt[1],
                                 &v[2], &vt[2]) == 6)
            {
                // v/vt
                vn[0] = vn[1] = vn[2] = -1;
            }
            else if (std::sscanf(line,
                                 "%d %d %d",
                                 &v[0], &v[1], &v[2]) == 3)
            {
                // v
                vt[0] = vt[1] = vt[2] = -1;
                vn[0] = vn[1] = vn[2] = -1;
            }
            else
            {
                std::cout << "Bad format\n";
                continue;
            }

            for (int i = 0; i < 3; i++)
                v[i]--;

            for (int i = 0; i < 3; i++)
            {
                if (vt[i] != -1) vt[i]--;
                if (vn[i] != -1) vn[i]--;
            }

            triangle tri;
            tri.v0 = vertPos[v[0]];
            tri.v1 = vertPos[v[1]];
            tri.v2 = vertPos[v[2]];

            tri.n0 = (vn[0] >= 0 && vn[0] < (int)vertNor.size())   ? vertNor[vn[0]]   : vec3(0, 0, 0);
            tri.n1 = (vn[1] >= 0 && vn[1] < (int)vertNor.size())   ? vertNor[vn[1]]   : vec3(0, 0, 0);
            tri.n2 = (vn[2] >= 0 && vn[2] < (int)vertNor.size())   ? vertNor[vn[2]]   : vec3(0, 0, 0);

            tri.t0 = (vt[0] >= 0 && vt[0] < (int)texCoords.size()) ? texCoords[vt[0]] : vec2(0, 0);
            tri.t1 = (vt[1] >= 0 && vt[1] < (int)texCoords.size()) ? texCoords[vt[1]] : vec2(0, 0);
            tri.t2 = (vt[2] >= 0 && vt[2] < (int)texCoords.size()) ? texCoords[vt[2]] : vec2(0, 0);

            tri.mat = currentMat;
            tri.mat.textureId = currentTextureId;

            result.scene.triangles.push_back(tri);
        }
        else if (std::strcmp(key, "texture") == 0)
        {
            char texFile[256];
            if (std::fscanf(fp, "%255s", texFile) != 1) break;

            Texture tex;
            if (!loadTexturePPM(texFile, tex))
            {
                std::cerr << "Failed to load texture: " << texFile << "\n";
                currentTextureId = -1;
            }
            else
            {
                result.scene.textures.push_back(tex);
                currentTextureId = (int)result.scene.textures.size() - 1;
            }
        }
        else
        {
            std::cout << "Unknown Command\n";
        }
    }

    std::fclose(fp);

    if (!hasEye || !hasViewdir || !hasUpdir || !hasVfov || !hasImsize)
    {
        std::cout << "Need proper input data\n";
        return result;
    }

    result.camera     = Camera(eye, viewdir, updir, vfov, imgW, imgH);
    result.outputFile = outputFileName(filename);
    result.valid      = true;
    return result;
}
