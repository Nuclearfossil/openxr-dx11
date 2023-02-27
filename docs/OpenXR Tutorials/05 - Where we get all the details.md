# Overview
At the end of this section, we'll _almost_ be to a point where we can start rendering objects to device.

Almost.

What we're going to do in this section is talk more about finalizaing OpenXR's initialization. This also includes discussing actually initializing Direct 3D 11. So there will be a few tangents here and there to talk about D3D and DXGI matters.

# What's left to do
As part of our next batch of topics, we now have to consider how we go about finishing up OpenXR initialization. One thing we have not covered is how OpenXR and Direct 3D 11 interact with each other. We'll try and cover that now.

# Assumptions

Before we go any further, I'm going to assume the following:
- You're relatively familiar with elementary rendering concepts. This includes, but is not limited to:
	- D3D Devices and Contexts
	- Swapchains
	- Shaders
If you aren't, I would recommend spending some time looking into basic rendering techniques, either with Modern OpenGL or DirectX 11. It goes beyond the scope of this series.

# Updates for OpenXR
## OpenXR 
One of the most important things we are going to need for OpenXR is to define a rendering system to use in order to create a rendering swapchain. As I've stated above, we're using DirectX 11 for our rendering system.

The OpenXR runtime can tell you what support is available given a specific Graphics API. At the time of this writing, OpenXR supports the following Graphics APIs:
- DirectX 11
- DirectX 12
- OpenGL (desktop and GLES)
- Vulkan

In order to determine what Graphics APIs are supported on your current platform, there are a number of OpenXR extension functions available to determine what graphics device can be used as well as any additional information necessary for that device. 

That may be a little confusing, so let's try and clarify it with an example. Say you have a desktop PC with two (or more) dedicated graphics cards (eg: nVidia and Intel on your laptop). You'll only have your headset plugged into the nVidia card, as the Intel graphics card will probably drive the laptop's LCD. In order to get the correct device, the OpenXR runtime will expose a function, as an extension, that will allow your application to detect the correct device to connect to.

All OpenXR graphics interaction functionality are exposed through a set of extensions. For D3D11, this is done through the use of the [XR_KHR_D3D11_enable](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_D3D11_enable) extension. 

Before you can use *any* graphics API, you will need to query support for that specific API via a specific extension call:

