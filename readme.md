# Raytracing

A multithreaded raytracing library built in C++. Example usages available in the "Sandbox" project.

![Screenshot](https://github.com/erbuka/raytracing/blob/master/screenshot.jpg)

## Building

This project uses [Premake](https://premake.github.io/) to build project files.

## Implementation

### Scene definition

The __Scene__ is constructed with a scene graph. Components can be attached to each node, and by default each node carries a __Transform__ component which defines local translation, rotation and scale. Shapes are component too, and so they have to be attached to a node in order to be rendered.

There are 3 basic shapes: __Sphere__, __Plane__ and __TriangleMesh__, but the base __Shape__ class can be extended to support more. Anyway the TriangleMesh allows to render almost everything. For an efficient rendering, triangle meshes use a KD-tree to store triangles inside to minimize the number of intersection tests.

Shapes can be assigned a __Material__ which defines the appearance of the shape. Materials inherit from the base class __Material__ which defines the properties of every point in space (color, reflectivity, etc.). The class __UniformMaterial__ can be used to build materials that have the same appearance in every point in space. To build more complex materials, they can be combined using __InterpolatedMaterial__, which interpolates between 2 materials given a 3D noise function. There are several built-in noise functions (Perlin, Worley, CheckerBoard, Marble), but the base __Noise__ class can be extended to achieve more complex results. 

All the examples in the __Sandbox__ project use the predefined noises, which already allow to achieve a lot of interesting results.

The __Background__ base class can be used to specify the appearance of the background. There are 2 implementations:
- the __SkyBox__ class, which renders a sky with a light source (sun/moon).
- the __ColorBackground__ class, which renders a solid color as a background.

If no background is specified, black is used.

### Rendering

The base class __Renderer__ defines a generic renderer for a __Scene__ object: basically the _Render_ method takes a Scene reference as a parameter and returns the rendered image as an array of pixels. The rendering process is supposed to be asynchronous, and that's why the result is stored in a __std::promise__.
__AbstractRaycaster__ inherits from Renderer, and defines the _Render_ method, which uses multiple threads to render the scene by calling the abstract method _Raycast_. The concrete class __Raytracer__ inherits from __AbstractRaycaster__ and of course implements the _Raycast_ method. 

The rendering process splits the screen into vertical lines 1 pixel wide, and then each line is rendered in a separate thread. A pool of N (user-defined) threads is instantiated, and they run concurrently rendering one line at time until all of them have been rendered.
