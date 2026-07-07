#include "CoreLayer.h"

using namespace Lion;

void CoreLayer::OnAttach()
{
	Window::SetSize(800, 600);
	Window::SetResizable(false);
	Window::SetTitle("Brickout");
	Window::SetBackgroundColor(0.05f, 0.05f, 0.05f);
	Window::SetIcon("Sprite/Brickout/tile-3.png");

	Graphics::SetVerticalSynchronization(false);
}

void CoreLayer::OnRenderUI()
{
	static bool showDemo = false;
	const ImGuiIO& io = ImGui::GetIO();

	ImGui::Begin("Lion - Debug");
	ImGui::Text("Dear ImGui is running.");
	ImGui::Separator();
	ImGui::Text("FPS:   %.1f", io.Framerate);
	ImGui::Text("Frame: %.3f ms", 1000.0f / io.Framerate);
	ImGui::Separator();
	ImGui::Checkbox("Show ImGui demo window", &showDemo);
	ImGui::End();

	// The demo window is a full feature showcase; handy to confirm docking and widgets work.
	if (showDemo)
		ImGui::ShowDemoWindow(&showDemo);
}

void CoreLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Bind<EventWindowClose>(LION_BIND_EVENT(CoreLayer::OnEventWindowClose));
}

bool CoreLayer::OnEventWindowClose(const EventWindowClose& event)
{
    Window::Close();
    return false;
}
