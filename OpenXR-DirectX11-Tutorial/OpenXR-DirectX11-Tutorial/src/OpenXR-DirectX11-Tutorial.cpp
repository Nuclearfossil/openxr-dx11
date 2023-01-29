#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP\

// Tell OpenXR what platform code we'll be using
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11

#include <d3d11.h>
#include <directxmath.h> // Matrix math functions and objects
#include <d3dcompiler.h> // For compiling shaders! D3DCompile
#include "TutorialStructs.h"

#include "Application.h"
#include "D3DRenderer.h"
#include "OpenXR.h"

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <thread> // sleep_for
#include <vector>
#include <algorithm> // any_of


using namespace std;
using namespace DirectX; // Matrix math

///////////////////////////////////////////

///////////////////////////////////////////

///////////////////////////////////////////




///////////////////////////////////////////
// Main                                  //
///////////////////////////////////////////

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
	if (!OpenXR::Init("Single file OpenXR", D3DRenderer::GetSwapchainFormat())) {
		D3DRenderer::d3d_shutdown();
		LOG(ERROR) << "OpenXR initialization failed";
		//MessageBox(nullptr, "OpenXR initialization failed\n", "Error", 1);
		return 1;
	}
	OpenXR::MakeActions();
	Application::Init();

	bool quit = false;
	while (!quit) 
	{
		OpenXR::PollEvents(quit);

		if (OpenXR::IsRunning) 
		{
			OpenXR::PollActions();
			Application::Update();
			OpenXR::RenderFrame();

			auto sessionState = OpenXR::GetSessionState();
			if (sessionState != XR_SESSION_STATE_VISIBLE &&
				sessionState != XR_SESSION_STATE_FOCUSED) {
				this_thread::sleep_for(chrono::milliseconds(250));
			}
		}
	}

	OpenXR::Shutdown();
	D3DRenderer::d3d_shutdown();
	return 0;
}
