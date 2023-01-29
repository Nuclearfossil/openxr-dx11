#include "TutorialStructs.h"
#include "OpenXR.h"
#include "D3DRenderer.h"

#include "Application.h"

std::vector<XrPosef> cubePoses;

void Application::Init()
{
	D3DRenderer::Setup();
}

///////////////////////////////////////////

void Application::Draw(XrCompositionLayerProjectionView& view)
{
	// Set up camera matrices based on OpenXR's predicted viewpoint information
	DirectX::XMMATRIX mat_projection = D3DRenderer::d3d_xr_projection(view.fov, 0.05f, 100.0f);
	DirectX::XMMATRIX mat_view = XMMatrixInverse(nullptr, XMMatrixAffineTransformation(
		DirectX::g_XMOne, DirectX::g_XMZero,
		DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)&view.pose.orientation),
		DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&view.pose.position)));

	// Set the active shaders and constant buffers.
	auto d3dContext = D3DRenderer::GetDeviceContext();
	auto constantsBuffer = D3DRenderer::GetConstantsBuffer();
	d3dContext->VSSetConstantBuffers(0, 1, &constantsBuffer);
	d3dContext->VSSetShader(D3DRenderer::GetVertexShader(), nullptr, 0);
	d3dContext->PSSetShader(D3DRenderer::GetPixelShader(), nullptr, 0);

	// Set up the cube mesh's information
	UINT strides[] = { sizeof(float) * 6 };
	UINT offsets[] = { 0 };
	auto vertexBuffer = D3DRenderer::GetVertexBuffer();
	d3dContext->IASetVertexBuffers(0, 1, &vertexBuffer, strides, offsets);
	d3dContext->IASetIndexBuffer(D3DRenderer::GetIndexBuffer(), DXGI_FORMAT_R16_UINT, 0);
	d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3dContext->IASetInputLayout(D3DRenderer::GetInputLayout());

	// Put camera matrices into the shader's constant buffer
	TransformBuffer transform_buffer;
	XMStoreFloat4x4(&transform_buffer.viewproj, XMMatrixTranspose(mat_view * mat_projection));

	// Draw all the cubes we have in our list!
	for (size_t i = 0; i < cubePoses.size(); i++) {
		// Create a translate, rotate, scale matrix for the cube's world location
		DirectX::XMMATRIX mat_model = XMMatrixAffineTransformation(
			DirectX::g_XMOne * 0.05f, DirectX::g_XMZero,
			DirectX::XMLoadFloat4((DirectX::XMFLOAT4*)&cubePoses[i].orientation),
			DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&cubePoses[i].position));

		// Update the shader's constant buffer with the transform matrix info, and then draw the mesh!
		XMStoreFloat4x4(&transform_buffer.world, XMMatrixTranspose(mat_model));
		d3dContext->UpdateSubresource(constantsBuffer, 0, nullptr, &transform_buffer, 0, 0);
		d3dContext->DrawIndexed(D3DRenderer::IndexCount(), 0, 0);
	}
}

///////////////////////////////////////////

void Application::Update()
{
	// If the user presses the select action, lets add a cube at that location!
	auto inputState = OpenXR::GetInputState();
	for (uint32_t i = 0; i < 2; i++) {
		if (inputState.handSelect[i])
			cubePoses.push_back(inputState.handPose[i]);
	}
}

///////////////////////////////////////////

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
