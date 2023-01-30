#pragma once

#include "TutorialStructs.h"
#include <vector>

namespace D3DRenderer
{

	bool					Init(LUID& adapter_luid);
	void					SetupResources();
	void					Shutdown();

	SwapchainSurfacedata	MakeSurfaceData(XrBaseInStructure& swapchainImage);
	void					SwapchainDestroy(Swapchain& swapchain);
	ID3DBlob*				CompileShader(const char* hlsl, const char* entrypoint, const char* target);

	void					DrawCubes(XrCompositionLayerProjectionView& view, std::vector<XrPosef>& poses);
	void					RenderLayer(XrCompositionLayerProjectionView& layerView, SwapchainSurfacedata& surface);

	IDXGIAdapter1*			GetAdapter(LUID& adapter_luid);
	ID3D11Device*			GetDevice();
 	int64_t					GetSwapchainFormat();
	DirectX::XMMATRIX		GetXRProjection(XrFovf fov, float clip_near, float clip_far);
}