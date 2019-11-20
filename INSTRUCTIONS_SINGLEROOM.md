# LPortal SingleRoom mode

### About
Although LPortal is primarily designed for occlusion culling with rooms and portals, it also contains some optimizations to increase the accuracy of general culling (both for the camera and lights). While I couldn't easily integrate these into Godot 3.2 core (because of shadow map caching), they can still be used relatively simply with most 3d games via LPortal.

### How does it work?
The optimizations speed up and increase the accuracy of general camera / light culling using several methods, especially the following:

* Shadow casters are culled so that only those that affect the camera frustum are drawn to shadow maps
* Entire lights are culled if they do not affect the camera frustum
* Objects are culled / shown by detaching / attaching to the scene graph 

Note that these methods will primarily speed up rendering in games that use realtime lighting, rather than baked lighting. They aren't designed to work with GI probe and may produce artefacts (such as shadow popping) when used with GI probe.

