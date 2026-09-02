#include "GameLayer.h"

#include <Lion/Core/Filesystem.h>

using namespace Lion;

void GameLayer::OnCreate()
{
	mCamera = MakeReference<CameraOrthographic>();

	if (!SceneManager::LoadScene("Scenes/Level01.lnscene"))
		Log::Console(LogLevel::Fatal, "[Game] Could not load the first level.");
}

void GameLayer::OnUpdate()
{
	SceneManager::Update();
}

void GameLayer::OnRender()
{
	const Reference<Scene> scene = SceneManager::GetActiveScene();

	if (!scene)
		return;

	Camera2D* sceneCamera = scene->FindComponent<Camera2D>();

	if (sceneCamera && sceneCamera->IsEnabled())
	{
		const glm::vec2 position = sceneCamera->GetViewPosition();
		mCamera->SetPosition(glm::vec3(position.x, position.y, 0.0f));

		mCamera->SetZoomLevel(sceneCamera->GetZoomForViewportHeight(mCamera->GetViewportHeight()));
	}

	Renderer::RenderBegin(mCamera);
	scene->OnRender();
	Renderer::RenderEnd();
}

void GameLayer::OnDetach()
{
	SceneManager::Clear();
}

void GameLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Bind<EventWindowResize>(LION_BIND_EVENT(GameLayer::OnEventWindowResize));
}

bool GameLayer::OnEventWindowResize(const EventWindowResize& event)
{
	mCamera->OnResize(static_cast<float32>(event.GetWidth()), static_cast<float32>(event.GetHeight()));
	return false;
}
