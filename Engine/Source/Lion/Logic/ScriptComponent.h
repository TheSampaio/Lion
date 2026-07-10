#pragma once

#include <Lion/Logic/Component.h>

namespace Lion
{
	class Script;

	// Attaches a native C++ script to an entity by name.
	//
	// The name refers to a class registered with LION_REGISTER_SCRIPT, so the binding survives
	// serialization and can be picked from a list in the editor. The instance is created when the
	// owning entity wakes, which is also when the simulation starts in the editor's play mode.
	class ScriptComponent : public Component
	{
	public:
		LION_API explicit ScriptComponent(const std::string& scriptName = std::string());
		LION_API ~ScriptComponent() override;

		void OnAwake() override;
		void OnUpdate() override;
		void OnDestroy() override;

		// Name of the bound script class (empty when none is assigned).
		const std::string& GetScriptName() const { return mScriptName; }

		// Rebinds the component to another registered script; the running instance is discarded.
		LION_API void SetScriptName(const std::string& name);

		// Whether a script instance currently exists (only while the scene is awake).
		bool HasInstance() const { return mInstance != nullptr; }

	private:
		std::string mScriptName;
		Scope<Script> mInstance;
	};
}
