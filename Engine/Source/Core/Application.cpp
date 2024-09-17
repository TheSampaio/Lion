#include "Engine.h"
#include "Application.h"

#include "Debug.h"
#include "Layer.h"

namespace Lion
{
	void Application::Push(Layer* layer)
	{
		m_Layer = layer;
		layer->OnAttach();
	}

	Application::Application()
		: m_Layer(nullptr)
	{
		Debug::New();
	}

	Application::~Application()
	{
		Debug::Delete();

		// Deletes the pushed layer (Temporary)
		m_Layer->OnDetach();
		delete m_Layer;
	}

	void Application::Run()
	{
		do
		{
			// Call layer events (Temporary)
			m_Layer->OnUpdate();
			m_Layer->OnRender();

		} while (true);
	}
}
