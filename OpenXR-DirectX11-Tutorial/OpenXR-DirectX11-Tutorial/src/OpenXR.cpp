#include "TutorialStructs.h"
#include "OpenXR.h"
#include "D3DRenderer.h"
#include "Application.h"

#include "easylogging++.h"

const XrPosef				poseIdentity = { {0,0,0,1}, {0,0,0} };
XrInstance					instance = {};
XrSession					session = {};
XrSessionState				sessionState = XR_SESSION_STATE_UNKNOWN;
bool						isRunning = false;
XrSpace						applicationSpace = {};
XrSystemId					systemID = XR_NULL_SYSTEM_ID;
InputState					xrInput = { };
XrEnvironmentBlendMode		blendMode = {};
XrDebugUtilsMessengerEXT	debugMessenger = {};

std::vector<XrView>						views;
std::vector<XrViewConfigurationView>	configViews;
std::vector<Swapchain>					swapchains;

// Function pointers for some OpenXR extension methods we'll use.
PFN_xrGetD3D11GraphicsRequirementsKHR xrGetD3D11GraphicsRequirementsKHREXT = nullptr;
PFN_xrCreateDebugUtilsMessengerEXT    xrCreateDebugUtilsMessengerEXT = nullptr;
PFN_xrDestroyDebugUtilsMessengerEXT   xrDestroyDebugUtilsMessengerEXT = nullptr;

XrFormFactor            hmdFormFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
XrViewConfigurationType hmdViewConfiguration = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;


