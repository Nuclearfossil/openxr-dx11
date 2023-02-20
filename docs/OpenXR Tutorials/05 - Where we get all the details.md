# Overview
At the end of this section, we'll _almost_ be to a point where we can start rendering objects to device.

Almost.

What we're going to do in this section is talk more about finalizaing OpenXR's initialization. This also includes discussing actually initializing Direct 3D 11. So there will be a few tangents here and there to talk about D3D and DXGI matters.

# What's left to do
As part of our next batch of topics, we now have to consider how we go about finishing up OpenXR initialization. One thing we have not covered is how OpenXR and Direct 3D 11 interact with each other. We'll try and cover that now.

# Updates for OpenXR
## OpenXR 
## XR Session

## Graphics
In a previous episode, I mentioned that in any typical OpenXR structure, we have two common properties:
```c++
typedef struct XrSomeStructCreateInfo {
    XrStructureType             type;
    const void* XR_MAY_ALIAS    next;
```
In the past, we've taked at length about the `type` property, but we very much glossed over the `next` property, and chaining calls.

We can use the `next` to add additional structures into the that structure for additional OpenXR runtimes to use.

# Summary
Everything that we've touched on can be found in the source code in the Visual Studio project `OpenXR-Tutorial-003`.

# Resources