| Graphics API | OpenXR Extension | OpenXR Extension Function |
|---|---|---|
| Direct3D 11 | [XR_KHR_D3D11_enable](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_D3D11_enable) | [xrGetD3D11GraphicsRequirementsKHR](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#xrGetD3D11GraphicsRequirementsKHR) |
| Direct3D 12 | [XR_KHR_D3D12_enable](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_D3D12_enable) | [xrGetD3D12GraphicsRequirementsKHR](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#xrGetD3D12GraphicsRequirementsKHR) |
| OpenGL | [XR_KHR_opengl_enable](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_opengl_enable) | [xrGetOpenGLGraphicsRequirementsKHR](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#xrGetOpenGLGraphicsRequirementsKHR) |
| OpenGL ES | [XR_KHR_opengl_es_enable](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_opengl_es_enable) | [xrGetOpenGLESGraphicsRequirementsKHR](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#xrGetOpenGLESGraphicsRequirementsKHR) |
| Vulkan | [XR_KHR_vulkan_enable](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_vulkan_enable) | [xrGetVulkanGraphicsRequirementsKHR](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#xrGetVulkanGraphicsRequirementsKHR) |
| Vulkan 2 | [XR_KHR_vulkan_enable2](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_vulkan_enable2) | [xrGetVulkanGraphicsRequirements2KHR](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#xrGetVulkanGraphicsRequirements2KHR) |

Digging into the Direct3D 11 call, the function call looks like this:
``` c++
XrResult xrGetD3D11GraphicsRequirementsKHR( 
	XrInstance instance, 
	XrSystemId systemId, 
	XrGraphicsRequirementsD3D11KHR* graphicsRequirements);
```
We've covered the `XrInstance` and `XrSystemId` previously, so what do we get back from the `XrGraphicsRequirementsD3D11KHR` structure? Well, the structure is defined like so:

``` c++
typedef struct XrGraphicsRequirementsD3D11KHR 
{ 
	XrStructureType type; 
	void* next; 
	LUID adapterLuid; 
	D3D_FEATURE_LEVEL minFeatureLevel; 
} XrGraphicsRequirementsD3D11KHR;
 ```

`XrStructureType` must be set to `XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR` and `next` should be `NULL`.

However, the `adapterLuid` will represent the D3D11 LUID of the graphics adapter to use. Finally, the `minFeatureLevel` will define the minimum `D3D_FEATURE_LEVEL` (which is defined as part of Direct3D in `d3dcommon.h` as part of the Windows SDK) that must be used. 

### A Gotcha
Notice that I mentioned that the above is part of an extension. So if you were to try and call the `xrGetD3D11GraphicsRequirementsKHR` function, you'd get a compile error!

```
error C3861: 'XrGetD3D11GraphicsRequirementsKHR': identifier not found
```

All Extension functions are defined as function pointers. That is, we bind to those functions based upon where they live in the given runtime. That is to say those extension functions are defined as functions in a DLL (Windows) or Symbolicly linked library (Android/linux). To actually get that function pointer, we need to use the appropriate [Function Pointer Library](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#function-pointers) call to get access to that function via the [xrGetInstanceProcAddr](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#xrGetInstanceProcAddr) function.

So, why don't we do this for all OpenXR functions? Technically we can, but, according to the OpenXR docs:
> [xrGetInstanceProcAddr](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#xrGetInstanceProcAddr) itself is obtained in a platform- and loader- specific manner. Typically, the loader library will export this function as a function symbol, so applications **can** link against the loader library, or load it dynamically and look up the symbol using platform-specific APIs. Loaders **must** export function symbols for all core OpenXR functions. Because of this, applications that use only the core OpenXR functions have no need to use [xrGetInstanceProcAddr](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#xrGetInstanceProcAddr).

In our case, to get access to the `XrGetD3D11GraphicsRequirementsKHR` function, we do this, with a valid XR Instance:
``` c++
PFN_xrGetD3D11GraphicsRequirementsKHR xrGetD3D11GraphicsRequirementsKHREXT = nullptr;

XrResult xrResult = xrGetInstanceProcAddr(instance, 
				  "xrGetD3D11GraphicsRequirementsKHR", 
				  (PFN_xrVoidFunction*)(&xrGetD3D11GraphicsRequirementsKHREXT));

if (xrResult != XR_SUCCESS)
{
	LogXRFailure(instance, xrResult, "Failed to get the D3D 11 Graphics requirements.");
	return true;
}
```
In the above example, I create a function pointer called `xrGetD3D11GraphicsRequirementsKHREXT` that I will use.

And, to use it, I'll call it like so:
``` c++
XrGraphicsRequirementsD3D11KHR requirement = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
xrResult = xrGetD3D11GraphicsRequirementsKHREXT(instance, systemID, &requirement);

if (xrResult != XR_SUCCESS)
{
	LogXRFailure(instance, xrResult, "Failed to get the KHR D3D11 graphics requirements");
	return false;
}
```

We now have a `requirement` variable with the appropriate data populated. We can now get at the `requirement.adapterLuid` to see what device adapter we should be using. We can get that D3D Device using DXGI to enumerate the adapters to locate the matching `LUID`. I've written a little utility function called `GetAdapter` that can identify the device to use when we create the D3D device.

The logic for creating the D3D Device looks like this:
``` c++
IDXGIAdapter1* adapter = GetAdapter(requirement.adapterLuid);
D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

if (adapter == nullptr)
{
	LOG(ERROR) << "Failed to create the DXGI Adapter";
	return true;
}

if (FAILED(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, d3dDevice, nullptr, d3dContext)))
{
	LOG(ERROR) << "Failed to create the D3D11 Device";
	return true;
}

adapter->Release();
```
For completeness, this is what `GetAdapter` looks like:
``` c++
IDXGIAdapter1* GetAdapter(LUID& adapterLUID)
{
    // find the appropriate DXGI adapter give a specific adapter LUID
    IDXGIAdapter1* foundAdapter = nullptr;
    IDXGIAdapter1* currentAdapter = nullptr;
    IDXGIFactory1* dxgiFactory;
    DXGI_ADAPTER_DESC1 adapterDescription;

    CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&dxgiFactory));

    int curr = 0;
    while (dxgiFactory->EnumAdapters1(curr++, &currentAdapter) == S_OK)
    {
        currentAdapter->GetDesc1(&adapterDescription);

        if (memcmp(&adapterDescription.AdapterLuid, &adapterLUID, 
	        sizeof(&adapterLUID)) == 0)
        {
            foundAdapter = currentAdapter;
            break;
        }
        currentAdapter->Release();
        currentAdapter = nullptr;
    }
    dxgiFactory->Release();
    return foundAdapter;
}
```

The flow of `GetAdapter` is fairly straightforward:
- Create a DXGI Factor for enumerating the connected Adapters (and, arguably, the display devices - monitors, that are connected) and get their capabilities.
- From the DXGI Factory, enumerate over all the connected adapters.
	- Check to see if the Adapter LUID matches the one passed into `GetAdapter` using a `memcmp` comparison function.
	- If they match, we've found the matching adapter and prepare to return it as an IDGIIAdapter1 pointer.
- Release the Factory
- return the found IDGIIAdapter1 pointer, or `nullptr` if none was found.

## XR Session
 
## Graphics
In a previous episode, I mentioned that in any typical OpenXR structure, we have two common properties:
```c++
typedef struct XrSomeStructCreateInfo {
    XrStructureType             type;
    const void* XR_MAY_ALIAS    next;
```
In the past, we've taked at length about the `type` property whenever we are using an OpenXR structure, but we very much glossed over the `next` property, and chaining calls.

We can use the `next` to add additional structures into the that structure for additional OpenXR runtimes to use. 

# Summary
Everything that we've touched on can be found in the source code in the Visual Studio project `OpenXR-Tutorial-003`.

# Resources
