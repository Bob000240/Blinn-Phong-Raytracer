#include "data.h"
#include <cstdio>
#include <cstring>
#include <iostream>

std::string outputFileName(const std::string &name)
{
    size_t dot = name.find_last_of('.');
    if (dot == std::string::npos)
        return name + ".ppm";
    return name.substr(0, dot) + ".ppm";
}

bool loadTexturePPM(const std::string &filename, Texture &tex)
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

    tex.width = w;
    tex.height = h;
    tex.pixels.resize(w * h);

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
            c.setR((double)r / maxv)
                .setG((double)g / maxv)
                .setB((double)b / maxv);

            tex.pixels[j * w + i] = c;
        }
    }

    std::fclose(fp);
    return true;
}

static bool parseToken(const char *tok, int &v, int &vt, int &vn)
{
    v = vt = vn = -1;

    if (std::strstr(tok, "//") != nullptr)
    {
        // v//vn
        if (std::sscanf(tok, "%d//%d", &v, &vn) != 2)
            return false;
    }
    else
    {
        int slashCount = 0;
        for (const char *p = tok; *p; p++)
            if (*p == '/')
                slashCount++;

        if (slashCount == 0)
        {
            // v
            if (std::sscanf(tok, "%d", &v) != 1)
                return false;
        }
        else if (slashCount == 1)
        {
            // v/vt
            if (std::sscanf(tok, "%d/%d", &v, &vt) != 2)
                return false;
        }
        else if (slashCount == 2)
        {
            // v/vt/vn
            if (std::sscanf(tok, "%d/%d/%d", &v, &vt, &vn) != 3)
                return false;
        }
        else
            return false;
    }

    if (v != -1)
        v -= 1;
    if (vt != -1)
        vt -= 1;
    if (vn != -1)
        vn -= 1;

    return true;
}

