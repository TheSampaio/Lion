#include "Engine.h"
#include "Application.h"

#include "Debug.h"
#include "Layer.h"
#include "Stack.h"

namespace Lion
{
	void Application::PushLayer(Layer* layer)
	{
		m_Stack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay)
	{
		m_Stack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	Application::Application()
	{
		Debug::New();
	}

	Application::~Application()
	{
		Debug::Delete();
	}

	void Application::Run()
	{
		do
		{
			// Call layer events
			for (Layer* layer : m_Stack)
				layer->OnUpdate();

			for (Layer* layer : m_Stack)
				layer->OnRender();

		} while (true);
	}
}
