# Overview

In this lesson, I will go over some of the essentials of DirectX, as that is part of the foundation of how we get content to the screen. By the end of this section, you has have a good understanding of the capabilities of DirectX 11 and how we will be using it to drive the display in OpenXR.

# What is DirectX 11?

By definition, DirectX 11 is an assortment of APIs for accessing low level hardware that varies from Graphics, to Input to audio and networking.  Wikipedia defines it DirectX as:
```
a collection of application programming interfaces (APIs) for handling tasks related to multimedia, especially game programming and video, on Microsoft platforms.
```
Over the years, DirectX has had many evolutionary passes, up until it's current offering: DirectX 12.

So why not use DirectX 12 for these lessons? Pretty much for the same reason we're not using Vulkan. The difference between DirectX 11 and 12 are fairly significant and require a lot more lower level resource management that you would need to do in DirectX 11.

# What APIs in DirectX 11 will we use

That's actually a pretty straightforward answer:
- DirectX Graphics Infrastructure (DXGI) for enumerating graphics adapters and monitors, enumerating display modes, choosing buffer formats, creating swap chains and presenting rendered frames.
- Direct3D 11 for accessing the Direct3D device and context, as well as generating and populating other graphical resources (shaders and their related resources).

## DXGI in some detail

I was very brief when previously talking about DXGI, but for our purposes, what we're using it for comes down to two things:
- Getting the correct graphics adapter to use in conjunction with OpenXR.
- And from that, we will create the Direct3D 11 device and context that we'll use to generate graphics resources. This will include vertex and index buffers, shader fragments as well as swapchain formats.

DXGI allows us to identify what display adapters are currently attached to our system (as well as other things). Normally we have to do a fair bit of interrogation through DXGI to find the appropriate display adapter. However, OpenXR has a way to tell us what adapter to use. That's convenient for us and reduces our setup to what is essentially 2 lines of code (it's very barebones and can be made more robust over time).

## Direct3D 11 in some detail

Direct3D is a huge topic for conversation. And over these lessons, we'll investigate them more. However, what we're using Direct3D for is the following:
- Access to the Graphics card and graphics resources through a well documented API
- A robust set of 3D math functions that are high performance and industry standards
- A series of compilers for various shader subsystems, including Compute, Vertex and Pixel shaders

In a later secion, I'll do a 'Graphics Programming 101' where I cover some of the basic topics of how to render triangles to the screen. That said, there are a gamut of better resources to learn how to be a graphics programmer. I'll add some links in the resource section at the end of this lesson.

# Summary

We didn't dig into a lot of details here, but I wanted to give a review on what Direct3D 11 is. We also talked about how DXGI and Direct3D 11 fit into the system we will be building, at a very high level.

# Resources

Be aware that DirectX 11 went through a transition between 11 and 11.2. If you're looking for resources yourself, make sure you're looking for DirectX 11.1 and above.

- https://youtu.be/_4FArgOX1I4 - if you want something fairly lightweight for your introduction to graphics coding.
- https://learn.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11 - if you like reading, and it's the official source for documentation for DirectX
- https://www.braynzarsoft.net/viewtutorial/q16390-braynzar-soft-directx-11-tutorials - great set of tutorials
- https://gist.github.com/d7samurai/261c69490cce0620d0bfc93003cd1052 - Minimal D3D11 setup as a gist.
- https://www.rastertek.com/tutdx11s3.html
- There's also a tonne of books out there, if you really want to go that route.
