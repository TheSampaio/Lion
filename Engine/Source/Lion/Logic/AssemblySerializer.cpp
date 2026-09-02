#include "Engine.h"
#include "AssemblySerializer.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Log.h>
#include <Lion/Core/Vault.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Logic/SceneSerializer.h>

namespace Lion
{
	namespace
	{
		std::filesystem::path ResolveAssemblyPath(const std::string& path, const std::string& resourceRoot)
		{
			const std::filesystem::path requested(path);

			if (requested.is_absolute())
				return requested;

			if (!resourceRoot.empty())
				return std::filesystem::path(resourceRoot) / requested;

			return ResolveResourcePath(path);
		}

		bool ReadDefinition(const std::string& path, const std::string& resourceRoot, std::string& definition)
		{
			const std::filesystem::path resolved = ResolveAssemblyPath(path, resourceRoot);
			std::ifstream file(resolved, std::ios::binary);

			if (!file.is_open())
			{
				Log::Console(LogLevel::Warning,
					LION_FORMAT_TEXT("[Assembly] Failed to open '{}'.", resolved.generic_string()));
				return false;
			}

			std::stringstream buffer;
			buffer << file.rdbuf();
			definition = Vault::Unseal(buffer.str());
			return true;
		}
	}

	bool AssemblySerializer::Serialize(const Reference<Entity>& entity, const std::string& filePath)
	{
		if (!entity)
			return false;

		std::ofstream file(filePath, std::ios::binary | std::ios::trunc);

		if (!file.is_open())
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Assembly] Failed to write '{}'.", filePath));
			return false;
		}

		file << Vault::Seal(SceneSerializer::SerializeEntityTreeDefinitionToString(entity));

		if (!file.good())
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Assembly] Failed to finish '{}'.", filePath));
			return false;
		}

		Log::Console(LogLevel::Success, LION_FORMAT_TEXT("[Assembly] Saved '{}'.", filePath));
		return true;
	}

	Reference<Entity> AssemblySerializer::Deserialize(const std::string& filePath, const std::string& resourceRoot)
	{
		std::vector<Reference<Entity>> tree = DeserializeTree(filePath, resourceRoot);

		if (tree.empty())
			return nullptr;

		// This compatibility entry point predates hierarchical Assemblies. Keep returning a safe detached
		// root for old callers; hierarchy-aware code owns the complete vector through DeserializeTree.
		for (size_t index = 1; index < tree.size(); ++index)
			tree[index]->SetParent(nullptr, false);

		return tree.front();
	}

	std::vector<Reference<Entity>> AssemblySerializer::DeserializeTree(const std::string& filePath,
		const std::string& resourceRoot)
	{
		std::string definition;

		if (!ReadDefinition(filePath, resourceRoot, definition))
			return {};

		std::vector<Reference<Entity>> entities =
			SceneSerializer::DeserializeEntityTreeDefinitionFromString(definition);

		if (entities.empty())
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Assembly] Invalid definition '{}'.", filePath));

		return entities;
	}

	Reference<Entity> AssemblySerializer::Instantiate(const Reference<Scene>& scene,
		const std::string& assemblyPath, const std::string& resourceRoot)
	{
		if (!scene || assemblyPath.empty())
			return nullptr;

		std::vector<Reference<Entity>> tree = DeserializeTree(assemblyPath, resourceRoot);

		if (tree.empty())
			return nullptr;

		Reference<Entity> instance = tree.front();
		instance->SetAssemblyPath(std::filesystem::path(assemblyPath).generic_string());

		for (const auto& entity : tree)
			scene->Add(entity);

		return instance;
	}

	bool AssemblySerializer::Refresh(const Reference<Entity>& instance, const std::string& resourceRoot)
	{
		if (!instance || !instance->IsAssemblyInstance())
			return false;

		const std::string assemblyPath = instance->GetAssemblyPath();
		const bool visible = instance->IsVisible();
		const Transform& source = instance->mAssemblySourceTransform;
		const Reference<Transform> transform = instance->GetTransform();
		const Vector2 positionOffset = transform->GetPosition() - source.GetPosition();
		const float32 rotationOffset = transform->GetRotation() - source.GetRotation();
		const Vector2 currentScale = transform->GetScale();
		const Vector2 sourceScale = source.GetScale();
		constexpr float32 kMinimumScale = 0.0001f;
		const Vector2 scaleFactor(
			std::fabs(sourceScale.x) > kMinimumScale ? currentScale.x / sourceScale.x : 1.0f,
			std::fabs(sourceScale.y) > kMinimumScale ? currentScale.y / sourceScale.y : 1.0f);
		std::string definition;

		if (!ReadDefinition(assemblyPath, resourceRoot, definition))
			return false;

		const std::vector<Reference<Entity>> sourceTree =
			SceneSerializer::DeserializeEntityTreeDefinitionFromString(definition);

		if (sourceTree.empty())
			return false;

		const Transform refreshedSource = *sourceTree.front()->GetTransform();
		transform->SetPosition(refreshedSource.GetPosition() + positionOffset);
		transform->SetRotation(refreshedSource.GetRotation() + rotationOffset);
		transform->SetScale(refreshedSource.GetScale() * scaleFactor);

		// Keep the final root Transform in place while components Awake so physics bodies are created at the
		// instance placement, not briefly at the raw authored Transform.
		if (!SceneSerializer::DeserializeEntityTreeDefinitionInto(
			instance->GetScene(), instance, definition, true))
			return false;

		instance->mAssemblyPath = assemblyPath;
		instance->mAssemblySourceTransform = refreshedSource;
		instance->SetVisible(visible);
		return true;
	}
}
