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

int main()
{
	// What do we have for available extensions
	// First off, how many OpenXR Extension do we have access to?
	XrResult xrResult;
	uint32_t extensionCount = 0;
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to enumerate the Instance Extension Properties. ";
		return -1;
	}

	if (extensionCount == 0)
	{
		LOG(ERROR) << "Failed to find any OpenXR extensions";
		return -1;
	}

	// now that we know how many, let's actually get the enumerated list of available extensions
	std::vector<XrExtensionProperties> availableExtensions(extensionCount, { XR_TYPE_EXTENSION_PROPERTIES });
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, availableExtensions.data());
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to enumerate the Instance Extension Properties";
		return -1;
	}

	// And let's see then
	LOG(INFO) << "Available extensions: " << extensionCount;
	for (auto& extension : availableExtensions)
	{
		LOG(INFO) << "Extension: " << extension.extensionName << " Version: " << extension.extensionVersion;
	}

	XrInstanceCreateInfo xrInstanceCreateInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
	xrInstanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	strcpy_s(xrInstanceCreateInfo.applicationInfo.applicationName, "Tutorial");
	XrInstance instance;

	xrResult = xrCreateInstance(&xrInstanceCreateInfo, &instance);
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to create the XR Instance";
		return -1;
	}

	// Tell us about the XR Instance
	XrInstanceProperties xrInstanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
	xrResult = xrGetInstanceProperties(instance, &xrInstanceProperties);

	if (xrResult == XR_SUCCESS)
	{
		XrVersion runtimeVerstion = xrInstanceProperties.runtimeVersion;
		LOG(INFO) << "XR Instance runtime name: " << xrInstanceProperties.runtimeName << " runtime version:" << XR_VERSION_MAJOR(runtimeVerstion) << ":" << XR_VERSION_MINOR(runtimeVerstion) << ":" << XR_VERSION_PATCH(runtimeVerstion);
	}

// 	// Tell us something about the system
	 XrSystemGetInfo systemGetInfo{ XR_TYPE_SYSTEM_GET_INFO };
	 systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

	 XrSystemId systemID;
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

	xrDestroyInstance(instance);

	LOG(INFO) << "Successful OpenXR Tutorial Run";
	return 0;
}
