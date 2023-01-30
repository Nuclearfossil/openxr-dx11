#pragma comment(lib,"D3D11.lib")
#pragma comment(lib,"D3dcompiler.lib")
#pragma comment(lib,"Dxgi.lib")

#include "D3DRenderer.h"
#include "Application.h"

ID3D11VertexShader*		vertexShader;
ID3D11PixelShader*		pixelShader;
ID3D11InputLayout*		shaderLayout;
ID3D11Buffer*			constantsBuffer;
ID3D11Buffer*			vertexBuffer;
ID3D11Buffer*			indexBuffer;

ID3D11Device*			d3dDevice = nullptr;
ID3D11DeviceContext*	d3dContext = nullptr;
int64_t					d3dSwapchainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

constexpr char xrHLSLShaderCode[] = R"_(
cbuffer TransformBuffer : register(b0) 
{
	float4x4 world;
	float4x4 viewproj;
};
struct vsIn 
{
	float4 pos  : SV_POSITION;
	float3 norm : NORMAL;
};
struct psIn 
{
	float4 pos   : SV_POSITION;
	float3 color : COLOR0;
};

psIn vs(vsIn input) 
{
	psIn output;
	output.pos = mul(float4(input.pos.xyz, 1), world);
	output.pos = mul(output.pos, viewproj);

	float3 normal = normalize(mul(float4(input.norm, 0), world).xyz);

	output.color = saturate(dot(normal, float3(0,1,0))).xxx;
	return output;
}

float4 ps(psIn input) : SV_TARGET 
{
	return float4(input.color, 1);
})_";

// vertices for a 1x1x1 cube
float cubeVertices[] = 
{
	-1,-1,-1, -1,-1,-1, // Bottom vertices
	 1,-1,-1,  1,-1,-1,
	 1, 1,-1,  1, 1,-1,
	-1, 1,-1, -1, 1,-1,
	-1,-1, 1, -1,-1, 1, // Top vertices
	 1,-1, 1,  1,-1, 1,
	 1, 1, 1,  1, 1, 1,
	-1, 1, 1, -1, 1, 1, 
};

uint16_t cuveIndices[] = 
{
	1,2,0, 2,3,0, 4,6,5, 7,6,4,
	6,2,1, 5,6,1, 3,7,4, 0,3,4,
	4,5,1, 0,4,1, 2,7,3, 2,6,7, 
};


bool D3DRenderer::Init(LUID& adapterLUID) 
{
	d3dDevice = nullptr;
	d3dContext = nullptr;

	IDXGIAdapter1* adapter = GetAdapter(adapterLUID);
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	if (adapter == nullptr)
		return false;

	if (FAILED(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, &d3dDevice, nullptr, &d3dContext)))
		return false;

	adapter->Release();
	return true;
}

