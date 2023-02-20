# Overview
In this section, we're actually looking at code. You can find all the code in the `OpenXR-Tutorial-001` Visual Studio project. In it, we will be reviewing the basic initialization of OpenXR, as well as covering in more details the elementary aspects of OpenXR. This project only displays information, to the console, about features available to you in OpenXR. We do not render anything to device.

# Our Project
## Project Setting
I like to have all my additional library includes and lib files in a globally accessible place. Thus, I tend to use package managers like [VCPKG](https://vcpkg.io/en/index.html) or [Conan](https://conan.io/) to manage all that. However, I don't want to force the reader to use one of those package managers, so I create two folders called `include` and `libs` to store those additional files:
![[Pasted image 20230210085201.png]]
![[Pasted image 20230210085219.png]]
If you're more comfortable using package managers, that's a far easier way to go.

In `OpenXR-Tutorial-001`, our project is set up to be a standard Windows Console app. It is not a true Windows app (no message pump, or windows creation events). All information we want will be written to the console, through a third party library called `EasyLogging` [github repo](https://github.com/amrayn/easyloggingpp). 

Initialization of Easylogging is pretty easy. Simply add the C++ source to your project, add the `easylogging++.h` header file, and add this macro to you source code in one spot:
``` C++
	INITIALIZE_EASYLOGGING\
```
And you're ready to start using it.

It's a data stream, like `std::cout`, where you can stream data to. So doing something like:
``` C++
LOG(INFO) << "You done screwed up " << (1 + 1);
```
will end up streaming to the console:
```
2023-02-09 21:52:01,583 INFO [default] You done screwed up 2
```
It will also generate a file called `myeasylog.log` to your disk, so you can always see what happened during each run.

editors note: easylogging is a bit of an older project. I may end up replacing it with something that's a bit more maintained in a future revision.

## Include Files and Set up
Aside from the include file for easylogging, you'll next need the header file for OpenXR. I've got a `libs` and 'include' global folder for additional libraried, including libs I've created for OpenXR. All the OpenXR headers live in an `openxr` folder, so my includes look like this:

``` C++
#include <windows.h>

#include "openxr\openxr.h"
#include "openxr\openxr_platform.h"

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP\

#include <vector>
#include <iostream>
```
If you've done any windows development, this will look familiar. Technically you do not need the include for `openxr_platform.h`, but we will be using it shortly, so I like to keep it for completeness.

Additionally, I also need to include libs into my project dependency for OpenXR. Here's my settings for that:
![[Pasted image 20230210085502.png]]
![[Pasted image 20230210085524.png]]

## The Main Loop

We can start off with something fairly trivial, like the following:
``` C++
int main()
{
	LOG(INFO) << "Successful OpenXR Tutorial Run";
	return 0;
}
```

Compiling and running this example will just spew the `Successful OpenXR Tutorial Run` text to the console, like so:

```
2023-02-10 09:01:55,206 INFO [default] Successful OpenXR Tutorial Run
```

If you've got a successfully compiling and running program at this point in time, we can now move on to adding OpenXR to this code.

## Determining Available OpenXR Extensions
In [[02 - Project Overview - OpenXR#^e6f366|this]] section, we discussed OpenXR extensions. What we would like to do next is actually determine what extensions we have available to us. To do that, we need to enumerate those extensions. It's actually really simple to do, through the `xrEnumerateInstanceExtensionProperties` OpenXR function. This will query the OpenXR runtime to determine what extensions are available, by name.

The pattern that we follow to use this function is:
- call the function with only one (or two) parameters so we can determine how many enumeratable objects there are, so we can allocate the appropriate number of buffers to hold the data.
- call the function again, but with the allocated buffers that the OpenXR function can put data into.

You'll see this pattern very often when enumerating OpenXR resources.

Thus, this is a relatively simple bit of code:
``` C++
	XrResult xrResult;
	uint32_t extensionCount = 0;
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to enumerate the Instance Extension Properties";
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
```
Let's break this one down, block by block starting with the following:
``` c++
	XrResult xrResult;
	uint32_t extensionCount = 0;
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to enumerate the Instance Extension Properties";
		return -1;
	}

	if (extensionCount == 0)
	{
		LOG(ERROR) << "Failed to find any OpenXR extensions";
		return -1;
	}
```

`xrEnumerateInstanceExtensionProperties` function signature looks like this:
```c++
// Provided by XR_VERSION_1_0
XrResult xrEnumerateInstanceExtensionProperties(
    const char*                                 layerName,
    uint32_t                                    propertyCapacityInput,
    uint32_t*                                   propertyCountOutput,
    XrExtensionProperties*                      properties);
```
Please note that we are using `nullptr` for both `layerName` and `properties`, and 0 for the `propertyCapacityInput` parameters. Again, in this call to `xrEnumerateInstanceExtensionProperties` we only want to get the number of properties that are available to us, so that we can pre-allocate the buffer to store the results into.

The function also returns an `XrResult` value that we can check again `XR_SUCCESS` to ensure a valid result has been returned. `XrResult` is an enumeration type that we can also check for specific failure types, or use `xrResultToString` to convert into a string version of that failure. We'll discuss using that in the future.

Therefore, when we call the following:
```c++
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
```
We're doing nothing more than populating `extensionCount` with the number of `XrExtensionProperties` that the OpenXR runtime knows about currently.

What we want to do next is allocate enough space to hold all the `XrExtensionProperties`.  Fortunately, we can use STL vectors to do that heavy lifting for us:
```c++
std::vector<XrExtensionProperties> availableExtensions(extensionCount, { XR_TYPE_EXTENSION_PROPERTIES });
```
What we're doing here is allocating, in that vector of `XrExtensionProperties`, `extensionCount` elements. We're also initializing each element in that array to be `XR_TYPE_EXTENSION_PROPERTIES`. That is a very, very important thing to remember about using OpenXR structures.

NOTE: If you use an OpenXR structure, you must ALWAYS define the data type as part of the structure. [[03.01 - Why OpenXR Structs Have to redefine their type|Details why here]]

Now that we have a buffer to hold the results, we call `XrEnumerateInstanceExtensionProperies` again, but with more information this time:
```c++
	xrResult = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, availableExtensions.data());
	if (xrResult != XR_SUCCESS)
	{
		LOG(ERROR) << "Failed to enumerate the Instance Extension Properties";
		return -1;
	}
```
As you can see above, we pass in the extension count into both `propertyCapacityInput` and `propertyCountOutput` to that the function knows how big our buffer is, as well as letting the function tell us how much of the buffer was used. This allows us to potentially allocate a really large initial buffer at start-up, and potentially not fill it when we call `xrEnumerateInstanceExtensionProperties`. 

We also check to make sure the result from calling `xrEnumerateInstanceExtensionProperties` was successful and handle any error conditions appropriately.

Finally, we iterate over the resultant vector and display some information about what extensions are available from the OpenXR Runtime.

```c++
	LOG(INFO) << "Available extensions: " << extensionCount;
	for (auto& extension : availableExtensions)
	{
		LOG(INFO) << "Extension: " << extension.extensionName << " Version: " << extension.extensionVersion;
	}
```
## Results so far
if we were to only implement this much of the code in main and run, we'd actually get this output:
```
2023-02-11 11:34:41,804 INFO [default] Available extensions: 26
2023-02-11 11:34:41,804 INFO [default] Extension: XR_KHR_D3D11_enable Version: 9
2023-02-11 11:34:41,804 INFO [default] Extension: XR_KHR_D3D12_enable Version: 9
2023-02-11 11:34:41,805 INFO [default] Extension: XR_KHR_opengl_enable Version: 10
2023-02-11 11:34:41,805 INFO [default] Extension: XR_KHR_vulkan_enable Version: 8
2023-02-11 11:34:41,805 INFO [default] Extension: XR_KHR_vulkan_enable2 Version: 2
2023-02-11 11:34:41,805 INFO [default] Extension: XR_KHR_composition_layer_depth Version: 6
2023-02-11 11:34:41,805 INFO [default] Extension: XR_KHR_win32_convert_performance_counter_time Version: 1
2023-02-11 11:34:41,805 INFO [default] Extension: XR_KHR_convert_timespec_time Version: 1
2023-02-11 11:34:41,805 INFO [default] Extension: XR_KHR_composition_layer_cube Version: 8
2023-02-11 11:34:41,805 INFO [default] Extension: XR_KHR_composition_layer_cylinder Version: 4
2023-02-11 11:34:41,805 INFO [default] Extension: XR_KHR_composition_layer_equirect Version: 3
2023-02-11 11:34:41,805 INFO [default] Extension: XR_KHR_visibility_mask Version: 2
2023-02-11 11:34:41,805 INFO [default] Extension: XR_KHR_composition_layer_color_scale_bias Version: 5
2023-02-11 11:34:41,805 INFO [default] Extension: XR_EXT_win32_appcontainer_compatible Version: 1
2023-02-11 11:34:41,805 INFO [default] Extension: XR_EXT_debug_utils Version: 4
2023-02-11 11:34:41,806 INFO [default] Extension: XR_OCULUS_recenter_event Version: 1
2023-02-11 11:34:41,806 INFO [default] Extension: XR_OCULUS_audio_device_guid Version: 1
2023-02-11 11:34:41,806 INFO [default] Extension: XR_FB_color_space Version: 3
2023-02-11 11:34:41,806 INFO [default] Extension: XR_FB_display_refresh_rate Version: 1
2023-02-11 11:34:41,806 INFO [default] Extension: XR_META_performance_metrics Version: 2
2023-02-11 11:34:41,806 INFO [default] Extension: XR_OCULUS_ovrsession_handle Version: 1
2023-02-11 11:34:41,806 INFO [default] Extension: XR_FB_haptic_amplitude_envelope Version: 1
2023-02-11 11:34:41,806 INFO [default] Extension: XR_FB_haptic_pcm Version: 1
2023-02-11 11:34:41,806 INFO [default] Extension: XR_FB_touch_controller_pro Version: 1
2023-02-11 11:34:41,806 INFO [default] Extension: XR_FB_touch_controller_proximity Version: 1
2023-02-11 11:34:41,806 INFO [default] Extension: XR_OCULUS_external_camera Version: 1
2023-02-11 11:34:41,806 INFO [default] Successful OpenXR Tutorial Run
```
One thing that may also happen if you've only implemented this much is that your program may actually crash with an Unhandled Exception error, like this one:
![[Pasted image 20230211113622.png]]
If you look closely at the exception, you'll see it's crashing in `LibOVRRT64_1.dll`. What's that DLL, you may be asking? [LibOVR](https://developer.oculus.com/documentation/native/pc/dg-libovr/) represents the Meta Native SDK for all supported devices. And it's included with all Meta VR devices. However, as of version 0.9 of this SDK, OpenXR has been in the library. with Version 1 (that's the `LibOVRRT64_1.dll`), OpenXR support is officially included.

So what's happening there is OpenXR (and Meta's libs) aren't getting cleaned up appropriately, so some handles (or pointers) are hanging around. We can clean that up pretty quickly by moving on to creating (and definining) an OpenXR Instance.

## XrInstance
An `XrInstance` is a type of [Handle](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#handles). An `XrInstance` is used to communicate with an OpenXR runtime. It stores and tracks OpenXR related state information, without having to have the Application actually track and store that information. For the most part (and for this example), we'll only ever create one `XrInstance` per application we create.

To create an `XrInstance` handle, we call the `xrCreateInstance` function to do so. Here's a sample of that code:

```c++
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
```
You're now seeing one of the first structures of the `xxxCreateInfo` types. These structures are used when you want to allocate Handles, Resources or other OpenXR objects. Like the structures we've seen before, they can be complex, but will start with two properties we've seen before, `XrStructureType type` and `const void* XR_MAY_ALIAS next`:
```c++
typedef struct XrInstanceCreateInfo {
    XrStructureType             type;
    const void* XR_MAY_ALIAS    next;
    XrInstanceCreateFlags       createFlags;
    XrApplicationInfo           applicationInfo;
    uint32_t                    enabledApiLayerCount;
    const char* const*          enabledApiLayerNames;
    uint32_t                    enabledExtensionCount;
    const char* const*          enabledExtensionNames;
} XrInstanceCreateInfo;
```

For our initial bit of code, all we are going to populate are the `type`, and `applicationInfo` parameters, like so:

```c++
XrInstanceCreateInfo xrInstanceCreateInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
xrInstanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
strcpy_s(xrInstanceCreateInfo.applicationInfo.applicationName, "Tutorial");
```

The `applicationInfo.apiVersion` indicates what OpenXR library version we intend to use, and the `applicationInfo.applicationName` is a null terminated string representing the OpenXR application name (the program identifier, as it were).

At a bare minimum, this is enough to create an XR Instance. It will be barely functional, and will only provide significantly limited functionality. At this time, we aren't adding full support, but will in a subsequent tutorial.

With that structure now populated, we can now go ahead and create the XR Instance using `xrCreateInstance`:

```c++
XrInstance instance;

xrResult = xrCreateInstance(&xrInstanceCreateInfo, &instance);
if (xrResult != XR_SUCCESS)
{
	LOG(ERROR) << "Failed to create the XR Instance";
	return -1;
}
```

What we have here, if the `xrCreateInstance` function succeeds, is an handle to the XR Instance that we can now use to access some functionality of the device. To fully access the HMD, you will need another Handle, for the XR Session. We will discuss this in an upcoming tutorial.

Now that we have the handle, we can let the OpenXR know that we are done with it by destroying the handle, via a call to `xrDestroyInstance`:

```c++
xrDestroyInstance(instance);
```

This function ensures that any OpenXR resources are released. This includes closing any references to `LibOVRRT64_1.dll`. If you now run the code, we will not get any more exceptions.

Here's what the `main` function would look like at this point:

```c++
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
	LogXRInstance(xrResult, instance);
	
	xrDestroyInstance(instance);
	
	LOG(INFO) << "Successful OpenXR Tutorial Run";
	return 0;
}
```

## Get information about  the XR Instance
Now, with the XR Instance handle, we can actually get a bit more information. There is a function called `xrGetInstanceProperties` that does pretty much exactly what you would expect it to do. Here's what that code would look like:
```c++
XrInstanceProperties xrInstanceProperties{ XR_TYPE_INSTANCE_PROPERTIES };
xrResult = xrGetInstanceProperties(instance, &xrInstanceProperties);

if (xrResult == XR_SUCCESS)
{
	XrVersion runtimeVerstion = xrInstanceProperties.runtimeVersion;
	LOG(INFO) << "XR Instance runtime name: " << xrInstanceProperties.runtimeName << " runtime version:" << XR_VERSION_MAJOR(runtimeVerstion) << ":" << XR_VERSION_MINOR(runtimeVerstion) << ":" << XR_VERSION_PATCH(runtimeVerstion);
}
```
`XrInstanceProperties` is another structure, and it should be starting to look familiar at this point in time:
```c++
typedef struct XrInstanceProperties {
    XrStructureType       type;
    void* XR_MAY_ALIAS    next;
    XrVersion             runtimeVersion;
    char                  runtimeName[XR_MAX_RUNTIME_NAME_SIZE];
```
You should now be able to infer that `XrGetInstanceProperties` will populate the `XrInstanceProperties` structure with the runtime version and a null terminated string representing the runtime Name.  And we use that data to log to the console

This workflow should not be starting to look familiar.
1. create an OpenXR structure for passing parameters into OpenXR functions
2. call the OpenXR function
3. check to ensure the result from the function call are a success
4. Process the data that's returned

## XR System ID
There are two more bits of information that we're going to want to access here in order to fully use OpenXR. The XR System ID and the XR Session handle. In our next tutorial, we'll touch on the XR Session handle, but we should take some time to discuss the `XrSystemId`.

The XR System ID is an 'atom' used by the OpenXR runtime to identify a system, based upon the form factor of the device. Currently there are only two form factors supported:
- Head Mounted Displays (HMD) that are, like the name says, displays that are attached to the users head and tracked in 3D space.
- A Handheld Displays is held in the user's hand and is independent from the users head. It may also support touch for On-screen UI. This is more in line with an Android device running an AR experience.

How do we get this session? Again, the code is straightforward:

```c++
 XrSystemGetInfo systemGetInfo{ XR_TYPE_SYSTEM_GET_INFO };
 systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

 XrSystemId systemID;
 xrResult = xrGetSystem(instance, &systemGetInfo, &systemID);
 if (xrResult == XR_SUCCESS)
 {
	 LOG(INFO) << "For Form Factor: " << systemGetInfo.formFactor << " Got System ID: " << systemID;
 }
```

Same steps as before: 
- create a structure to set/get information from an OpenXR function. 
- populate that function with some initial settings (in this case, that we want to get the XR System ID for a HMD)
- and call the OpenXR function to populate the data.

Now that we have the system ID, we can now use that ID to get some properties from the system.

```c++
 XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES };
 xrResult = xrGetSystemProperties(instance, systemID, &systemProperties);
 if (xrResult == XR_SUCCESS)
 {
	 LOG(INFO) << "System Name: " << systemProperties.systemName << 
				 " Vendor ID: " << systemProperties.vendorId;
	 LOG(INFO) << "Max Swapchain Image Width: " << 
				 systemProperties.graphicsProperties.maxSwapchainImageWidth << 
				 " Height: " <<
				 systemProperties.graphicsProperties.maxSwapchainImageHeight << 
				 " Layer Count: " << systemProperties.graphicsProperties.maxLayerCount;
	 LOG(INFO) << "Orientation Tracking: " << 
				 (systemProperties.trackingProperties.orientationTracking == XR_TRUE ?
				  "Enabled" : "Disabled") << 
				  " Position Tracking: " << 
				  (systemProperties.trackingProperties.positionTracking == XR_TRUE ? 
				  "Enabled" : "Disabled");
 }
```

As long as the call succeds, we can now determine if the XR System supports orientation and position tracking, and various Graphics capacities of the hardware, like number of compositor layers as well as swapchain information.

# Summary
Everything that we've touched on can be found in the source code in the Visual Studio project `OpenXR-Tutorial-001`.

We've covered a fairly broad amount of content in this tutorial, and haven't even gotten to rendering anything onto the screen. We're still a way aways from that, but it's coming.

# Resources
- [Easy Logging Github Repo](https://github.com/amrayn/easyloggingpp) - Github Repo for EasyLogging lib
- [OpenXR Reference](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#introduction) The OpenXR reference docs on Khronos