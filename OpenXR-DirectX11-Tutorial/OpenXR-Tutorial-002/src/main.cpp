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

#include <windows.h>

#include "openxr\openxr.h"
#include "openxr\openxr_platform.h"

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP\

#include <vector>
#include <iostream>

void LogXRFailure(XrInstance instance, XrResult result, const char* message)
{
	char buffer[XR_MAX_RESULT_STRING_SIZE];
	xrResultToString(instance, result, buffer);
	LOG(ERROR) << message << " : " << buffer;
}

bool LogExtensions()
{
	// First off, how many OpenXR Extension do we have access to?
	XrResult xrResult;
	uint32_t extensionCount = 0;
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
	if (xrResult != XR_SUCCESS)
	{
		LogXRFailure(nullptr, xrResult, "Failed to enumerate the Instance Extension Properties. ");
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
		LogXRFailure(nullptr, xrResult, "Failed to enumerate the Instance Extension Properties");
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
		LogXRFailure(nullptr, xrResult, "Failed to enumerate the Instance Extension Properties");
		return;
	}

	LOG(INFO) << "Available API Layers: " << layerCount;
	if (layerCount > 0)
	{
		std::vector<XrApiLayerProperties> layers(layerCount, { XR_TYPE_API_LAYER_PROPERTIES });
		xrResult = xrEnumerateApiLayerProperties(layerCount, &layerCount, layers.data());
		if (xrResult != XR_SUCCESS)
		{
			LogXRFailure(nullptr, xrResult, "Failed to enumerate the Instance Extension Properties");
			return;
		}
		for (auto& property : layers)
		{
			LOG(INFO) << "Layer Name: " << property.layerName << " Version: " << property.layerVersion << " Spec Version: " << property.specVersion << " Desc: " << property.description;
		}
	}
}

bool CreateXRInstance(XrInstance& instance)
{
	XrInstanceCreateInfo xrInstanceCreateInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
	xrInstanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	strcpy_s(xrInstanceCreateInfo.applicationInfo.applicationName, "Tutorial");

	XrResult xrResult = xrCreateInstance(&xrInstanceCreateInfo, &instance);
	if (xrResult != XR_SUCCESS)
	{
		LogXRFailure(instance, xrResult, "Failed to create the XR Instance");
		return false;
	}
	return true;
}

bool LogXRInstance(XrInstance instance)
{
	XrResult xrResult;
    XrInstanceProperties xrInstanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
    xrResult = xrGetInstanceProperties(instance, &xrInstanceProperties);

	if (xrResult != XR_SUCCESS)
	{
		LogXRFailure(instance, xrResult, "Failed to get the XR Instance Properties.");
		return false;
	}

	XrVersion runtimeVerstion = xrInstanceProperties.runtimeVersion;
    LOG(INFO) << "XR Instance runtime name: " << xrInstanceProperties.runtimeName << " runtime version:" << XR_VERSION_MAJOR(runtimeVerstion) << ":" << XR_VERSION_MINOR(runtimeVerstion) << ":" << XR_VERSION_PATCH(runtimeVerstion);

    return false;
}

bool GetSystemIdAndLogProperties(XrInstance instance, XrSystemId& systemID)
{
	XrResult xrResult;
    XrSystemGetInfo systemGetInfo{ XR_TYPE_SYSTEM_GET_INFO };
    systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    xrResult = xrGetSystem(instance, &systemGetInfo, &systemID);
    if (xrResult == XR_SUCCESS)
    {
        LOG(INFO) << "For Form Factor: " << systemGetInfo.formFactor << " Got System ID: " << systemID;
    }

	if (systemID == XR_NULL_SYSTEM_ID)
	{
		LOG(ERROR) << "Failed to get a valid system ID";
		return false;
	}

    XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES };
    xrResult = xrGetSystemProperties(instance, systemID, &systemProperties);
	if (xrResult != XR_SUCCESS)
	{
		LogXRFailure(instance, xrResult, "Failed to get the XR System Properties");
		return false;
	}

	LOG(INFO) << "System Name: " << systemProperties.systemName << " Vendor ID: " << systemProperties.vendorId;
    LOG(INFO) << "Max Swapchain Image Width: " << systemProperties.graphicsProperties.maxSwapchainImageWidth << " Height: " << systemProperties.graphicsProperties.maxSwapchainImageHeight << " Layer Count: " << systemProperties.graphicsProperties.maxLayerCount;
    LOG(INFO) << "Orientation Tracking: " << (systemProperties.trackingProperties.orientationTracking == XR_TRUE ? "Enabled" : "Disabled") << " Position Tracking: " << (systemProperties.trackingProperties.positionTracking == XR_TRUE ? "Enabled" : "Disabled");

    return false;
}

int main()
{
	// What do we have for available extensions
	if (!LogExtensions()) return -1;

	// What API Layers are available to us?
	LogApiLayers();

	XrInstance instance;
	if (!CreateXRInstance(instance)) return -1;

	// Tell us about the XR Instance
	LogXRInstance(instance);

	// Tell us something about the system
    XrSystemId systemID;
	if (!GetSystemIdAndLogProperties(instance, systemID))
		return -1;

	xrDestroyInstance(instance);

    LOG(INFO) << "Successful OpenXR Tutorial Run";
	return 0;
}