void D3DRenderer::SetupResources()
{
	// Compile our shader code, and turn it into a shader resource!
	ID3DBlob* vertexShaderBlob = CompileShader(xrHLSLShaderCode, "vs", "vs_5_0");
	ID3DBlob* pixelShaderBlob = CompileShader(xrHLSLShaderCode, "ps", "ps_5_0");

	d3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertexShader);
	d3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &pixelShader);

	// Describe how our mesh is laid out in memory
	D3D11_INPUT_ELEMENT_DESC vertexInputElementDescription[] = {
		{"SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}, };
	d3dDevice->CreateInputLayout(vertexInputElementDescription, (UINT)_countof(vertexInputElementDescription), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &shaderLayout);

	// Create GPU resources for our mesh's vertices and indices! Constant buffers are for passing transform
	// matrices into the shaders, so make a buffer for them too!
	D3D11_SUBRESOURCE_DATA vertexBufferData = { cubeVertices };
	D3D11_SUBRESOURCE_DATA indexBufferData = { cuveIndices };
	CD3D11_BUFFER_DESC     vertexBufferDescription(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
	CD3D11_BUFFER_DESC     indexBufferDescription(sizeof(cuveIndices), D3D11_BIND_INDEX_BUFFER);
	CD3D11_BUFFER_DESC     constantsBufferDescription(sizeof(TransformBuffer), D3D11_BIND_CONSTANT_BUFFER);
	d3dDevice->CreateBuffer(&vertexBufferDescription, &vertexBufferData, &vertexBuffer);
	d3dDevice->CreateBuffer(&indexBufferDescription, &indexBufferData, &indexBuffer);
	d3dDevice->CreateBuffer(&constantsBufferDescription, nullptr, &constantsBuffer);

}

void D3DRenderer::Shutdown() 
{
	if (d3dContext) 
	{ 
		d3dContext->Release(); 
		d3dContext = nullptr; 
	}
	if (d3dDevice) 
	{ 
		d3dDevice->Release();  
		d3dDevice = nullptr; 
	}
}

SwapchainSurfacedata D3DRenderer::MakeSurfaceData(XrBaseInStructure& swapchainImage) 
{
	SwapchainSurfacedata result = {};

	// OpenXR has created a swapchain for us. use that to internally track the swapchain.
	XrSwapchainImageD3D11KHR& d3dSwapchainImage = (XrSwapchainImageD3D11KHR&)swapchainImage;
	D3D11_TEXTURE2D_DESC      colorDescription;
	d3dSwapchainImage.texture->GetDesc(&colorDescription);

	// Create a view resource for the swapchain image target that we can use to set up rendering.
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDescription = {};
	renderTargetViewDescription.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	// NOTE: Why not use color_desc.Format? Check the notes over near the xrCreateSwapchain call!
	// Basically, the color_desc.Format of the OpenXR created swapchain is TYPELESS, but in order to
	// create a View for the texture, we need a concrete variant of the texture format like UNORM.
	renderTargetViewDescription.Format = (DXGI_FORMAT)d3dSwapchainFormat;
	d3dDevice->CreateRenderTargetView(d3dSwapchainImage.texture, &renderTargetViewDescription, &result.targetView);

	// Create a depth buffer that matches 
	ID3D11Texture2D* depthTexture;
	D3D11_TEXTURE2D_DESC depthTextureDescription = {};
	depthTextureDescription.SampleDesc.Count = 1;
	depthTextureDescription.MipLevels = 1;
	depthTextureDescription.Width = colorDescription.Width;
	depthTextureDescription.Height = colorDescription.Height;
	depthTextureDescription.ArraySize = colorDescription.ArraySize;
	depthTextureDescription.Format = DXGI_FORMAT_R32_TYPELESS;
	depthTextureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	d3dDevice->CreateTexture2D(&depthTextureDescription, nullptr, &depthTexture);

	// And create a view resource for the depth buffer, so we can set that up for rendering to as well!
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription = {};
	depthStencilDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilDescription.Format = DXGI_FORMAT_D32_FLOAT;
	d3dDevice->CreateDepthStencilView(depthTexture, &depthStencilDescription, &result.depthView);

	// We don't need direct access to the ID3D11Texture2D object anymore, we only need the view
	depthTexture->Release();

	return result;
}

void D3DRenderer::SwapchainDestroy(Swapchain& swapchain)
{
	for (uint32_t i = 0; i < swapchain.surfaceData.size(); i++)
	{
		swapchain.surfaceData[i].depthView->Release();
		swapchain.surfaceData[i].targetView->Release();
	}
}

ID3DBlob* D3DRenderer::CompileShader(const char* hlslSource, const char* entrypoint, const char* target)
{
	DWORD flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#ifdef _DEBUG
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
	flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	ID3DBlob* compiled, * errors;
	if (FAILED(D3DCompile(hlslSource, strlen(hlslSource), nullptr, nullptr, nullptr, entrypoint, target, flags, 0, &compiled, &errors)))
		printf("Error: D3DCompile failed %s", (char*)errors->GetBufferPointer());

	if (errors) errors->Release();

	return compiled;
}

void D3DRenderer::DrawCubes(XrCompositionLayerProjectionView& view, std::vector<XrPosef>& poses)
{
	// Set up the projection and view matrices for OpenXR
	DirectX::XMMATRIX projectionMatrix = GetXRProjection(view.fov, 0.05f, 100.0f);
	DirectX::XMMATRIX viewMatrix = XMMatrixInverse(nullptr, 
		XMMatrixAffineTransformation(
			DirectX::g_XMOne,
			DirectX::g_XMZero,
			DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)&view.pose.orientation),
			DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&view.pose.position)));

	// For the D3D Context, set up the shader resources that will be used
	d3dContext->VSSetConstantBuffers(0, 1, &constantsBuffer);
	d3dContext->VSSetShader(vertexShader, nullptr, 0);
	d3dContext->PSSetShader(pixelShader, nullptr, 0);

	// Prepare the vertex buffers for rendering
	UINT strides[] = { sizeof(float) * 6 };
	UINT offsets[] = { 0 };

	d3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, strides, offsets);
	d3dContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3dContext->IASetInputLayout(shaderLayout);

	// Create the view x projection matrix and store it into the transform buffer.
	TransformBuffer transformBuffer{};
	XMStoreFloat4x4(&transformBuffer.viewproj, XMMatrixTranspose(viewMatrix * projectionMatrix));

	// And for the cubes
	// - create the model matrix,
	// - update the transform buffer with the model matrix
	for (size_t i = 0; i < poses.size(); i++)
	{
		// Create a translate, rotate, scale matrix for the cube's world location
		DirectX::XMMATRIX modelMatrix = XMMatrixAffineTransformation(
			DirectX::g_XMOne * 0.05f,
			DirectX::g_XMZero,
			DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)&poses[i].orientation),
			DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&poses[i].position));

		// We're push the world transform for the object into the transform buffer.
		// Push that transform buffer to the shader and then draw the primitives.
		XMStoreFloat4x4(&transformBuffer.world, XMMatrixTranspose(modelMatrix));
		d3dContext->UpdateSubresource(constantsBuffer, 0, nullptr, &transformBuffer, 0, 0);
		d3dContext->DrawIndexed(_countof(cuveIndices), 0, 0);
	}
}

