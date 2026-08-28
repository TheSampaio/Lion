#include "GameLayer.h"

#include <Lion/Core/Filesystem.h>

using namespace Lion;

void GameLayer::OnCreate()
{
	mCamera = MakeReference<CameraOrthographic>();
	mScene = MakeReference<Scene>();

	const std::string scenePath = ResourceRootDirectory() + "Scenes/Main.lnscene";

	if (!SceneSerializer::Deserialize(mScene, scenePath))
	{
		Log::Console(LogLevel::Fatal, LION_FORMAT_TEXT("[Game] Could not load the main scene: '{}'.", scenePath));
		return;
	}

	mSceneCamera = mScene->FindComponent<Camera2D>();
}

void GameLayer::OnUpdate()
{
	mScene->OnUpdate();
}

void GameLayer::OnRender()
{
	if (mSceneCamera && mSceneCamera->IsEnabled())
	{
		const glm::vec2 position = mSceneCamera->GetViewPosition();
		mCamera->SetPosition(glm::vec3(position.x, position.y, 0.0f));

		mCamera->SetZoomLevel(mSceneCamera->GetZoomForViewportHeight(mCamera->GetViewportHeight()));
	}

	Renderer::RenderBegin(mCamera);
	mScene->OnRender();
	Renderer::RenderEnd();
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
