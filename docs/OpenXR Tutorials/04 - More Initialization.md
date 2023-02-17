# Overview
In this section, we'll cover some additional OpenXR initialization elements while Improving the structure and functionality of our codebase.
# One thing we didn't do
When talking about [[02 - Project Overview - OpenXR|OpenXR]], we mentioned Extensions  and API Layers, but only enumerated the Extensions that were available. I didn't go into details about API Layers as we don't currently have any API layers to work with. We will discuss them in more detail in the future, but for completeness sake, I'll show you the code that will allow you to enumerate them.

```c++
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
		LOG(INFO) << "Layer Name: " << property.layerName << " Version: " << 
		property.layerVersion << " Spec Version: " << property.specVersion << 
		" Desc: " << property.description;
	}
}
```
As you've seen in the past, it follows a very similar workflow for enumerating OpenXR data:
1. If we are calling an OpenXR enumeration function, call the function with only one (or two) parameters so we can determine how many enumeratable objects there are, so we can allocate the appropriate number of buffers to hold the data.
2. call the function again, but with the allocated buffers that the OpenXR function can put data into.
Please note that if you now add this code to our project, we don't actually see any additional API layers avaialble (unless you've added some yourself). That's OK, as we'll get to that in the future.
# Code Cleanup
Right now, what we have is a pretty lumpy piece of code. Let's take the code we have and break it down into something that's a little more friendly to work with. Let's create a few new functons called:
```c++
bool LogExtensions();
void LogApiLayers();
bool CreateXRInstance(XrInstance& instance);
bool LogXRInstance(XrInstance instance);
bool GetSystemIdAndLogProperties(XrInstance instance, XrSystemId& systemID);
```
And inside of these functions, let's add the code that would be appropriate for those sections. Rather than adding the body of that work here, please reference the Visual Studio project `OpenXR-Tutorial-002`.

We should have a `main` function that looks a lot simpler:
```c++
int main()
{
	// What do we have for available extensions
	if (!LogExtensions()) return -1;

	// What API Layers are available to us?
	LogApiLayers();

	XrInstance instance;
	if (!CreateXRInstance(instance)) return -1;

	// Tell us about the XR Instance
	XrResult xrResult = XR_SUCCESS;
	LogXRInstance(xrResult, instance);

	// Tell us something about the system
    XrSystemId systemID;
	if (!GetSystemIdAndLogProperties(instance, systemID))
		return -1;

	xrDestroyInstance(instance);

    LOG(INFO) << "Successful OpenXR Tutorial Run";
	return 0;
}
```
# More Details
Right now, our error logging is still pretty haphazzard. We can do a fair bit to improve it, with just a couple of tweaks. If we call an OpenXR function that returns an `XrResult`, we can actually use an OpenXR function to get an error buffer reporting on what has actually failed. That function is called `xrResultToString`. Here's a fairly straightforward implementation:
```c++
void LogXRFailure(XrInstance instance, XrResult result, const char* message)
{
    char buffer[XR_MAX_RESULT_STRING_SIZE];
    xrResultToString(instance, result, buffer);
    LOG(ERROR) << message << " : " << buffer;
}
```
Now, whenever we actually have a failure, we can get a verbose description now of what failed.
So, wherever we see a test like this:
```c++
if (xrResult != XR_SUCCESS)
{
	LOG(ERROR) << "Failed to enumerate the Instance Extension Properties";
	return false;
}
```
You can extend your error information by chaging one line:
```c++
if (xrResult != XR_SUCCESS)
{
	LogXRFailure(instance, xrResult, "Failed to enumerate the Instance Extension Properties");
	return false;
}
```
This isn't the only way to improve debugging, and we'll detail that in a future talk.

# One Additional thing to note
When we're looking for the XR System ID, there is one possible value that we need to check against. It's entirely possible for `xrGetSystem` to return a success code, but actually have the `XrSystemId` be invalid. So we can do a quick test against the returned `XrSystemId` with the value `XR_NULL_SYSTEM_ID`. If you ever see that value for a XR System ID, the device is either not connected, or if you're on PC, the Link Cable is not attached or working.

This is also a great way to early-out in your application to say "no, I don't actually have a device connected".

# Summary
We're almost there, to get all the information about the device that is attached to your PC. There's still one more bit that requires us to do some D3D 11 investigation. We'll do that in our next session.

# Resources
- [OpenXR Reference](https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#introduction) The OpenXR reference docs on Khronos