#include "Engine.h"
#include "ComponentRegistry.h"

#include <Lion/Logic/Component.h>

namespace Lion
{
	namespace
	{
		// Function-local statics: components register during static initialization, so the storage
		// must be constructed on first use rather than depending on translation-unit ordering.
		std::unordered_map<std::string, ComponentRegistry::Factory>& Factories()
		{
			static std::unordered_map<std::string, ComponentRegistry::Factory> factories;
			return factories;
		}

		std::vector<std::string>& RegisteredNames()
		{
			static std::vector<std::string> names;
			return names;
		}

		// Reverse map, so a live component (identified by its runtime type) yields its registered name.
		std::unordered_map<std::type_index, std::string>& TypeNames()
		{
			static std::unordered_map<std::type_index, std::string> typeNames;
			return typeNames;
		}

		// Names registered while a module was loading; dropped again when it is unloaded.
		bool& InModule()
		{
			static bool inModule = false;
			return inModule;
		}

		std::vector<std::string>& ModuleNames()
		{
			static std::vector<std::string> names;
			return names;
		}
	}

	void ComponentRegistry::Register(const std::string& name, std::type_index type, Factory factory)
	{
		if (name.empty() || !factory)
			return;

		const bool isNew = Factories().find(name) == Factories().end();
		Factories()[name] = std::move(factory);
		TypeNames().insert_or_assign(type, name);

		if (isNew)
		{
			RegisteredNames().push_back(name);

			// Only a name the module introduces is attributed to it: one that shadows an existing entry
			// must not take the original down with it when the module unloads.
			if (InModule())
				ModuleNames().push_back(name);
		}
	}

	Scope<Component> ComponentRegistry::Create(const std::string& name)
	{
		const auto it = Factories().find(name);
		return (it != Factories().end()) ? it->second() : nullptr;
	}

	bool ComponentRegistry::Contains(const std::string& name)
	{
		return Factories().find(name) != Factories().end();
	}

	const std::string& ComponentRegistry::GetName(std::type_index type)
	{
		static const std::string empty;
		const auto it = TypeNames().find(type);
		return (it != TypeNames().end()) ? it->second : empty;
	}

	const std::vector<std::string>& ComponentRegistry::GetNames()
	{
		return RegisteredNames();
	}

	void ComponentRegistry::BeginModule()
	{
		InModule() = true;
	}

	void ComponentRegistry::EndModule()
	{
		InModule() = false;
	}

	void ComponentRegistry::UnloadModule()
	{
		for (const std::string& name : ModuleNames())
		{
			Factories().erase(name);

			auto& names = RegisteredNames();
			names.erase(std::remove(names.begin(), names.end(), name), names.end());

			for (auto it = TypeNames().begin(); it != TypeNames().end(); )
				it = (it->second == name) ? TypeNames().erase(it) : std::next(it);
		}

		ModuleNames().clear();
	}
}
