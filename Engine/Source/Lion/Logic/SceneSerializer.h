#pragma once

namespace Lion
{
	class Scene;

	// Reads and writes scenes as JSON: entity transforms and their components
	// (SpriteRenderer, RigidBody2D, BoxCollider2D, CircleCollider2D) plus the world gravity.
	class SceneSerializer
	{
	public:
		// Writes the scene to a JSON file. Returns false on failure.
		static LION_API bool Serialize(const Reference<Scene>& scene, const std::string& filePath);

		// Replaces the scene's contents with those loaded from a JSON file. Returns false on failure.
		static LION_API bool Deserialize(const Reference<Scene>& scene, const std::string& filePath);
	};
}
