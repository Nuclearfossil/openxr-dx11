

#include "D3DRenderer.h"
#include "OpenXR.h"
#include "Application.h"

#include <thread>

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP\

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) 
{
	if (!OpenXR::Init("OpenXR with DirectX 11", D3DRenderer::GetSwapchainFormat())) 
	{
		D3DRenderer::Shutdown();
		LOG(ERROR) << "OpenXR initialization failed";
		return -11;
	}

	OpenXR::MakeActions();
	D3DRenderer::SetupResources();

	bool quit = false;
	while (!quit) 
	{
		OpenXR::PollEvents(quit);

		if (OpenXR::IsRunning()) 
		{
			OpenXR::PollActions();
			Application::Update();
			OpenXR::RenderFrame();

			auto sessionState = OpenXR::GetSessionState();
			if (!OpenXR::IsValidSessionState())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
			}
		}
	}

	OpenXR::Shutdown();
	D3DRenderer::Shutdown();
	return 0;
}
