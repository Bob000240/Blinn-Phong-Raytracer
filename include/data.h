#ifndef DATA_H
#define DATA_H

#include <string>
#include <vector>
#include "scene_types.h"

std::string outputFileName(const std::string &name);
struct Texture
{
    int width = 0;
    int height = 0;
    std::vector<Color> pixels;

    bool valid() const
    {
        return width > 0 && height > 0 && !pixels.empty();
    }
};
bool loadTexturePPM(const std::string &filename, Texture &tex);

class Data
{
private:
    std::string filename;

public:
    std::vector<double> eye;
    std::vector<double> viewdir;
    std::vector<double> updir;
    int vfov = 0;
    std::vector<int> imsize;

    Color bkgcolor{0, 0, 0};
    Color diffuseLight{1, 1, 1};
    Color specularLight{1, 1, 1};
    std::vector<double> coefficients{1, 1, 1};
    int shininess = 0;
    std::vector<Light> lights;

    std::vector<vec3> vertPos;
    std::vector<vec3> vertNor;
    std::vector<vec2> texCoords;

    std::vector<Texture> textures;
    int currentTexture = -1;

    std::vector<sphere> spheres;
    std::vector<cone> cones;
    std::vector<cylinder> cylinders;
    std::vector<triangle> triangles;

    double opacity = 1;
    double bgdIoRefraction = 1;
    double IoRefraction = 1;

    std::string getFilename() const;
    Data(const std::string &fname);
};

#endif