Data::Data(const std::string &fname) : filename(fname)
{
    FILE *fp = std::fopen(filename.c_str(), "r");
    if (!fp)
    {
        std::cerr << "Failed to open file: " << filename << "\n";
        return;
    }

    char key[128];
    while (std::fscanf(fp, "%s", key) == 1)
    {
        if (std::strcmp(key, "eye") == 0)
        {
            double x, y, z;
            if (std::fscanf(fp, "%lf %lf %lf", &x, &y, &z) != 3)
                break;
            eye = {x, y, z};
        }
        else if (std::strcmp(key, "viewdir") == 0)
        {
            double x, y, z;
            if (std::fscanf(fp, "%lf %lf %lf", &x, &y, &z) != 3)
                break;
            viewdir = {x, y, z};
        }
        else if (std::strcmp(key, "updir") == 0)
        {
            double x, y, z;
            if (std::fscanf(fp, "%lf %lf %lf", &x, &y, &z) != 3)
                break;
            updir = {x, y, z};
        }
        else if (std::strcmp(key, "vfov") == 0)
        {
            if (std::fscanf(fp, "%d", &vfov) != 1)
                break;
        }
        else if (std::strcmp(key, "imsize") == 0)
        {
            int w, h;
            if (std::fscanf(fp, "%d %d", &w, &h) != 2)
                break;
            imsize = {w, h};
        }
        else if (std::strcmp(key, "bkgcolor") == 0)
        {
            double r, g, b;
            if (std::fscanf(fp, "%lf %lf %lf %lf", &r, &g, &b, &bgdIoRefraction) != 4)
                break;
            bkgcolor.setR(r).setG(g).setB(b);
        }
        else if (std::strcmp(key, "mtlcolor") == 0)
        {
            double dr, dg, db, sr, sg, sb;

            if (std::fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %d %lf %lf",
                            &dr, &dg, &db,
                            &sr, &sg, &sb,
                            &coefficients[0], &coefficients[1], &coefficients[2], &shininess,
                            &opacity, &IoRefraction) != 12)
                break;

            diffuseLight.setR(dr).setG(dg).setB(db);
            specularLight.setR(sr).setG(sg).setB(sb);
        }
        else if (std::strcmp(key, "sphere") == 0)
        {
            double cx, cy, cz, radius;
            if (std::fscanf(fp, "%lf %lf %lf %lf", &cx, &cy, &cz, &radius) != 4)
                break;

            sphere s;
            s.cx = cx;
            s.cy = cy;
            s.cz = cz;
            s.radius = radius;
            s.diffuseLight = diffuseLight;
            s.specularLight = specularLight;
            s.coefficients = coefficients;
            s.shininess = shininess;
            s.textureId = currentTexture;
            s.alpha = opacity;
            s.eta = IoRefraction;
            spheres.push_back(s);
        }
        else if (std::strcmp(key, "cone") == 0)
        {
            double cx, cy, cz, dx, dy, dz, angle, height;
            if (std::fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf %lf",
                            &cx, &cy, &cz, &dx, &dy, &dz, &angle, &height) != 8)
                break;
            cone c;
            c.cx = cx;
            c.cy = cy;
            c.cz = cz;
            c.dx = dx;
            c.dy = dy;
            c.dz = dz;
            c.angle = angle;
            c.height = height;
            c.diffuseLight = diffuseLight;
            c.specularLight = specularLight;
            c.coefficients = coefficients;
            c.shininess = shininess;
            c.textureId = currentTexture;
            c.alpha = opacity;
            c.eta = IoRefraction;
            cones.push_back(c);
        }
        else if (std::strcmp(key, "cylinder") == 0)
        {
            double cx, cy, cz, dx, dy, dz, radius, length;
            if (std::fscanf(fp, "%lf %lf %lf %lf %lf %lf %lf %lf",
                            &cx, &cy, &cz, &dx, &dy, &dz, &radius, &length) != 8)
                break;
            cylinder c;
            c.cx = cx;
            c.cy = cy;
            c.cz = cz;
            c.dx = dx;
            c.dy = dy;
            c.dz = dz;
            c.radius = radius;
            c.length = length;
            c.diffuseLight = diffuseLight;
            c.specularLight = specularLight;
            c.coefficients = coefficients;
            c.shininess = shininess;
            c.textureId = currentTexture;
            c.alpha = opacity;
            c.eta = IoRefraction;
            cylinders.push_back(c);
        }
        else if (std::strcmp(key, "light") == 0)
        {
            double x, y, z, intensity;
            int w;
            if (std::fscanf(fp, "%lf %lf %lf %d %lf", &x, &y, &z, &w, &intensity) != 5)
                break;
            lights.push_back(Light{x, y, z, w, intensity});
        }
        else if (std::strcmp(key, "v") == 0)
        {
            double x, y, z;
            if (std::fscanf(fp, "%lf %lf %lf", &x, &y, &z) != 3)
                break;
            vertPos.push_back(vec3(x, y, z));
        }
        else if (std::strcmp(key, "vn") == 0)
        {
            double nx, ny, nz;
            if (std::fscanf(fp, "%lf %lf %lf", &nx, &ny, &nz) != 3)
                break;
            vertNor.push_back(vec3(nx, ny, nz));
        }
        else if (std::strcmp(key, "vt") == 0)
        {
            double u, v;
            if (std::fscanf(fp, "%lf %lf", &u, &v) != 2)
                break;
            texCoords.push_back(vec2(u, v));
        }
        else if (std::strcmp(key, "f") == 0)
        {
            char line[256];
            if (!std::fgets(line, sizeof(line), fp))
                break;

            int v[3], vt[3], vn[3];

            if (std::sscanf(line,
                            "%d/%d/%d %d/%d/%d %d/%d/%d",
                            &v[0], &vt[0], &vn[0],
                            &v[1], &vt[1], &vn[1],
                            &v[2], &vt[2], &vn[2]) == 9)
            {
                // v/t/n
            }
            else if (std::sscanf(line,
                                 "%d//%d %d//%d %d//%d",
                                 &v[0], &vn[0],
                                 &v[1], &vn[1],
                                 &v[2], &vn[2]) == 6)
            {
                // v//n
                vt[0] = vt[1] = vt[2] = -1;
            }
            else if (std::sscanf(line,
                                 "%d/%d %d/%d %d/%d",
                                 &v[0], &vt[0],
                                 &v[1], &vt[1],
                                 &v[2], &vt[2]) == 6)
            {
                // v/t
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
                if (vt[i] != -1)
                    vt[i]--;
                if (vn[i] != -1)
                    vn[i]--;
            }

            triangle tri;
            tri.v0 = vertPos[v[0]];
            tri.v1 = vertPos[v[1]];
            tri.v2 = vertPos[v[2]];

            tri.n0 = (vn[0] >= 0 && vn[0] < (int)vertNor.size()) ? vertNor[vn[0]] : vec3(0, 0, 0);
            tri.n1 = (vn[1] >= 0 && vn[1] < (int)vertNor.size()) ? vertNor[vn[1]] : vec3(0, 0, 0);
            tri.n2 = (vn[2] >= 0 && vn[2] < (int)vertNor.size()) ? vertNor[vn[2]] : vec3(0, 0, 0);

            tri.t0 = (vt[0] >= 0 && vt[0] < (int)texCoords.size()) ? texCoords[vt[0]] : vec2(0, 0);
            tri.t1 = (vt[1] >= 0 && vt[1] < (int)texCoords.size()) ? texCoords[vt[1]] : vec2(0, 0);
            tri.t2 = (vt[2] >= 0 && vt[2] < (int)texCoords.size()) ? texCoords[vt[2]] : vec2(0, 0);

            tri.diffuseLight = diffuseLight;
            tri.specularLight = specularLight;
            tri.coefficients = coefficients;
            tri.shininess = shininess;
            tri.textureId = currentTexture;
            tri.bumpId = -1;
            tri.alpha = opacity;
            tri.eta = IoRefraction;

            triangles.push_back(tri);
        }
        else if (std::strcmp(key, "texture") == 0)
        {
            char texFile[256];
            if (std::fscanf(fp, "%255s", texFile) != 1)
                break;

            Texture tex;
            if (!loadTexturePPM(texFile, tex))
            {
                std::cerr << "Failed to load texture: " << texFile << "\n";
                currentTexture = -1;
            }
            else
            {
                textures.push_back(tex);
                currentTexture = (int)textures.size() - 1;
            }
        }
        else
        {
            std::cout << "Unknown Command\n";
        }
    }

    std::fclose(fp);
}

std::string Data::getFilename() const { return filename; }