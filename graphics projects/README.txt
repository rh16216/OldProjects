Hypertexture is a method of describing 3D shapes where every point is given a density value between 0 and 1, where 0 is transparent and 1 is opaque.
Hypertexures can be used to model phenomena such as fire.
My lab partner's final year individual project was designing a language to describe hypertexture, the raytracer and rasteriser render images of a fireball generated from her Haskell project.
The fireball is contained within a bounding box, which occupies its own coordinate space, where the origin is the bottom front left vertex, and the axes are in the direction of the edges of the box.

Both are compiled using 'make' , and both are run using './Build/main'.


RAYTRACER:

We implemented raymarching as outlined by Perlin and Hoffert's 1989 paper 'Hypertexture'
This includes the dynamic normal calculation outlined in the paper, which can be turned on and off in our code using the NORMAL_FLAG flag.
The number of steps taken when raymarching can be set by the RAY_MARCH_STEP variable.
Another light source was placed inside the fireball to make it 'glow'.
Soft shadows are simulated by use of multiple dimmer light sources. The shadow ray also utilises the raymarching by looking at the accumulated density to calcualte the darkness of the shadow.
Note the resolution is set very low by default to enable quick computation of the initial frame.

RASTERISER:

We couldn't find a raymarching equivalent for the rastersier, so we invented our own!

It works as follows:
-Identifies the corners of the bounding box (eg top front left, bottom back right etc)
-Interpolates front to back to get the coordinates of the four corners of each 2D slice
-These corners are used in similar fashion to triangle coordinates in a traditional rasteriser to work out the rows and colums required per slice, and their 3D coordinates
-Each point is then moved to hyperspace to lookup its density using the haskell function
-The colour of the point is then calculated using the density, and the pixel's colour is calculated by compositing the results from the previous slices

This works real time, except for the haskell lookup function. To see it run in real time, we have implemented a basic 'soft sphere' in C++ which can be used by setting the FIREBALL flag to false.

NORMAL CALCULATION:

An optional flag in both the rasteriser and the raytracer; dynamic normal calculation associates a normal with each point instead of using the normal of the bounding box face, which made the slices look flatter.

It works as follows:
-The direction of the ray is established by calculating the difference between the current point and previous point along the ray
-A 'throw away' direction is then calculated by rotating this vector by 30 degrees
-The cross product of those two vectors is then calculated to generate a third vector orthogonal to both
-The cross product of the original vector and this third vector is then taken to create another vector orthogonal to both
-The 'throw away' vector is then discarded and the other vectors normalised forming an orthonormal basis
-The change in value of the haskell fireball function is then determined in these three directions
-These three difference values are then multiplied by the basis vectors to convert the overall derivative to geometric space
-This vector is then normalised and used as a normal
