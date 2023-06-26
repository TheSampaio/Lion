#pragma once

// Enumerates the v-sync mode
enum ESynchronizationMode
{
	Disabled = 0,
	Full,
	Half
};

namespace owl
{
	class OWL_API Graphics
	{
	public:
		// === SET methods ======

		// Sets the v-sync mode
		static void SetVerticalSynchronization(ESynchronizationMode Mode) { GetInstance().m_VSyncMode = Mode; }

		// === Friends ======

		friend class Application;

	protected:
		Graphics();

		// Deletes copy constructor and assigment operator
		Graphics(const Graphics&) = delete;
		Graphics operator=(const Graphics&) = delete;

		// Gets the class's static reference
		static Graphics& GetInstance() { static Graphics s_Instance; return s_Instance; }

	private:
		// Attributes
		ID3D11Device* m_D3D11Device;
		ID3D11DeviceContext* m_D3D11Context;
		D3D11_VIEWPORT m_D3D11Viewport;
		IDXGISwapChain* m_DXGISwapChain;
		ID3D11RenderTargetView* m_D3D11RenderTargetView;
		ID3D11BlendState* m_D3D11BlendState;
		D3D_FEATURE_LEVEL m_D3DFeatureLevel;
		ESynchronizationMode m_VSyncMode;
		float m_BackgroundColour[4];

		// MAIN methods
		bool Initialize();
		void Finalize();
		void ClearBuffers() { m_D3D11Context->ClearRenderTargetView(m_D3D11RenderTargetView, m_BackgroundColour); }
		void SwapBuffers() { m_DXGISwapChain->Present(m_VSyncMode, 0); m_D3D11Context->OMSetRenderTargets(1, &m_D3D11RenderTargetView, nullptr); }
	};
}
