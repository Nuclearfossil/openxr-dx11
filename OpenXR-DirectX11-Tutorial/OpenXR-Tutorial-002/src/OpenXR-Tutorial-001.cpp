#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"D3dcompiler.lib")
#pragma comment(lib,"Dxgi.lib")

// Defines necessary to describe the platform we are building for:
// In this case, we're building for Windows.
// The full list can be found in openxr_platform.h.
// Some platforms available:
// - XR_USE_PLATFORM_ANDROID
// - XR_USE_PLATFORM_WIN32
// - XR_USE_PLATFORM_XLIB
// - XR_USE_PLATFORM_XCB
// - XR_USE_PLATFORM_WAYLAND
#define XR_USE_PLATFORM_WIN32

// Defines necessary for the underlying Graphics API
// In this case, DX11.
// The full list is in openxr_platform.h, but currently they are:
// - XR_USE_GRAPHICS_API_VULKAN
// - XR_USE_GRAPHICS_API_D3D11
// - XR_USE_GRAPHICS_API_D3D12
// - XR_USE_GRAPHICS_API_OPENGL_ES
// - XR_USE_GRAPHICS_API_OPENGL
// -
#define XR_USE_GRAPHICS_API_D3D11

#include <windows.h>
#include <d3d11.h>

#include "openxr\openxr.h"
#include "openxr\openxr_platform.h"

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP\

#include <vector>
#include <iostream>


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

        if (memcmp(&adapterDescription.AdapterLuid, &adapterLUID, sizeof(&adapterLUID)) == 0)
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

const char* XrSpaceToString(XrReferenceSpaceType spaceType)
{
    const char* result;
    switch (spaceType)
    {
    case XR_REFERENCE_SPACE_TYPE_VIEW:
        result = "View";
        break;
    case XR_REFERENCE_SPACE_TYPE_LOCAL:
        result = "Local";
        break;
    case XR_REFERENCE_SPACE_TYPE_STAGE:
        result = "Stage";
        break;
    case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT:
        result = "Unbounded MSFT";
        break;
    case XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO:
        result = "Combined Eye Varjo";
        break;
    default:
        result = "Undefined";
        break;
    }

    return result;
}

void LogXRFailure(XrInstance instance, XrResult result)
{
    char buffer[XR_MAX_RESULT_STRING_SIZE];
    xrResultToString(instance, result, buffer);
    LOG(ERROR) << "Unable to get the 'xrGetD3D11GraphicsRequirementsKHR' function pointer." << buffer;
}

bool LogExtensions()
{
	// First off, how many OpenXR Extension do we have access to?
	XrResult xrResult;
	uint32_t extensionCount = 0;
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to enumerate the Instance Extension Properties";
		return false;
	}

	if (extensionCount == 0)
	{
		LOG(ERROR) << "Failed to find any OpenXR extensions";
		return false;
	}

	// now that we know how many, let's actually get the enumerated list of available extensions
	std::vector<XrExtensionProperties> availableExtensions(extensionCount, { XR_TYPE_EXTENSION_PROPERTIES });
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, availableExtensions.data());
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to enumerate the Instance Extension Properties";
		return false;
	}

	// And let's see then
	LOG(INFO) << "Available extensions: " << extensionCount;
	for (auto& extension : availableExtensions)
	{
		LOG(INFO) << "Extension: " << extension.extensionName << " Version: " << extension.extensionVersion;
	}

	return true;
}

void LogApiLayers()
{
	uint32_t layerCount = 0;
	XrResult xrResult = xrEnumerateApiLayerProperties(0, &layerCount, nullptr);
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to enumerate the Instance Extension Properties";
		return;
	}

	LOG(INFO) << "Available API Layers: " << layerCount;
	if (layerCount > 0)
	{
		std::vector<XrApiLayerProperties> layers(layerCount, { XR_TYPE_API_LAYER_PROPERTIES });
		xrResult = xrEnumerateApiLayerProperties(layerCount, &layerCount, layers.data());
		if (xrResult != XR_SUCCESS)
		{
			LOG(ERROR) << "Failed to enumerate the Instance Extension Properties";
			return;
		}
		for (auto& property : layers)
		{
			LOG(INFO) << "Layer Name: " << property.layerName << " Version: " << property.layerVersion << " Spec Version: " << property.specVersion << " Desc: " << property.description;
		}
	}
}

bool LogXRInstance(XrResult xrResult, XrInstance instance)
{
    XrInstanceProperties xrInstanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
    xrResult = xrGetInstanceProperties(instance, &xrInstanceProperties);

    if (xrResult == XR_SUCCESS)
    {
        XrVersion runtimeVerstion = xrInstanceProperties.runtimeVersion;
        LOG(INFO) << "XR Instance runtime name: " << xrInstanceProperties.runtimeName << " runtime version:" << XR_VERSION_MAJOR(runtimeVerstion) << ":" << XR_VERSION_MINOR(runtimeVerstion) << ":" << XR_VERSION_PATCH(runtimeVerstion);
    }

    return xrResult == XR_SUCCESS;
}

