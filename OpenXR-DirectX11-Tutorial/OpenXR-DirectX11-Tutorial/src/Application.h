#pragma once

#include "OpenXR_setup.h"

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

namespace Application
{
	void Draw(XrCompositionLayerProjectionView& layerView);
	void Update();
	void UpdatePredicted();
}