#pragma once

#include "OpenXR_setup.h"

#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>
#include <vector>

#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"

struct SwapchainSurfacedata 
{
	ID3D11DepthStencilView* depthView;
	ID3D11RenderTargetView* targetView;
};

struct Swapchain 
{
	XrSwapchain handle;
	int32_t     width;
	int32_t     height;
	std::vector<XrSwapchainImageD3D11KHR> surfaceImages;
	std::vector<SwapchainSurfacedata>     surfaceData;
};

struct InputState 
{
	XrActionSet actionSet;
	XrAction    poseAction;
	XrAction    selectAction;
	XrPath   handSubactionPath[2];
	XrSpace  handSpace[2];
	XrPosef  handPose[2];
	XrBool32 renderHand[2];
	XrBool32 handSelect[2];
};

struct TransformBuffer 
{
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 viewproj;
};