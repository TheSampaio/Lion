#pragma once

#include <typeindex>

namespace Lion
{
	class Component;

	// Registry of component classes, keyed by their type name.
	//
	// Components register themselves at start-up (see LION_REGISTER_COMPONENT), which lets scenes
	// reference them by name — so serialization and the editor's component picker work without
	// compile-time knowledge of the type. This is what makes user-defined components, compiled only
	// into the game module, addable and serializable from the engine and the editor.
	class ComponentRegistry
	{
	public:
		using Factory = std::function<Scope<Component>()>;

		// Registers a factory for 'name' and remembers the mapping from its type to that name (used to
		// look up a live component's name when serializing). Re-registering the same name replaces it.
		static LION_API void Register(const std::string& name, std::type_index type, Factory factory);

		// Instantiates a registered component, or returns null when the name is unknown/empty.
		static LION_API Scope<Component> Create(const std::string& name);

		static LION_API bool Contains(const std::string& name);

		// Registered name for a component type, or an empty string when the type was never registered.
		static LION_API const std::string& GetName(std::type_index type);

		// Registered component names, in registration order.
		static LION_API const std::vector<std::string>& GetNames();

		// Module scoping, for hot-reloading a game module. Everything registered between BeginModule
		// and EndModule is attributed to that module — bracket the library load, whose static
		// initializers do the registering. UnloadModule drops those entries again, which must happen
		// before the library is unloaded: their factories point into its code.
		static LION_API void BeginModule();
		static LION_API void EndModule();
		static LION_API void UnloadModule();
	};
}

// Registers a Component subclass under its own type name, at static-initialization time. The type
// must be default-constructible (the factory calls MakeScope<Type>()).
#define LION_REGISTER_COMPONENT(Type)                                                              \
	namespace                                                                                       \
	{                                                                                               \
		const bool kLionComponentRegistered_##Type = []                                             \
		{                                                                                           \
			::Lion::ComponentRegistry::Register(#Type, std::type_index(typeid(Type)),               \
				[]() -> ::Lion::Scope<::Lion::Component> { return ::Lion::MakeScope<Type>(); });     \
			return true;                                                                            \
		}();                                                                                        \
	}
