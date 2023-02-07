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

int main()
{
	// What do we have for available extensions
	if (!LogExtensions()) return -1;

	// What API Layers are available to us?
	LogApiLayers();

	// So we don't get a crash on exit, we need to create and destroy the XR Instance.
	XrInstanceCreateInfo xrInstanceCreateInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
	xrInstanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	strcpy_s(xrInstanceCreateInfo.applicationInfo.applicationName, "Tutorial");
	XrInstance instance;

	XrResult xrResult = xrCreateInstance(&xrInstanceCreateInfo, &instance);
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to create the XR Instance";
		return -1;
	}

	// Tell us about the XR Instance
	XrInstanceProperties xrInstanceProperties {XR_TYPE_INSTANCE_PROPERTIES};
	xrResult = xrGetInstanceProperties(instance, &xrInstanceProperties);
	if (xrResult == XR_SUCCESS)
	{
		XrVersion runtimeVerstion = xrInstanceProperties.runtimeVersion;
		LOG(INFO) << "XR Instance runtime name: " << xrInstanceProperties.runtimeName << " runtime version:" << XR_VERSION_MAJOR(runtimeVerstion) << ":" << XR_VERSION_MINOR(runtimeVerstion) << ":" << XR_VERSION_PATCH(runtimeVerstion);
	}

	// Tell us something about the system
	XrSystemGetInfo systemGetInfo {XR_TYPE_SYSTEM_GET_INFO};
	systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	XrSystemId systemID;
	xrResult = xrGetSystem(instance, &systemGetInfo, &systemID);
	if (xrResult == XR_SUCCESS)
	{
		LOG(INFO) << "For Form Factor: " << systemGetInfo.formFactor << " Got System ID: " << systemID;
	}

	XrSystemProperties systemProperties {XR_TYPE_SYSTEM_PROPERTIES};
	xrResult = xrGetSystemProperties(instance, systemID, &systemProperties);
	if (xrResult == XR_SUCCESS)
	{
		LOG(INFO) << "System Name: " << systemProperties.systemName << " Vendor ID: "<< systemProperties.vendorId;
	    LOG(INFO) << "Max Swapchain Image Width: " << systemProperties.graphicsProperties.maxSwapchainImageWidth << " Height: " << systemProperties.graphicsProperties.maxSwapchainImageHeight << " Layer Count: " <<systemProperties.graphicsProperties.maxLayerCount;
		LOG(INFO) << "Orientation Tracking: " << (systemProperties.trackingProperties.orientationTracking == XR_TRUE ? "Enabled" : "Disabled") << " Position Tracking: " << (systemProperties.trackingProperties.positionTracking == XR_TRUE ? "Enabled" : "Disabled");
	}

	// To get more data, we need to create a session
	// To create the session, we must identify a valid display subsystem
	XrSession session;

	XrSessionCreateInfo sessionCreateInfo {XR_TYPE_SESSION_CREATE_INFO};
	sessionCreateInfo.systemId = systemID;
	xrResult = xrCreateSession(instance, &sessionCreateInfo, &session);
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to create the XR Session";
		return -1;
	}
	// What spaces does the hardware support?
	uint32_t spaceCount = 0;
	xrEnumerateReferenceSpaces(session, 0, &spaceCount, nullptr);

	if (spaceCount > 0)
	{
		std::vector<XrReferenceSpaceType> spaces(spaceCount);
		xrResult = xrEnumerateReferenceSpaces(session, spaceCount, &spaceCount, spaces.data());
		if (xrResult != XR_SUCCESS)
		{
			LOG(ERROR) << "Failed to enumerate the reference spaces";
			return -1;
		}
	}
	xrDestroySession(session);
	xrDestroyInstance(instance);

	return 0;
}