bool OpenXR::Init(const char* appName, int64_t swapchainFormat) 
{
	// OpenXR will fail to initialize if we ask for an extension that OpenXR
	// can't provide! So we need to check our all extensions before 
	// initializing OpenXR with them. Note that even if the extension is 
	// present, it's still possible you may not be able to use it. For 
	// example: the hand tracking extension may be present, but the hand
	// sensor might not be plugged in or turned on. There are often 
	// additional checks that should be made before using certain features!
	std::vector<const char*> extensionToUse;
	const char* necessaryExtensions[] = {
		XR_KHR_D3D11_ENABLE_EXTENSION_NAME, // Use Direct3D11 for rendering
		XR_EXT_DEBUG_UTILS_EXTENSION_NAME,  // Debug utils for extra info
	};

	// We'll get a list of extensions that OpenXR provides using this 
	// enumerate pattern. OpenXR often uses a two-call enumeration pattern 
	// where the first call will tell you how much memory to allocate, and
	// the second call will provide you with the actual data!
	uint32_t extensionCount = 0;
	xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
	std::vector<XrExtensionProperties> availableExtensions(extensionCount, { XR_TYPE_EXTENSION_PROPERTIES });
	xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, availableExtensions.data());

	printf("OpenXR extensions available:\n");
	for (size_t i = 0; i < availableExtensions.size(); i++) 
	{
		LOG(INFO) << availableExtensions[i].extensionName;

		// Check if we're asking for this extensions, and add it to our use list of extensions to use
		for (int32_t ask = 0; ask < _countof(necessaryExtensions); ask++) 
		{
			if (strcmp(necessaryExtensions[ask], availableExtensions[i].extensionName) == 0) 
			{
				extensionToUse.push_back(necessaryExtensions[ask]);
				break;
			}
		}
	}
	// If a required extension isn't present, you want to ditch out here!
	// It's possible something like your rendering API might not be provided
	// by the active runtime. APIs like OpenGL don't have universal support.
	if (!std::any_of(
			extensionToUse.begin(), 
			extensionToUse.end(), 
			[](const char* ext) 
			{
				return strcmp(ext, XR_KHR_D3D11_ENABLE_EXTENSION_NAME) == 0;
			})
		)
		return false;

	// Initialize OpenXR with the extensions we've found!
	XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionToUse.size());
	createInfo.enabledExtensionNames = extensionToUse.data();
	createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	strcpy_s(createInfo.applicationInfo.applicationName, appName);
	XrResult result = xrCreateInstance(&createInfo, &instance);

	// Check if OpenXR is on this system, if this is null here, the user 
	// needs to install an OpenXR runtime and ensure it's active!
	if (instance == nullptr)
		return false;

	// Load extension methods that we'll need for this application! There's a
	// couple ways to do this, and this is a fairly manual one. Chek out this
	// file for another way to do it:
	// https://github.com/maluoi/StereoKit/blob/master/StereoKitC/systems/platform/openxr_extensions.h
	xrGetInstanceProcAddr(instance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&xrCreateDebugUtilsMessengerEXT));
	xrGetInstanceProcAddr(instance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)(&xrDestroyDebugUtilsMessengerEXT));
	xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)(&xrGetD3D11GraphicsRequirementsKHREXT));

	// Set up a really verbose debug log! Great for dev, but turn this off or
	// down for final builds. WMR doesn't produce much output here, but it
	// may be more useful for other runtimes?
	// Here's some extra information about the message types and severities:
	// https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#debug-message-categorization
	XrDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = { XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debugMessengerCreateInfo.messageTypes =
		XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
	debugMessengerCreateInfo.messageSeverities =
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugMessengerCreateInfo.userCallback = [](XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT types, const XrDebugUtilsMessengerCallbackDataEXT* msg, void* user_data) 
	{
		// Print the debug message we got! There's a bunch more info we could
		// add here too, but this is a pretty good start, and you can always
		// add a breakpoint this line!
		LOG(ERROR) << msg->functionName << ": " << msg->message;

		// Output to debug window
		char text[512];
		sprintf_s(text, "%s: %s", msg->functionName, msg->message);
		OutputDebugStringA(text);

		// Returning XR_TRUE here will force the calling function to fail
		return (XrBool32)XR_FALSE;
	};
	// bind the above debug message handler to OpenXR
	if (xrCreateDebugUtilsMessengerEXT)
		xrCreateDebugUtilsMessengerEXT(instance, &debugMessengerCreateInfo, &debugMessenger);

	// Request a form factor from the device (HMD, Handheld, etc.)
	XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
	systemInfo.formFactor = hmdFormFactor;
	xrGetSystem(instance, &systemInfo, &systemID);

	// Check what blend mode is valid for this device (opaque vs transparent displays)
	// We'll just take the first one available!
	uint32_t blendCount = 0;
	xrEnumerateEnvironmentBlendModes(instance, systemID, hmdViewConfiguration, 1, &blendCount, &blendMode);

	// OpenXR wants to ensure apps are using the correct graphics card, so this MUST be called 
	// before xrCreateSession. This is crucial on devices that have multiple graphics cards, 
	// like laptops with integrated graphics chips in addition to dedicated graphics cards.
	XrGraphicsRequirementsD3D11KHR requirement = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
	xrGetD3D11GraphicsRequirementsKHREXT(instance, systemID, &requirement);
	if (!D3DRenderer::Init(requirement.adapterLuid))
		return false;

	// A session represents this application's desire to display things! This is where we hook up our graphics API.
	// This does not start the session, for that, you'll need a call to xrBeginSession, which we do in openxr_poll_events
	XrGraphicsBindingD3D11KHR binding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
	binding.device = D3DRenderer::GetDevice();
	XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
	sessionInfo.next = &binding;
	sessionInfo.systemId = systemID;
	xrCreateSession(instance, &sessionInfo, &session);

	// Unable to start a session, may not have an MR device attached or ready
	if (session == nullptr)
		return false;

	// OpenXR uses a couple different types of reference frames for positioning content, we need to choose one for
	// displaying our content! STAGE would be relative to the center of your guardian system's bounds, and LOCAL
	// would be relative to your device's starting location. HoloLens doesn't have a STAGE, so we'll use LOCAL.
	XrReferenceSpaceCreateInfo ref_space = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	ref_space.poseInReferenceSpace = OpenXR::GetIdentityPose();
	ref_space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	xrCreateReferenceSpace(session, &ref_space, &applicationSpace);

	// Now we need to find all the viewpoints we need to take care of! For a stereo headset, this should be 2.
	// Similarly, for an AR phone, we'll need 1, and a VR cave could have 6, or even 12!
	uint32_t viewConfigurationCount = 0;
	xrEnumerateViewConfigurationViews(instance, systemID, hmdViewConfiguration, 0, &viewConfigurationCount, nullptr);
	configViews.resize(viewConfigurationCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
	views.resize(viewConfigurationCount, { XR_TYPE_VIEW });
	xrEnumerateViewConfigurationViews(instance, systemID, hmdViewConfiguration, viewConfigurationCount, &viewConfigurationCount, configViews.data());
	for (uint32_t i = 0; i < viewConfigurationCount; i++) 
	{
		// Create a swapchain for this viewpoint! A swapchain is a set of texture buffers used for displaying to screen,
		// typically this is a backbuffer and a front buffer, one for rendering data to, and one for displaying on-screen.
		// A note about swapchain image format here! OpenXR doesn't create a concrete image format for the texture, like 
		// DXGI_FORMAT_R8G8B8A8_UNORM. Instead, it switches to the TYPELESS variant of the provided texture format, like 
		// DXGI_FORMAT_R8G8B8A8_TYPELESS. When creating an ID3D11RenderTargetView for the swapchain texture, we must specify
		// a concrete type like DXGI_FORMAT_R8G8B8A8_UNORM, as attempting to create a TYPELESS view will throw errors, so 
		// we do need to store the format separately and remember it later.
		XrViewConfigurationView& view = configViews[i];
		XrSwapchainCreateInfo    swapchainCreateInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
		XrSwapchain              handle;
		swapchainCreateInfo.arraySize = 1;
		swapchainCreateInfo.mipCount = 1;
		swapchainCreateInfo.faceCount = 1;
		swapchainCreateInfo.format = swapchainFormat;
		swapchainCreateInfo.width = view.recommendedImageRectWidth;
		swapchainCreateInfo.height = view.recommendedImageRectHeight;
		swapchainCreateInfo.sampleCount = view.recommendedSwapchainSampleCount;
		swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		xrCreateSwapchain(session, &swapchainCreateInfo, &handle);

		// Find out how many textures were generated for the swapchain
		uint32_t swapchainImageCount = 0;
		xrEnumerateSwapchainImages(handle, 0, &swapchainImageCount, nullptr);

		// We'll want to track our own information about the swapchain, so we can draw stuff onto it! We'll also create
		// a depth buffer for each generated texture here as well with make_surfacedata.
		Swapchain swapchain = {};
		swapchain.width = swapchainCreateInfo.width;
		swapchain.height = swapchainCreateInfo.height;
		swapchain.handle = handle;
		swapchain.surfaceImages.resize(swapchainImageCount, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
		swapchain.surfaceData.resize(swapchainImageCount);
		xrEnumerateSwapchainImages(swapchain.handle, swapchainImageCount, &swapchainImageCount, (XrSwapchainImageBaseHeader*)swapchain.surfaceImages.data());
		for (uint32_t i = 0; i < swapchainImageCount; i++) 
		{
			swapchain.surfaceData[i] = D3DRenderer::MakeSurfaceData((XrBaseInStructure&)swapchain.surfaceImages[i]);
		}
		swapchains.push_back(swapchain);
	}

	return true;
}

void OpenXR::MakeActions() 
{
	XrActionSetCreateInfo actionsetCreateInfo = { XR_TYPE_ACTION_SET_CREATE_INFO };
	strcpy_s(actionsetCreateInfo.actionSetName, "gameplay");
	strcpy_s(actionsetCreateInfo.localizedActionSetName, "Gameplay");
	xrCreateActionSet(instance, &actionsetCreateInfo, &xrInput.actionSet);
	xrStringToPath(instance, "/user/hand/left", &xrInput.handSubactionPath[0]);
	xrStringToPath(instance, "/user/hand/right", &xrInput.handSubactionPath[1]);

	// Create an action to track the position and orientation of the hands! This is
	// the controller location, or the center of the palms for actual hands.
	XrActionCreateInfo actionCreateInfo = { XR_TYPE_ACTION_CREATE_INFO };
	actionCreateInfo.countSubactionPaths = _countof(xrInput.handSubactionPath);
	actionCreateInfo.subactionPaths = xrInput.handSubactionPath;
	actionCreateInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
	strcpy_s(actionCreateInfo.actionName, "hand_pose");
	strcpy_s(actionCreateInfo.localizedActionName, "Hand Pose");
	xrCreateAction(xrInput.actionSet, &actionCreateInfo, &xrInput.poseAction);

	// Create an action for listening to the select action! This is primary trigger
	// on controllers, and an airtap on HoloLens
	actionCreateInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy_s(actionCreateInfo.actionName, "select");
	strcpy_s(actionCreateInfo.localizedActionName, "Select");
	xrCreateAction(xrInput.actionSet, &actionCreateInfo, &xrInput.selectAction);

	// Bind the actions we just created to specific locations on the Khronos simple_controller
	// definition! These are labeled as 'suggested' because they may be overridden by the runtime
	// preferences. For example, if the runtime allows you to remap buttons, or provides input
	// accessibility settings.
	XrPath profilePath;
	XrPath posePath[2];
	XrPath selectPath[2];
	xrStringToPath(instance, "/user/hand/left/input/grip/pose", &posePath[0]);
	xrStringToPath(instance, "/user/hand/right/input/grip/pose", &posePath[1]);
	xrStringToPath(instance, "/user/hand/left/input/select/click", &selectPath[0]);
	xrStringToPath(instance, "/user/hand/right/input/select/click", &selectPath[1]);
	xrStringToPath(instance, "/interaction_profiles/khr/simple_controller", &profilePath);
	XrActionSuggestedBinding bindings[] = 
	{
		{ xrInput.poseAction,   posePath[0]   },
		{ xrInput.poseAction,   posePath[1]   },
		{ xrInput.selectAction, selectPath[0] },
		{ xrInput.selectAction, selectPath[1] }, };
	XrInteractionProfileSuggestedBinding suggestedBindings = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
	suggestedBindings.interactionProfile = profilePath;
	suggestedBindings.suggestedBindings = &bindings[0];
	suggestedBindings.countSuggestedBindings = _countof(bindings);
	xrSuggestInteractionProfileBindings(instance, &suggestedBindings);

	// Create frames of reference for the pose actions
	for (int32_t i = 0; i < 2; i++) 
	{
		XrActionSpaceCreateInfo actionspaceCreateInfo = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
		actionspaceCreateInfo.action = xrInput.poseAction;
		actionspaceCreateInfo.poseInActionSpace = OpenXR::GetIdentityPose();
		actionspaceCreateInfo.subactionPath = xrInput.handSubactionPath[i];
		xrCreateActionSpace(session, &actionspaceCreateInfo, &xrInput.handSpace[i]);
	}

	// Attach the action set we just made to the session
	XrSessionActionSetsAttachInfo actionsetsAttachInfo = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
	actionsetsAttachInfo.countActionSets = 1;
	actionsetsAttachInfo.actionSets = &xrInput.actionSet;
	xrAttachSessionActionSets(session, &actionsetsAttachInfo);
}

void OpenXR::Shutdown() 
{
	// We used a graphics API to initialize the swapchain data, so we'll
	// give it a chance to release anythig here!
	for (int32_t i = 0; i < swapchains.size(); i++) 
	{
		xrDestroySwapchain(swapchains[i].handle);
		D3DRenderer::SwapchainDestroy(swapchains[i]);
	}
	swapchains.clear();

	// Release all the other OpenXR resources that we've created!
	// What gets allocated, must get deallocated!
	if (xrInput.actionSet != XR_NULL_HANDLE) 
	{
		if (xrInput.handSpace[0] != XR_NULL_HANDLE) xrDestroySpace(xrInput.handSpace[0]);
		if (xrInput.handSpace[1] != XR_NULL_HANDLE) xrDestroySpace(xrInput.handSpace[1]);
		xrDestroyActionSet(xrInput.actionSet);
	}

	if (applicationSpace != XR_NULL_HANDLE) 
		xrDestroySpace(applicationSpace);
	if (session != XR_NULL_HANDLE) 
		xrDestroySession(session);
	if (debugMessenger != XR_NULL_HANDLE) 
		xrDestroyDebugUtilsMessengerEXT(debugMessenger);
	if (instance != XR_NULL_HANDLE) 
		xrDestroyInstance(instance);
}

void OpenXR::PollEvents(bool& exit) 
{
	exit = false;

	XrEventDataBuffer xrEventBuffer = { XR_TYPE_EVENT_DATA_BUFFER };

	while (xrPollEvent(instance, &xrEventBuffer) == XR_SUCCESS) 
	{
		switch (xrEventBuffer.type) 
		{
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: 
			{
				XrEventDataSessionStateChanged* changed = (XrEventDataSessionStateChanged*)&xrEventBuffer;
				sessionState = changed->state;

				// Session state change is where we can begin and end sessions, as well as find quit messages!
				switch (sessionState) 
				{
				case XR_SESSION_STATE_READY: 
				{
					XrSessionBeginInfo sessionBeginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
					sessionBeginInfo.primaryViewConfigurationType = hmdViewConfiguration;
					xrBeginSession(session, &sessionBeginInfo);
					isRunning = true;
				} break;
				case XR_SESSION_STATE_STOPPING: 
				{
					isRunning = false;
					xrEndSession(session);
				} break;
				case XR_SESSION_STATE_EXITING:
				case XR_SESSION_STATE_LOSS_PENDING:
				{
					exit = true;              
					break;
				}
				}
			}
			break;
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
			{
				exit = true; 
				return;
			}
		}
		xrEventBuffer = { XR_TYPE_EVENT_DATA_BUFFER };
	}
}

void OpenXR::PollActions() 
{
	if (sessionState != XR_SESSION_STATE_FOCUSED)
		return;

	// Update our action set with up-to-date input data!
	XrActiveActionSet activeActionSet = { };
	activeActionSet.actionSet = xrInput.actionSet;
	activeActionSet.subactionPath = XR_NULL_PATH;

	XrActionsSyncInfo actionSyncInfo = { XR_TYPE_ACTIONS_SYNC_INFO };
	actionSyncInfo.countActiveActionSets = 1;
	actionSyncInfo.activeActionSets = &activeActionSet;

	xrSyncActions(session, &actionSyncInfo);

	// Now we'll get the current states of our actions, and store them for later use
	for (uint32_t hand = 0; hand < 2; hand++) 
	{
		XrActionStateGetInfo get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
		get_info.subactionPath = xrInput.handSubactionPath[hand];

		XrActionStatePose pose_state = { XR_TYPE_ACTION_STATE_POSE };
		get_info.action = xrInput.poseAction;
		xrGetActionStatePose(session, &get_info, &pose_state);
		xrInput.renderHand[hand] = pose_state.isActive;

		// Events come with a timestamp
		XrActionStateBoolean selectActionState = { XR_TYPE_ACTION_STATE_BOOLEAN };
		get_info.action = xrInput.selectAction;
		xrGetActionStateBoolean(session, &get_info, &selectActionState);
		xrInput.handSelect[hand] = selectActionState.currentState && selectActionState.changedSinceLastSync;

		// If we have a select event, update the hand pose to match the event's timestamp
		if (xrInput.handSelect[hand]) 
		{
			XrSpaceLocation handSpaceLocation = { XR_TYPE_SPACE_LOCATION };
			XrResult        res = xrLocateSpace(xrInput.handSpace[hand], applicationSpace, selectActionState.lastChangeTime, &handSpaceLocation);
			if (XR_UNQUALIFIED_SUCCESS(res) &&
				(handSpaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
				(handSpaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) 
			{
				xrInput.handPose[hand] = handSpaceLocation.pose;
			}
		}
	}
}

void OpenXR::PollPredicted(XrTime predicted_time) 
{
	if (sessionState != XR_SESSION_STATE_FOCUSED)
		return;

	// Update hand position based on the predicted time of when the frame will be rendered! This 
	// should result in a more accurate location, and reduce perceived lag.
	for (size_t index = 0; index < 2; index++) 
	{
		if (!xrInput.renderHand[index])
			continue;
		XrSpaceLocation spaceRelation = { XR_TYPE_SPACE_LOCATION };
		XrResult        res = xrLocateSpace(xrInput.handSpace[index], applicationSpace, predicted_time, &spaceRelation);
		if (XR_UNQUALIFIED_SUCCESS(res) &&
			(spaceRelation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
			(spaceRelation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) 
		{
			xrInput.handPose[index] = spaceRelation.pose;
		}
	}
}

void OpenXR::RenderFrame() 
{
	// Block until the previous frame is finished displaying, and is ready for another one.
	// Also returns a prediction of when the next frame will be displayed, for use with predicting
	// locations of controllers, viewpoints, etc.
	XrFrameState xrCurrentFramState = { XR_TYPE_FRAME_STATE };
	xrWaitFrame(session, nullptr, &xrCurrentFramState);
	// Must be called before any rendering is done! This can return some interesting flags, like 
	// XR_SESSION_VISIBILITY_UNAVAILABLE, which means we could skip rendering this frame and call
	// xrEndFrame right away.
	xrBeginFrame(session, nullptr);

	// Execute any code that's dependent on the predicted time, such as updating the location of
	// controller models.
	PollPredicted(xrCurrentFramState.predictedDisplayTime);
	Application::UpdatePredicted();

	// If the session is active, lets render our layer in the compositor!
	XrCompositionLayerBaseHeader*	layer = nullptr;
	XrCompositionLayerProjection    compositionLayerProjection = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	std::vector<XrCompositionLayerProjectionView> views;
	bool isSessioncurrentlyActive = sessionState == XR_SESSION_STATE_VISIBLE || sessionState == XR_SESSION_STATE_FOCUSED;
	if (isSessioncurrentlyActive && RenderLayer(xrCurrentFramState.predictedDisplayTime, views, compositionLayerProjection)) {
		layer = (XrCompositionLayerBaseHeader*)&compositionLayerProjection;
	}

	// We're finished with rendering our layer, so send it off for display!
	XrFrameEndInfo xrFrameEndInfo{ XR_TYPE_FRAME_END_INFO };
	xrFrameEndInfo.displayTime = xrCurrentFramState.predictedDisplayTime;
	xrFrameEndInfo.environmentBlendMode = blendMode;
	xrFrameEndInfo.layerCount = layer == nullptr ? 0 : 1;
	xrFrameEndInfo.layers = &layer;
	xrEndFrame(session, &xrFrameEndInfo);
}

bool OpenXR::RenderLayer(XrTime predictedTime, std::vector<XrCompositionLayerProjectionView>& layerProjectionViews, XrCompositionLayerProjection& layer) {

	// Find the state and location of each viewpoint at the predicted time
	uint32_t         viewCount = 0;
	XrViewState      viewState = { XR_TYPE_VIEW_STATE };
	XrViewLocateInfo viewLocateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
	viewLocateInfo.viewConfigurationType = hmdViewConfiguration;
	viewLocateInfo.displayTime = predictedTime;
	viewLocateInfo.space = applicationSpace;
	xrLocateViews(session, &viewLocateInfo, &viewState, (uint32_t)views.size(), &viewCount, views.data());
	layerProjectionViews.resize(viewCount);

	// And now we'll iterate through each viewpoint, and render it!
	for (uint32_t i = 0; i < viewCount; i++) {

		// We need to ask which swapchain image to use for rendering! Which one will we get?
		// Who knows! It's up to the runtime to decide.
		uint32_t                    imageID;
		XrSwapchainImageAcquireInfo swapchainImageAcquireInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
		xrAcquireSwapchainImage(swapchains[i].handle, &swapchainImageAcquireInfo, &imageID);

		// Wait until the image is available to render to. The compositor could still be
		// reading from it.
		XrSwapchainImageWaitInfo swapchainImageWaitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
		swapchainImageWaitInfo.timeout = XR_INFINITE_DURATION;
		xrWaitSwapchainImage(swapchains[i].handle, &swapchainImageWaitInfo);

		// Set up our rendering information for the viewpoint we're using right now!
		layerProjectionViews[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
		layerProjectionViews[i].pose = views[i].pose;
		layerProjectionViews[i].fov = views[i].fov;
		layerProjectionViews[i].subImage.swapchain = swapchains[i].handle;
		layerProjectionViews[i].subImage.imageRect.offset = { 0, 0 };
		layerProjectionViews[i].subImage.imageRect.extent = { swapchains[i].width, swapchains[i].height };

		// Call the rendering callback with our view and swapchain info
		D3DRenderer::RenderLayer(layerProjectionViews[i], swapchains[i].surfaceData[imageID]);

		// And tell OpenXR we're done with rendering to this one!
		XrSwapchainImageReleaseInfo swapchainReleaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		xrReleaseSwapchainImage(swapchains[i].handle, &swapchainReleaseInfo);
	}

	layer.space = applicationSpace;
	layer.viewCount = (uint32_t)layerProjectionViews.size();
	layer.views = layerProjectionViews.data();
	return true;
}

XrSessionState OpenXR::GetSessionState()
{
	return sessionState;
}

const InputState& OpenXR::GetInputState()
{
	return xrInput;
}

const XrPosef& OpenXR::GetIdentityPose()
{
	return poseIdentity;
}

bool OpenXR::IsRunning()
{
	return isRunning;
}

bool OpenXR::IsValidSessionState()
{
	return (sessionState == XR_SESSION_STATE_VISIBLE || sessionState == XR_SESSION_STATE_FOCUSED);
}

