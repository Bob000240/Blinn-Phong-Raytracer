## Design Specifications
This program reads a text file from the command line and generates an image based on parameters.


## How to Compile
`make all` or `mingw32-make all` if on windows or just `g++ -std=c++20 -Iinclude main.cc src/vec.cc src/color.cc src/intersections.cc src/data.cc src/raytracer.cc -o raytracer`

## How to Runmake 
`./raytracer ___.txt`

This will generate
`___.ppm`
which can be opened with GIMP