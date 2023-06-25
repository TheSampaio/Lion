#pragma once

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

		static void SetVerticalSynchronization(ESynchronizationMode Mode) { GetInstance().m_VSyncMode = Mode; }

		// === Friends ======

		friend class Application;

	protected:
		Graphics();
		~Graphics();

		// Deletes copy constructor and assigment operator
		Graphics(const Graphics&) = delete;
		Graphics operator=(const Graphics&) = delete;

		// Gets the class's static reference
		static Graphics& GetInstance() { static Graphics s_Instance; return s_Instance; }

	private:
		// Attributes
		ID3D11Device* m_Device;
		ID3D11DeviceContext* m_Context;
		D3D11_VIEWPORT m_Viewport;

		IDXGISwapChain* m_SwapChain;
		ID3D11RenderTargetView* m_RenderTargetView;
		ID3D11BlendState* m_BlendState;
		D3D_FEATURE_LEVEL m_FeatureLevel;
		ESynchronizationMode m_VSyncMode;
		float m_BackgroundColour[4];

		// MAIN methods
		bool Initialize();

		void ClearBuffers() { m_Context->ClearRenderTargetView(m_RenderTargetView, m_BackgroundColour); }
		void SwapBuffers() { m_SwapChain->Present(m_VSyncMode, 0); m_Context->OMSetRenderTargets(1, &m_RenderTargetView, nullptr); }
	};
}
