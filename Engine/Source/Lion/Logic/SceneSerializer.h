#pragma once

namespace Lion
{
	class Entity;
	class Scene;

	// Reads and writes scenes as JSON: entity transforms, every registered component and the world gravity.
	class SceneSerializer
	{
	public:
		// Writes the scene to a JSON file. Returns false on failure.
		static LION_API bool Serialize(const Reference<Scene>& scene, const std::string& filePath);

		// Replaces the scene's contents with those loaded from a JSON file. Returns false on failure.
		static LION_API bool Deserialize(const Reference<Scene>& scene, const std::string& filePath);

		// Serializes the scene to an in-memory JSON string (no logging), e.g. for editor undo/redo.
		static LION_API std::string SerializeToString(const Reference<Scene>& scene);

		// Replaces the scene's contents from an in-memory JSON string. Returns false on failure.
		static LION_API bool DeserializeFromString(const Reference<Scene>& scene, const std::string& text);
		static LION_API bool DeserializeFromString(const Reference<Scene>& scene, const std::string& text,
			const std::string& resourceRoot);

		// Serializes a single entity (for the editor's copy/paste/duplicate clipboard).
		static LION_API std::string SerializeEntityToString(const Reference<Entity>& entity);

		// Rebuilds a single entity from JSON and adds it to the scene. Returns null on failure.
		static LION_API Reference<Entity> DeserializeEntityFromString(const Reference<Scene>& scene, const std::string& text);
		static LION_API Reference<Entity> DeserializeEntityFromString(const Reference<Scene>& scene,
			const std::string& text, const std::string& resourceRoot);

		// Assembly definitions use the complete authored entity rather than the compact linked-instance
		// representation scenes and the editor clipboard use.
		static LION_API std::string SerializeEntityDefinitionToString(const Reference<Entity>& entity);
		static LION_API Reference<Entity> DeserializeEntityDefinitionFromString(const std::string& text);
		static LION_API bool DeserializeEntityDefinitionInto(const Reference<Entity>& entity,
			const std::string& text, bool preserveTransform);
	};
}
