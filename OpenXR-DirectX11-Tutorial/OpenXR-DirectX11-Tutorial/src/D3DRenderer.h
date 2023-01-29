#pragma once

#include "TutorialStructs.h"

namespace D3DRenderer
{

	bool					d3d_init(LUID& adapter_luid);
	void					d3d_shutdown();
	IDXGIAdapter1*			d3d_get_adapter(LUID& adapter_luid);
	SwapchainSurfacedata	d3d_make_surface_data(XrBaseInStructure& swapchainImage);
	void					d3d_render_layer(XrCompositionLayerProjectionView& layerView, SwapchainSurfacedata& surface);
	void					d3d_swapchain_destroy(Swapchain& swapchain);
	DirectX::XMMATRIX		d3d_xr_projection(XrFovf fov, float clip_near, float clip_far);
	ID3DBlob*				d3d_compile_shader(const char* hlsl, const char* entrypoint, const char* target);

	ID3D11Device*			GetDevice();
	ID3D11DeviceContext*	GetDeviceContext();
	ID3D11VertexShader*		GetVertexShader();
	ID3D11PixelShader*		GetPixelShader();
	ID3D11InputLayout*		GetInputLayout();
 	int64_t					GetSwapchainFormat();
 	ID3D11Buffer*			GetConstantsBuffer();
	ID3D11Buffer*			GetVertexBuffer();
	ID3D11Buffer*			GetIndexBuffer();
	UINT					IndexCount();
	ID3D11InputLayout*		GetShaderLayout();


	void					Setup();

}