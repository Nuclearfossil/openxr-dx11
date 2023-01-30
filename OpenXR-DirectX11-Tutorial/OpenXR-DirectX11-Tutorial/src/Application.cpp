#include "TutorialStructs.h"
#include "OpenXR.h"
#include "D3DRenderer.h"

#include "Application.h"

std::vector<XrPosef> cubePoses;

void Application::Draw(XrCompositionLayerProjectionView& view)
{
	D3DRenderer::DrawCubes(view, cubePoses);
}

void Application::Update()
{
	// If the user presses the select action, lets add a cube at that location!
	auto inputState = OpenXR::GetInputState();
	for (uint32_t i = 0; i < 2; i++) 
	{
		if (inputState.handSelect[i])
			cubePoses.push_back(inputState.handPose[i]);
	}
}

void Application::UpdatePredicted()
{
	// Update the location of the hand cubes. This is done after the inputs have been updated to 
	// use the predicted location, but during the render code, so we have the most up-to-date location.
	if (cubePoses.size() < 2)
		cubePoses.resize(2, OpenXR::GetIdentityPose());

	auto inputState = OpenXR::GetInputState();
	for (uint32_t i = 0; i < 2; i++) 
	{
		cubePoses[i] = inputState.renderHand[i] ? inputState.handPose[i] : OpenXR::GetIdentityPose();
	}
}
