#pragma once

namespace Lion
{
	class Entity;
	class Scene;

	// Persists and instantiates one reusable entity definition. A scene keeps the Assembly's
	// resource-relative path and the instance placement; all other authored state comes from this asset.
	class AssemblySerializer
	{
	public:
		// Writes the complete entity definition as a sealed .lnassembly asset.
		static LION_API bool Serialize(const Reference<Entity>& entity, const std::string& filePath);

		// Loads a detached definition. filePath may be absolute or resource-relative.
		static LION_API Reference<Entity> Deserialize(const std::string& filePath,
			const std::string& resourceRoot = {});

		// Creates a linked instance in scene. assemblyPath is kept exactly as the resource-relative identity
		// written into scene files; resourceRoot only resolves it for this run.
		static LION_API Reference<Entity> Instantiate(const Reference<Scene>& scene,
			const std::string& assemblyPath, const std::string& resourceRoot = {});

		// Reapplies the original definition to a live instance while preserving its scene placement.
		static LION_API bool Refresh(const Reference<Entity>& instance,
			const std::string& resourceRoot = {});
	};
}
