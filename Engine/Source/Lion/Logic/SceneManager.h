#pragma once

namespace Lion
{
	class Scene;

	// Owns the scene currently played by a game and applies scene changes at a safe frame boundary.
	//
	// LoadScene may be called from a component while its scene is updating. In that case the request is
	// queued until the update finishes, so no component destroys the scene whose callback is still running.
	class SceneManager
	{
	public:
		static LION_API bool LoadScene(const std::string& path);
		static LION_API bool ReloadScene();
		static LION_API Reference<Scene> GetActiveScene();
		static LION_API const std::string& GetActivePath();

		// Hosts call these to adopt an editor-authored scene, advance the active scene, or release it before
		// unloading a game module.
		static LION_API void SetActiveScene(const Reference<Scene>& scene, const std::string& path = {});
		static LION_API void Update(float32 deltaTime = -1.0f);
		static LION_API void Clear();

	private:
		static bool LoadPendingScene();

		static Reference<Scene> sActiveScene;
		static std::string sActivePath;
		static std::string sPendingPath;
		static bool sUpdating;
	};
}