void D3DRenderer::RenderLayer(XrCompositionLayerProjectionView& view, SwapchainSurfacedata& surface) 
{
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

IDXGIAdapter1* D3DRenderer::GetAdapter(LUID& adapterLUID) 
{
	// find the appropriate DXGI adapter give a specific adapter LUID
	IDXGIAdapter1* foundAdapter = nullptr;
	IDXGIAdapter1* currentAdapter = nullptr;
	IDXGIFactory1* dxgiFactory;
	DXGI_ADAPTER_DESC1 adapterDescription;

	CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&dxgiFactory));

	int curr = 0;
	while (dxgiFactory->EnumAdapters1(curr++, &currentAdapter) == S_OK) 
	{
		currentAdapter->GetDesc1(&adapterDescription);

		if (memcmp(&adapterDescription.AdapterLuid, &adapterLUID, sizeof(&adapterLUID)) == 0) 
		{
			foundAdapter = currentAdapter;
			break;
		}
		currentAdapter->Release();
		currentAdapter = nullptr;
	}
	dxgiFactory->Release();
	return foundAdapter;
}

ID3D11Device* D3DRenderer::GetDevice()
{
	return d3dDevice;
}

int64_t D3DRenderer::GetSwapchainFormat()
{
	return d3dSwapchainFormat;
}

DirectX::XMMATRIX D3DRenderer::GetXRProjection(XrFovf fov, float clip_near, float clip_far) 
{
	const float left = clip_near * tanf(fov.angleLeft);
	const float right = clip_near * tanf(fov.angleRight);
	const float down = clip_near * tanf(fov.angleDown);
	const float up = clip_near * tanf(fov.angleUp);

	return DirectX:: XMMatrixPerspectiveOffCenterRH(left, right, down, up, clip_near, clip_far);
}
