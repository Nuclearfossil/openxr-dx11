# Overview

Once we get past the nuances of getting our graphics subsystems working, we'll have another, larger API to get familiar with - OpenXR.

# So what exactly is OpenXR?

Lets first off talk about what OpenXR isn't:
- OpenXR isn't an Engine. It's an API.

With that out of the way, what is OpenXR?
- First off, it's an open, royalty-free API standard from the Khronos group that provides codebases with native access to a range of devices across various HMD providers.
- The API provides a hardware abstraction for core pose predictions, frame timing and spatial input functionality.
- It also exposes a set of cross vendor and vendor specific extensions that enable additional features beyond what is exposed by the OpenXR 1.0 core specification. For example:
	- eye tracking
	- spatial mapping
	- articulated hand tracking

You can consider extensions the most powerful aspect of the OpenXR specification. Based upon your needs, and the platform you are targeting, you can enable extensions in OpenXR to give you additional functionality.

As it stands, there are three kinds of OpenXR extensions:
1. Vendor extensions. These enable, based on the HMD vendor, access to specific features. For example, Foveated Gaze rendering on Oculus devices.
2. Cross-vendor extensions. These are extensions that multiple companies define and implement.
3. Official Khronos extensions. These are extensions that have been ratified by Khronos as part of the core specification release

For this series of tutorials, I've pre-built a static version of the OpenXR loader. However there are other ways of getting access to the OpenXR loader:
- There is a nuget package called *OpenXR.Loader* that you can add to your project.
- Include the official OpenXR loader source. You can pull and build the official source from the khronos [github repository](https://github.com/KhronosGroup/OpenXR-SDK)

# The OpenXR Loader?

First off, you have to consider that OpenXR is based upon a layered architecture. The following illustration may held shed light on the OpenXR architecture:

![[Pasted image 20230129212240.png]]

Each OpenXR application interfaces directly with the OpenXR loader, which in turn will detect, load and interact with any number of OpenXR Runtimes or API Layers.

The OpenXR Loader is the 'conductor' for dispatching OpenXR commands to the appropriate Runtime or API Layer. So, what does this conductor do?
- It must support one or more OpenXR Runtimes on a users device (computer, standalone device, etc)
- It has to support OpenXR API layers, and optional modules that are enabled by application developers
- It must strive to reduce the overall memory footprint and performance profile for an OpenXR application.

OpenXR Runtimes essentially map to one or more HMD devices.  You can have multiple Runtimes installed on a single system, but only one can be active at a given time. The OpenXR Loader discovers what Runtimes have been installed on the system and loads it as needed. nb: when you use the OpenXR command `xrCreateInstance`, the `XrInstance` that it returns is associated with the active OpenXR Runtime.

OpenXR API Layers are optional components that can intercept, evaluate, modify and insert existing OpenXR commands from the Application down to the Runtime. API Layers are implemented as libraries that are enabled in various ways. nb: When you call `XrCreateInstance`, all API layers will be enabled. Note that an API layer does not need to intercept all OpenXR commands.

# Summary

There's a lot to talk about when discussing OpenXR. At this point, we've introduced the concpets of the OpenXR Loader, Runtimes and API Layer.

In our next steps, we'll talk more about how we use the OpenXR commands to interact with a headset.

# Resources

- https://registry.khronos.org/OpenXR/specs/1.0/loader.html#openxr-loader The one-stop shop for all things OpenXR
- https://registry.khronos.org/OpenXR/ - Landing page for OpenXR at Khronos
- https://github.com/KhronosGroup/OpenXR-SDK - The OpenXR github repository