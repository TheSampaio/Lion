#include "Engine.h"
#include "Application.h"

#include <Lion/Audio/Audio.h>
#include <Lion/Core/Asset.h>
#include <Lion/Core/Clock.h>
#include <Lion/Core/Input.h>
#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Layer.h>
#include <Lion/Core/Log.h>
#include <Lion/Core/Stack.h>
#include <Lion/Core/Window.h>

#include <Lion/Signal/Event.h>
#include <Lion/Signal/EventDispatcher.h>
#include <Lion/Signal/EventWindow.h>

#include <Lion/Render/Graphics.h>
#include <Lion/Render/CameraOrthographic.h>
#include <Lion/Render/Renderer.h>
#include <Lion/Render/Sprite.h>
#include <Lion/Render/Texture.h>

namespace Lion
{
	ApplicationKind Application::sKind = ApplicationKind::Game;
	bool Application::sQuitRequested = false;

	template<typename T>
	static T TryInitialize(T result, const char8* name)
	{
		if (!result)
		{
			Lion::Log::Console(Lion::LogLevel::Fatal, LION_FORMAT_TEXT("[Application] {} initialization failed.", name));
			return result;
		}

		Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[Application] {} initialized successfully.", name));
		return result;
	}

	void Application::PushLayer(Layer* layer)
	{
		mStack->PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay)
	{
		mStack->PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		// Event window resize
		dispatcher.Bind<EventWindowResize>([this](const EventWindowResize& e)
			{
				if (e.GetWidth() == 0 || e.GetHeight() == 0)
				{
					mMinimized = true;
					return false;
				}

				mMinimized = false;
				Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
				return false;
			});

		for (auto it = mStack->rbegin(); it != mStack->rend(); ++it)
		{
			if (event.Handled)
				break;

			(*it)->OnEvent(event);
		}
	}

	Application::Application(ApplicationKind kind)
		: mStack(nullptr), mMinimized(false)
	{
		sKind = kind;
		// Created in every configuration: what the log actually emits is decided at runtime, by the
		// verbosity, since the editor needs it whatever build it was compiled in.
		Log::New();

		mAsset = MakeScope<Asset>();
		mStack = MakeScope<Stack>();

		Window::New();
		Input::New();
		Audio::New();
		Graphics::New();
		Renderer::New();
		Clock::New();
	}

	bool Application::IsEditor()
	{
		return sKind == ApplicationKind::Editor;
	}

	void Application::RequestQuit()
	{
		if (IsEditor())
			sQuitRequested = true;
		else
			Window::RequestClose();
	}

	bool Application::ConsumeQuitRequest()
	{
		const bool requested = sQuitRequested;
		sQuitRequested = false;
		return requested;
	}

	Application::~Application()
	{
		Clock::Delete();

		// Layers own scenes and sprites, while the asset cache keeps their shared textures alive. Both must
		// release those GPU resources before the renderer, graphics context and window disappear.
		mStack.reset();
		mAsset.reset();

		Audio::Delete();
		Renderer::Delete();
		Graphics::Delete();
		Input::Delete();
		Window::Delete();

		Log::Delete();
	}

	void Application::Run()
	{
		// Initializes Window, Graphics and Renderer
		Initialize();

#ifdef LN_DEBUG
		// Show graphics information
		Graphics::ShowSpecification();

#endif // LN_DEBUG

		// Window's events sign up
		Window::SetEventCallback(LION_BIND_EVENT(Application::OnEvent));

		// Starts engine's clock
		Clock::GetTimer().Start();

		// Load resources
		Input::LoadActionMap(ResolveResourcePath(Input::kDefaultActionMapFile));

		for (Layer* layer : *mStack)
			layer->OnCreate();

		// The window draws itself while it is being dragged by an edge, because Windows keeps the thread
		// for the whole of that drag and hands it back only through this.
		Window::SetRefreshCallback([this] { Frame(); });

		// Show window
		Window::Show();

		if (!IsEditor())
			ShowStartupSplash();

		do
		{
			Window::PollEvents();
			Input::Update();
			Audio::Update();
			Frame();

		} while (!Window::Close());

		// Detach layers while the window and graphics context are still alive, so they can
		// release GPU/UI resources safely (the stack destructor only deletes them afterwards).
		for (Layer* layer : *mStack)
			layer->OnDetach();
	}

	void Application::ShowStartupSplash()
	{
		const Reference<Texture> texture = Asset::LoadTexture(
			"Lion Engine Startup Splash", "Images/lion-engine-banner.png");

		if (!texture)
			return;

		const Size windowSize = Window::GetSize();
		Reference<CameraOrthographic> camera = MakeReference<CameraOrthographic>();
		camera->OnResize(windowSize.width, windowSize.height);
		Sprite logo(texture);
		const float32 targetWidth = std::min(windowSize.width * 0.48f, 620.0f);
		const float32 scale = targetWidth / std::max(logo.GetSize().width, 1.0f);
		const auto started = std::chrono::steady_clock::now();
		constexpr auto duration = std::chrono::milliseconds(900);

		while (!Window::Close() && std::chrono::steady_clock::now() - started < duration)
		{
			Window::PollEvents();
			Input::Update();
			Clock::UpdateFrameTime();
			Renderer::Clear(0.012f, 0.012f, 0.014f, 1.0f);
			Renderer::RenderBegin(camera);
			logo.Draw(Vector2(0.0f, 0.0f), Vector(), Vector(scale, scale, 1.0f));
			Renderer::RenderEnd();
			Graphics::SwapBuffers();
		}
	}

	void Application::Frame()
	{
		// PollEvents can call back into here, and does while the window is being resized. One frame at a
		// time: the second would begin a UI frame the first has not ended.
		if (mInFrame || mMinimized)
			return;

		mInFrame = true;

		Clock::UpdateFrameTime();

		for (Layer* layer : *mStack)
			layer->OnUpdateBegin();

		for (Layer* layer : *mStack)
			layer->OnUpdate();

		for (Layer* layer : *mStack)
			layer->OnUpdateEnd();

		Graphics::ClearBuffers();

		for (Layer* layer : *mStack)
			layer->OnRender();

		Graphics::SwapBuffers();

		mInFrame = false;
	}

	void Application::Initialize()
	{
		if (!TryInitialize(Window::Initialize(), "Window"))
			return;

		if(!TryInitialize(Graphics::Initialize(), "Graphics"))
			return;

		if (!TryInitialize(Renderer::Initialize(), "Renderer"))
			return;
	}
}
