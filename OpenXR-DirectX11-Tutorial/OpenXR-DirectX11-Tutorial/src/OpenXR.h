#pragma once

#include "TutorialStructs.h"

#include <vector>

namespace OpenXR
{
	bool Init(const char* app_name, int64_t swapchain_format);
	void MakeActions();
	void Shutdown();
	
	void PollEvents(bool& exit);
	void PollActions();
	void PollPredicted(XrTime predicted_time);
	
	void RenderFrame();
	bool RenderLayer(XrTime predictedTime, std::vector<XrCompositionLayerProjectionView>& projectionViews, XrCompositionLayerProjection& layer);

	XrSessionState		GetSessionState();
	const InputState&	GetInputState();
	const XrPosef&		GetIdentityPose();

	bool IsRunning();
	bool IsValidSessionState();
}