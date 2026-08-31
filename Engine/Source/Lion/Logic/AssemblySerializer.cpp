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

		file << Vault::Seal(SceneSerializer::SerializeEntityDefinitionToString(entity));

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
		std::string definition;

		if (!ReadDefinition(filePath, resourceRoot, definition))
			return nullptr;

		Reference<Entity> entity = SceneSerializer::DeserializeEntityDefinitionFromString(definition);

		if (!entity)
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[Assembly] Invalid definition '{}'.", filePath));

		return entity;
	}

	Reference<Entity> AssemblySerializer::Instantiate(const Reference<Scene>& scene,
		const std::string& assemblyPath, const std::string& resourceRoot)
	{
		if (!scene || assemblyPath.empty())
			return nullptr;

		Reference<Entity> instance = Deserialize(assemblyPath, resourceRoot);

		if (!instance)
			return nullptr;

		instance->SetAssemblyPath(std::filesystem::path(assemblyPath).generic_string());
		scene->Add(instance);
		return instance;
	}

	bool AssemblySerializer::Refresh(const Reference<Entity>& instance, const std::string& resourceRoot)
	{
		if (!instance || !instance->IsAssemblyInstance())
			return false;

		const std::string assemblyPath = instance->GetAssemblyPath();
		std::string definition;

		if (!ReadDefinition(assemblyPath, resourceRoot, definition)
			|| !SceneSerializer::DeserializeEntityDefinitionInto(instance, definition, true))
			return false;

		instance->SetAssemblyPath(assemblyPath);
		return true;
	}
}
