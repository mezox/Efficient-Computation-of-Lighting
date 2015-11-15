Efficient-Computation-of-Lighting
=================================

This bachelor's thesis deals with efficient computation of lighting in graphics scenes containing many light sources. Basic lighting calculation techniques as well as techniques derived from them will be introduced. The main goal of this thesis is to investigate tiled shading and its optimizations described more in detail in both theoretical and practical sections. Concluding part consists of experiments performed on this techniques measuring their efficiency as well as implementation details of some interesting or important parts of them.

Features:
* Deferred shading
* Tiled deferred shading
* Tiled forward shading (Forward+)
* Depth optimization per tile using downsampling of depth buffer

![Scene with 1000 lights](http://i.imgur.com/mkLe29u.jpg)

Illustration of G-Buffer data when using Tiled Deferred approach:
G-buffer holds data:
* diffuse tex
* specular tex
* viewspace normals
* viewspace positions
* depth buffer

Also on screen
* downsampled depthbuffer(minimum depth per tile)

![G-Buffer](http://i.imgur.com/GqGLjOL.jpg)

Visualisation of tiles affected by lights:
![Tiles with lighting](http://i.imgur.com/FHdD3U0.png)

Visualisation of screen space bounding boxes of lights (256 lights)
![BBoxes](http://i.imgur.com/TcEsUko.png)

Light heat map:
![Light heat map](http://i.imgur.com/3nuLFJ7.png)
