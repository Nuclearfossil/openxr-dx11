#pragma once

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