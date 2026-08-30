#include "Engine.h"
#include "SceneManager.h"

#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Log.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Logic/SceneSerializer.h>

namespace Lion
{
	Reference<Scene> SceneManager::sActiveScene;
	std::string SceneManager::sActivePath;
	std::string SceneManager::sPendingPath;
	bool SceneManager::sUpdating = false;

	bool SceneManager::LoadScene(const std::string& path)
	{
		if (path.empty())
			return false;

		sPendingPath = path;
		return sUpdating || LoadPendingScene();
	}

	bool SceneManager::ReloadScene()
	{
		return !sActivePath.empty() && LoadScene(sActivePath);
	}

	Reference<Scene> SceneManager::GetActiveScene()
	{
		return sActiveScene;
	}

	const std::string& SceneManager::GetActivePath()
	{
		return sActivePath;
	}

	void SceneManager::SetActiveScene(const Reference<Scene>& scene, const std::string& path)
	{
		if (sActiveScene && sActiveScene != scene)
			sActiveScene->Clear();

		sActiveScene = scene;
		sActivePath = path;
		sPendingPath.clear();
	}

	void SceneManager::Update(float32 deltaTime)
	{
		if (!sPendingPath.empty() && !LoadPendingScene())
			return;

		if (!sActiveScene)
			return;

		const Reference<Scene> updating = sActiveScene;
		sUpdating = true;
		updating->OnUpdate(deltaTime);
		sUpdating = false;

		if (!sPendingPath.empty())
			LoadPendingScene();
	}

	void SceneManager::Clear()
	{
		if (sActiveScene)
			sActiveScene->Clear();

		sActiveScene.reset();
		sActivePath.clear();
		sPendingPath.clear();
		sUpdating = false;
	}

	bool SceneManager::LoadPendingScene()
	{
		if (sPendingPath.empty())
			return true;

		const std::string requested = std::move(sPendingPath);
		sPendingPath.clear();
		const std::string resolved = ResolveResourcePath(requested);
		Reference<Scene> scene = MakeReference<Scene>();

		if (!SceneSerializer::Deserialize(scene, resolved))
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[SceneManager] Could not load scene '{}'.", requested));
			return false;
		}

		SetActiveScene(scene, requested);
		Log::Console(LogLevel::Information, LION_FORMAT_TEXT("[SceneManager] Active scene: '{}'.", requested));
		return true;
	}
}