bool LogSystemInfoAndProperties(XrResult xrResult, XrInstance instance, XrSystemId& systemID)
{
    XrSystemGetInfo systemGetInfo{ XR_TYPE_SYSTEM_GET_INFO };
    systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    xrResult = xrGetSystem(instance, &systemGetInfo, &systemID);
    if (xrResult == XR_SUCCESS)
    {
        LOG(INFO) << "For Form Factor: " << systemGetInfo.formFactor << " Got System ID: " << systemID;
    }

    XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES };
    xrResult = xrGetSystemProperties(instance, systemID, &systemProperties);
    if (xrResult == XR_SUCCESS)
    {
        LOG(INFO) << "System Name: " << systemProperties.systemName << " Vendor ID: " << systemProperties.vendorId;
        LOG(INFO) << "Max Swapchain Image Width: " << systemProperties.graphicsProperties.maxSwapchainImageWidth << " Height: " << systemProperties.graphicsProperties.maxSwapchainImageHeight << " Layer Count: " << systemProperties.graphicsProperties.maxLayerCount;
        LOG(INFO) << "Orientation Tracking: " << (systemProperties.trackingProperties.orientationTracking == XR_TRUE ? "Enabled" : "Disabled") << " Position Tracking: " << (systemProperties.trackingProperties.positionTracking == XR_TRUE ? "Enabled" : "Disabled");
    }

    return xrResult == XR_SUCCESS;
}

bool LogXRSpaces(XrSession session, XrResult& xrResult)
{
    uint32_t spaceCount = 0;
    xrEnumerateReferenceSpaces(session, 0, &spaceCount, nullptr);

    if (spaceCount > 0)
    {
        std::vector<XrReferenceSpaceType> spaces(spaceCount);
        xrResult = xrEnumerateReferenceSpaces(session, spaceCount, &spaceCount, spaces.data());
        if (xrResult != XR_SUCCESS)
        {
            LOG(ERROR) << "Failed to enumerate the reference spaces";
            return false;
        }

        for (auto refSpace : spaces)
        {
            LOG(INFO) << "Reference Space: " << XrSpaceToString(refSpace);
        }
    }

	return true;
}

bool CreateXRSession(ID3D11Device* d3dDevice, XrResult& xrResult, XrInstance instance, XrSession& session, XrSystemId systemID)
{
	// To get more data, we need to create a session
	// To create the session, we must identify a valid display subsystem
	XrGraphicsBindingD3D11KHR binding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
	binding.device = d3dDevice;

	XrSessionCreateInfo sessionCreateInfo{ XR_TYPE_SESSION_CREATE_INFO };
	sessionCreateInfo.systemId = systemID;
	sessionCreateInfo.next = &binding;

	xrResult = xrCreateSession(instance, &sessionCreateInfo, &session);
	if (xrResult != XR_SUCCESS)
	{
		LogXRFailure(instance, xrResult);
		return false;
	}

	return true;
}

int main()
{
	// What do we have for available extensions
	if (!LogExtensions()) return -1;

	// What API Layers are available to us?
	LogApiLayers();

	// So we don't get a crash on exit, we need to create and destroy the XR Instance.
    const char* necessaryExtensions[] =
	{
        XR_KHR_D3D11_ENABLE_EXTENSION_NAME, // Use Direct3D11 for rendering
        XR_EXT_DEBUG_UTILS_EXTENSION_NAME,  // Debug utils for extra info
    };

	XrInstanceCreateInfo xrInstanceCreateInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
	xrInstanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	xrInstanceCreateInfo.enabledExtensionCount = 2;
	xrInstanceCreateInfo.enabledExtensionNames = necessaryExtensions;
	strcpy_s(xrInstanceCreateInfo.applicationInfo.applicationName, "Tutorial");
	XrInstance instance;

	XrResult xrResult = xrCreateInstance(&xrInstanceCreateInfo, &instance);
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to create the XR Instance";
		return -1;
	}

	// Tell us about the XR Instance
	LogXRInstance(xrResult, instance);

	// Tell us something about the system
    XrSystemId systemID;
	LogSystemInfoAndProperties(xrResult, instance, systemID);

	// Initialize DirectX so we can get a D3D Device to play with
	// First off, we need to get the function pointer to the xrGetD3D11GraphicsRequirementsKHR extension
	PFN_xrGetD3D11GraphicsRequirementsKHR xrGetD3D11GraphicsRequirementsKHREXT = nullptr;
	xrResult = xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&xrGetD3D11GraphicsRequirementsKHREXT));
    if (xrResult != XR_SUCCESS)
    {
        LogXRFailure(instance, xrResult);
        return -1;
    }

    XrGraphicsRequirementsD3D11KHR requirement = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
    xrGetD3D11GraphicsRequirementsKHREXT(instance, systemID, &requirement);

    ID3D11Device*			d3dDevice = nullptr;
    ID3D11DeviceContext*	d3dContext = nullptr;

    IDXGIAdapter1* adapter = GetAdapter(requirement.adapterLuid);
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    if (adapter == nullptr)
    {
		LOG(ERROR) << "Failed to create the DXGI Adapter";
        return -1;
    }

    if (FAILED(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, &d3dDevice, nullptr, &d3dContext)))
    {
		LOG(ERROR) << "Failed to create the D3D11 Device";
        return -1;
    }

    adapter->Release();

    XrSession session;

    if (!CreateXRSession(d3dDevice, xrResult, instance, session, systemID))
		return -1;

    // What spaces does the hardware support?
	if (!LogXRSpaces(session, xrResult))
		return -1;

	d3dContext->Release();
	d3dDevice->Release();
	xrDestroySession(session);
	xrDestroyInstance(instance);

	return 0;
}
