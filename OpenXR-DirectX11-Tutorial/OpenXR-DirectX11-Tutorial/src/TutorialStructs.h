#pragma once

// using pragmas' to define the libs used.
#pragma comment(lib,"D3D11.lib")
#pragma comment(lib, "D3dcompiler.lib")
#pragma comment(lib, "Dxgi.lib")

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

// Defines necessary for the underlying Graphics API
// In this case, DX11.
// The full list is in openxr_platform.h, but currently they are:
// - XR_USE_GRAPHICS_API_VULKAN
// - XR_USE_GRAPHICS_API_D3D11
// - XR_USE_GRAPHICS_API_D3D12
// - XR_USE_GRAPHICS_API_OPENGL_ES
// - XR_USE_GRAPHICS_API_OPENGL
// -
#define XR_USE_GRAPHICS_API_D3D11

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