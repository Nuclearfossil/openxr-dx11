#include "D3DRenderer.h"
#include "Application.h"

ID3D11VertexShader* vertexShader;
ID3D11PixelShader* pixelShader;
ID3D11InputLayout* shaderLayout;
ID3D11Buffer* constantBuffer;
ID3D11Buffer* vertexBuffer;
ID3D11Buffer* indexBuffer;

ID3D11Device*			d3dDevice = nullptr;
ID3D11DeviceContext*	d3dContext = nullptr;
int64_t					d3dSwapchainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

constexpr char app_shader_code[] = R"_(
cbuffer TransformBuffer : register(b0) {
	float4x4 world;
	float4x4 viewproj;
};
struct vsIn {
	float4 pos  : SV_POSITION;
	float3 norm : NORMAL;
};
struct psIn {
	float4 pos   : SV_POSITION;
	float3 color : COLOR0;
};

psIn vs(vsIn input) {
	psIn output;
	output.pos = mul(float4(input.pos.xyz, 1), world);
	output.pos = mul(output.pos, viewproj);

	float3 normal = normalize(mul(float4(input.norm, 0), world).xyz);

	output.color = saturate(dot(normal, float3(0,1,0))).xxx;
	return output;
}
float4 ps(psIn input) : SV_TARGET {
	return float4(input.color, 1);
})_";

float app_verts[] = {
	-1,-1,-1, -1,-1,-1, // Bottom verts
	 1,-1,-1,  1,-1,-1,
	 1, 1,-1,  1, 1,-1,
	-1, 1,-1, -1, 1,-1,
	-1,-1, 1, -1,-1, 1, // Top verts
	 1,-1, 1,  1,-1, 1,
	 1, 1, 1,  1, 1, 1,
	-1, 1, 1, -1, 1, 1, };

uint16_t app_inds[] = {
	1,2,0, 2,3,0, 4,6,5, 7,6,4,
	6,2,1, 5,6,1, 3,7,4, 0,3,4,
	4,5,1, 0,4,1, 2,7,3, 2,6,7, };


UINT D3DRenderer::IndexCount()
{
	return (UINT)_countof(app_inds);
}

ID3D11Device* D3DRenderer::GetDevice()
{
	return d3dDevice;
}

ID3D11DeviceContext* D3DRenderer::GetDeviceContext()
{
	return d3dContext;
}

ID3D11VertexShader* D3DRenderer::GetVertexShader()
{
	return vertexShader;
}

ID3D11PixelShader* D3DRenderer::GetPixelShader()
{
	return pixelShader;
}

ID3D11InputLayout* D3DRenderer::GetInputLayout()
{
	return shaderLayout;
}

ID3D11Buffer* D3DRenderer::GetConstantsBuffer()
{
	return constantBuffer;
}

ID3D11Buffer* D3DRenderer::GetVertexBuffer()
{
	return vertexBuffer;
}

ID3D11Buffer* D3DRenderer::GetIndexBuffer()
{
	return indexBuffer;
}

int64_t D3DRenderer::GetSwapchainFormat()
{
	return d3dSwapchainFormat;
}

ID3D11InputLayout* D3DRenderer::GetShaderLayout()
{
	return shaderLayout;
}

void D3DRenderer::Setup()
{
	// Compile our shader code, and turn it into a shader resource!
	ID3DBlob* vert_shader_blob = d3d_compile_shader(app_shader_code, "vs", "vs_5_0");
	ID3DBlob* pixel_shader_blob = d3d_compile_shader(app_shader_code, "ps", "ps_5_0");

	d3dDevice->CreateVertexShader(vert_shader_blob->GetBufferPointer(), vert_shader_blob->GetBufferSize(), nullptr, &vertexShader);
	d3dDevice->CreatePixelShader(pixel_shader_blob->GetBufferPointer(), pixel_shader_blob->GetBufferSize(), nullptr, &pixelShader);

	// Describe how our mesh is laid out in memory
	D3D11_INPUT_ELEMENT_DESC vert_desc[] = {
		{"SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}, };
	d3dDevice->CreateInputLayout(vert_desc, (UINT)_countof(vert_desc), vert_shader_blob->GetBufferPointer(), vert_shader_blob->GetBufferSize(), &shaderLayout);

	// Create GPU resources for our mesh's vertices and indices! Constant buffers are for passing transform
	// matrices into the shaders, so make a buffer for them too!
	D3D11_SUBRESOURCE_DATA vert_buff_data = { app_verts };
	D3D11_SUBRESOURCE_DATA ind_buff_data = { app_inds };
	CD3D11_BUFFER_DESC     vert_buff_desc(sizeof(app_verts), D3D11_BIND_VERTEX_BUFFER);
	CD3D11_BUFFER_DESC     ind_buff_desc(sizeof(app_inds), D3D11_BIND_INDEX_BUFFER);
	CD3D11_BUFFER_DESC     const_buff_desc(sizeof(TransformBuffer), D3D11_BIND_CONSTANT_BUFFER);
	d3dDevice->CreateBuffer(&vert_buff_desc, &vert_buff_data, &vertexBuffer);
	d3dDevice->CreateBuffer(&ind_buff_desc, &ind_buff_data, &indexBuffer);
	d3dDevice->CreateBuffer(&const_buff_desc, nullptr, &constantBuffer);

}
bool D3DRenderer::d3d_init(LUID& adapter_luid) 
{
	d3dDevice = nullptr;
	d3dContext = nullptr;

	IDXGIAdapter1* adapter = d3d_get_adapter(adapter_luid);
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	if (adapter == nullptr)
		return false;
	if (FAILED(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, &d3dDevice, nullptr, &d3dContext)))
		return false;

	adapter->Release();
	return true;
}

///////////////////////////////////////////

IDXGIAdapter1* D3DRenderer::d3d_get_adapter(LUID& adapter_luid) {
	// Turn the LUID into a specific graphics device adapter
	IDXGIAdapter1* final_adapter = nullptr;
	IDXGIAdapter1* curr_adapter = nullptr;
	IDXGIFactory1* dxgi_factory;
	DXGI_ADAPTER_DESC1 adapter_desc;

	CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&dxgi_factory));

	int curr = 0;
	while (dxgi_factory->EnumAdapters1(curr++, &curr_adapter) == S_OK) {
		curr_adapter->GetDesc1(&adapter_desc);

		if (memcmp(&adapter_desc.AdapterLuid, &adapter_luid, sizeof(&adapter_luid)) == 0) {
			final_adapter = curr_adapter;
			break;
		}
		curr_adapter->Release();
		curr_adapter = nullptr;
	}
	dxgi_factory->Release();
	return final_adapter;
}

