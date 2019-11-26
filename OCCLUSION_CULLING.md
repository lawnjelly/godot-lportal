# Occlusion Culling Overview

## What is occlusion culling?
Occlusion culling refers to the removal of objects (and faces) as early as possible from the rendering pipeline when they have been determined to be hidden from view (occluded) by geometry that is in front of them. This most often is taken to mean hidden from the point of view of the camera, but occlusion culling can also take place from other viewpoints such as the view of lights, in order to determine which objects can cast shadows.

while it is fairly easy to determine which objects are outside a view frustum (for example using a series of plane checks), determining which objects are in view within that frustum is a far more difficult problem. If we send everything to the GPU to render this will be resolved using brute force using the Z buffer, however if we can use cheaper techniques this can potentially save a lot of processing.

## How does a naive modern renderer determine visibility?
As GPUs have become more powerful over time, the types of fragment shaders used now are far more complex than their counterparts in the early years of GPUs (early 2000s). Modern fragment shaders often use PBR rendering and many calculations, whereas early (fixed function) shaders were simply one or two texture lookups. Due to this, a very common optimization technique to save on fill rate has been to split rendering into 2 passes:

1) A simple shader that only writes to the z buffer
2) Full shader that tests the pre-created z buffer

While rendering everything twice appears counter-intuitive, it is often faster because the first pass has a very cheap shader, and as a result of the first pass, the expensive second pass only has to render those fragments that have already been determined to be visible.

As a result, if we naively draw an object behind something that will occlude it, it is wasting processing, however it is NOT AS EXPENSIVE as if the object were also put through the second pass (rendered in its entirety).

## The benefits of occlusion culling
As such, the savings we get from occlusion culling depend on

1) The proportion of objects we can cull away before the first pass
2) The relative cost of the shaders used in pass (1) and pass (2)

As fragment shaders have become more expensive over the years, the cost of pass (1) relative to pass (2) has dropped. The cost of vertex processing relative to fragment shading has also probably dropped over the years. This means that the performance benefits from occlusion culling have become less pronounced on high powered PCs and consoles.

However it can still be very important on low power devices such as phones, tablets and PCs that don't feature expensive GPUs, where there is often no choice but to run simpler shaders for high performance.

There are also benefits to visibility determination outside of rendering.

Occlusion culling can potentially save performance by:

1) Reducing the fragment shader costs of pass (1)
2) Reducing the number of draw calls / material changes
3) Eliminating some of the transparent objects
4) Enabling other processing such as physics / AI to be switched off for invisible objects



