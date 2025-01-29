#include "Engine.h"
#include "Application.h"

#include "Layer.h"
#include "Log.h"
#include "Stack.h"

namespace Lion
{
	void Application::PushLayer(Layer* layer)
	{
		m_Stack->PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay)
	{
		m_Stack->PushOverlay(overlay);
		overlay->OnAttach();
	}

	Application::Application()
		: m_Stack(nullptr)
	{
#ifndef LN_SHIPPING
		Log::New();

#endif // !LN_SHIPPING

		m_Stack = new Stack();
	}

	Application::~Application()
	{
		delete m_Stack;

		Log::Delete();
	}

	void Application::Run()
	{
		do
		{
			// Call layer events
			for (Layer* layer : *m_Stack)
				layer->OnUpdate();

			for (Layer* layer : *m_Stack)
				layer->OnRender();

		} while (true);
	}
}