///////////////////////////////////////////

void D3DRenderer::d3d_shutdown() {
	if (d3dContext) { d3dContext->Release(); d3dContext = nullptr; }
	if (d3dDevice) { d3dDevice->Release();  d3dDevice = nullptr; }
}

///////////////////////////////////////////

SwapchainSurfacedata D3DRenderer::d3d_make_surface_data(XrBaseInStructure& swapchain_img) {
	SwapchainSurfacedata result = {};

	// Get information about the swapchain image that OpenXR made for us!
	XrSwapchainImageD3D11KHR& d3d_swapchain_img = (XrSwapchainImageD3D11KHR&)swapchain_img;
	D3D11_TEXTURE2D_DESC      color_desc;
	d3d_swapchain_img.texture->GetDesc(&color_desc);

	// Create a view resource for the swapchain image target that we can use to set up rendering.
	D3D11_RENDER_TARGET_VIEW_DESC target_desc = {};
	target_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	// NOTE: Why not use color_desc.Format? Check the notes over near the xrCreateSwapchain call!
	// Basically, the color_desc.Format of the OpenXR created swapchain is TYPELESS, but in order to
	// create a View for the texture, we need a concrete variant of the texture format like UNORM.
	target_desc.Format = (DXGI_FORMAT)d3dSwapchainFormat;
	d3dDevice->CreateRenderTargetView(d3d_swapchain_img.texture, &target_desc, &result.targetView);

	// Create a depth buffer that matches 
	ID3D11Texture2D* depth_texture;
	D3D11_TEXTURE2D_DESC depth_desc = {};
	depth_desc.SampleDesc.Count = 1;
	depth_desc.MipLevels = 1;
	depth_desc.Width = color_desc.Width;
	depth_desc.Height = color_desc.Height;
	depth_desc.ArraySize = color_desc.ArraySize;
	depth_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	depth_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	d3dDevice->CreateTexture2D(&depth_desc, nullptr, &depth_texture);

	// And create a view resource for the depth buffer, so we can set that up for rendering to as well!
	D3D11_DEPTH_STENCIL_VIEW_DESC stencil_desc = {};
	stencil_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	stencil_desc.Format = DXGI_FORMAT_D32_FLOAT;
	d3dDevice->CreateDepthStencilView(depth_texture, &stencil_desc, &result.depthView);

	// We don't need direct access to the ID3D11Texture2D object anymore, we only need the view
	depth_texture->Release();

	return result;
}

///////////////////////////////////////////

void D3DRenderer::d3d_render_layer(XrCompositionLayerProjectionView& view, SwapchainSurfacedata& surface) {
	// Set up where on the render target we want to draw, the view has a 
	XrRect2Di& rect = view.subImage.imageRect;
	D3D11_VIEWPORT viewport = CD3D11_VIEWPORT((float)rect.offset.x, (float)rect.offset.y, (float)rect.extent.width, (float)rect.extent.height);
	d3dContext->RSSetViewports(1, &viewport);

	// Wipe our swapchain color and depth target clean, and then set them up for rendering!
	float clear[] = { 0, 0, 0, 1 };
	d3dContext->ClearRenderTargetView(surface.targetView, clear);
	d3dContext->ClearDepthStencilView(surface.depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	d3dContext->OMSetRenderTargets(1, &surface.targetView, surface.depthView);

	// And now that we're set up, pass on the rest of our rendering to the application
	Application::Draw(view);
}

///////////////////////////////////////////

void D3DRenderer::d3d_swapchain_destroy(Swapchain& swapchain) {
	for (uint32_t i = 0; i < swapchain.surfaceData.size(); i++) {
		swapchain.surfaceData[i].depthView->Release();
		swapchain.surfaceData[i].targetView->Release();
	}
}

///////////////////////////////////////////

DirectX::XMMATRIX D3DRenderer::d3d_xr_projection(XrFovf fov, float clip_near, float clip_far) {
	const float left = clip_near * tanf(fov.angleLeft);
	const float right = clip_near * tanf(fov.angleRight);
	const float down = clip_near * tanf(fov.angleDown);
	const float up = clip_near * tanf(fov.angleUp);

	return DirectX:: XMMatrixPerspectiveOffCenterRH(left, right, down, up, clip_near, clip_far);
}

///////////////////////////////////////////

ID3DBlob* D3DRenderer::d3d_compile_shader(const char* hlsl, const char* entrypoint, const char* target) {
	DWORD flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#ifdef _DEBUG
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
	flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	ID3DBlob* compiled, * errors;
	if (FAILED(D3DCompile(hlsl, strlen(hlsl), nullptr, nullptr, nullptr, entrypoint, target, flags, 0, &compiled, &errors)))
		printf("Error: D3DCompile failed %s", (char*)errors->GetBufferPointer());
	if (errors) errors->Release();

	return compiled;
}
